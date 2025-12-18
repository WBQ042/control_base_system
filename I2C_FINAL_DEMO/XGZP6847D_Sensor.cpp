#include "XGZP6847D_Sensor.h"
#include <Arduino.h>

XGZP6847D_Sensor::XGZP6847D_Sensor(TCA9548A_Mux *mux_ptr, uint8_t muxChannel,
                                   float pressure_k) {
  _mux = mux_ptr;
  _muxChannel = muxChannel;
  _pressure_k = pressure_k;
}

// 辅助函数：切换Mux通道
void XGZP6847D_Sensor::selectMuxChannel() {
  if (_mux) {
    _mux->selectChannel(_muxChannel);
  }
  // 注意：如果_mux为空，说明没有使用多路复用器，直接继续I2C通信
}

// 辅助函数：写入寄存器
bool XGZP6847D_Sensor::writeRegister(uint8_t reg, uint8_t data) {
  selectMuxChannel(); // 切换通道
  Wire.beginTransmission(XGZP6847D_ADDRESS);
  Wire.write(reg);
  Wire.write(data);
  return (Wire.endTransmission() == 0);
}

// 辅助函数：读取寄存器
uint8_t XGZP6847D_Sensor::readRegister(uint8_t reg) {
  selectMuxChannel(); // 切换通道

  // 1. 写寄存器地址 [cite: 445]
  Wire.beginTransmission(XGZP6847D_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false); // 保持连接，进行Re-start [cite: 446]

  // 2. 读取1个字节 [cite: 446]
  uint8_t receivedData = 0;
  if (Wire.requestFrom(XGZP6847D_ADDRESS, 1) == 1) {
    receivedData = Wire.read();
  } else {
    Serial.print("[XGZP6847D Error] 无法从寄存器 0x");
    Serial.print(reg, HEX);
    Serial.println(" 读取数据。");
  }
  return receivedData;
}

// 初始化（设置高过采样率）
bool XGZP6847D_Sensor::begin() {
  Serial.print("XGZP6847D (Channel ");
  Serial.print(_muxChannel);
  Serial.println(") 初始化中...");

  // 目标：提高精度。配置 OSR_P<2:0> 为 110b (16384X)
  // 寄存器 0xA6 P_config 的 Bit 2:0 为 OSR_P [cite: 499, 524]
  // 默认 P_config 的 Bit 5:3 Gain_P 为 000b (1X)，Bit 7:6 Input Swap 为 00b
  // [cite: 499, 522, 521]

  // 我们只修改 OSR_P 为 110b (6)
  // OSR_P<2:0> = 6 (110b)
  uint8_t configByte = 0x06;

  if (!writeRegister(REG_P_CONFIG, configByte)) {
    Serial.println("[XGZP6847D Error] 无法设置 P_CONFIG 寄存器。");
    return false;
  }

  Serial.println("XGZP6847D OSR设置为16384X，精度提升。");
  return true;
}

// 启动测量
bool XGZP6847D_Sensor::startMeasurement() {
  // 写入 0x0A 到 0x30 寄存器，启动组合测量 [cite: 532]
  return writeRegister(REG_CMD, CMD_START_COMBINED_MEAS);
}

// 读取并计算数据
bool XGZP6847D_Sensor::readData(float &pressurePa, float &tempC) {
  // 1. 检查数据采集是否完成
  uint8_t status = readRegister(REG_CMD);
  if ((status & STATUS_SCO_BIT) > 0) {
    // SCO = 1，采集未完成 [cite: 507]
    // 规格书建议延迟 20ms 后再次检查或直接等待 [cite: 533]
    Serial.println("[XGZP6847D Warning] 数据未准备好，正在等待...");

    // 增加一个简单的忙等循环 (最多等待 100ms)
    unsigned long startTime = millis();
    while ((readRegister(REG_CMD) & STATUS_SCO_BIT) > 0) {
      if (millis() - startTime > 100) {
        Serial.println("[XGZP6847D Error] 测量超时。");
        return false;
      }
      delay(1);
    }
  }

  // 2. 读取压力数据 (24-bit)
  selectMuxChannel();
  Wire.beginTransmission(XGZP6847D_ADDRESS);
  Wire.write(REG_PRESSURE_MSB); // 从0x06开始 [cite: 534]
  Wire.endTransmission(false);

  // 3. 读取3个压力字节 (0x06, 0x07, 0x08)
  int32_t pressure_adc_signed = 0;
  if (Wire.requestFrom(XGZP6847D_ADDRESS, 3) == 3) {
    uint8_t h = Wire.read();
    uint8_t m = Wire.read();
    uint8_t l = Wire.read();

    // 组合 24 位原始数据
    uint32_t val = ((uint32_t)h << 16) | ((uint32_t)m << 8) | l;

    // 符号扩展 (24-bit -> 32-bit)
    // 如果第24位(Bit 23)是1，说明是负数，高8位全置1
    if (val & 0x800000) {
      val |= 0xFF000000;
    }

    pressure_adc_signed = (int32_t)val;
  } else {
    Serial.println("[XGZP6847D Error] 无法读取压力数据。");
    return false;
  }

  // 4. 读取温度数据 (16-bit)
  selectMuxChannel();
  Wire.beginTransmission(XGZP6847D_ADDRESS);
  Wire.write(REG_TEMP_MSB); // 从0x09开始 [cite: 534]
  Wire.endTransmission(false);

  // 读取2个温度字节 (0x09, 0x0A)
  // 同样使用符号扩展逻辑处理 16-bit 温度
  int16_t temperature_adc_signed = 0;
  if (Wire.requestFrom(XGZP6847D_ADDRESS, 2) == 2) {
    uint8_t h = Wire.read();
    uint8_t l = Wire.read();
    // int16_t 直接强转即可正确处理补码 (如果是负数)
    temperature_adc_signed = (int16_t)((h << 8) | l);
  } else {
    Serial.println("[XGZP6847D Error] 无法读取温度数据。");
    return false;
  }

  // 5. 计算最终物理量
  // 补码逻辑下，正负数除法计算公式统一
  pressurePa = (float)pressure_adc_signed / _pressure_k;
  tempC = (float)temperature_adc_signed / 256.0f;

  return true;
}
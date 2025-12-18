#include "TCA9548A_Mux.h"
#include <Arduino.h>

// 构造函数
TCA9548A_Mux::TCA9548A_Mux(uint8_t addr) { _muxAddress = addr; }

// 初始化（对于TCA9548A，主要是设置地址）
void TCA9548A_Mux::begin() {
  Wire.begin(); // 确保Wire库已初始化
  // 建议在初始化时禁用所有通道，确保一个干净的开始
  disableAllChannels();
}

// 切换到指定的I2C通道
bool TCA9548A_Mux::selectChannel(uint8_t channel) {
  // 检查通道号是否有效 (0 到 7)
  if (channel > 7) {
    Serial.println("[TCA9548A_Mux ERROR] 无效通道号。通道必须在 0 到 7 之间。");
    return false;
  }

  // 借鉴 I2CMux.cpp 的稳健逻辑：
  // 1. 先禁用所有通道，确保一个干净的状态
  // (虽然理论上可以直接覆盖，但在某些不稳定的I2C总线上，先Reset更安全)
  Wire.beginTransmission(_muxAddress);
  Wire.write(0);
  Wire.endTransmission();
  // 短暂延时，让Mux内部逻辑复位
  delay(2);

  // 2. 选择目标通道
  Wire.beginTransmission(_muxAddress);
  Wire.write(1 << channel);
  uint8_t result = Wire.endTransmission();

  if (result != 0) {
    Serial.print("[TCA9548A_Mux ERROR] 切换通道 ");
    Serial.print(channel);
    Serial.print(" 失败。I2C 错误代码: ");
    Serial.println(result);
    return false;
  }

  // 3. 再次延时，确保通道切换后的电气特性稳定
  // (I2CMux.cpp用了20ms，这里保守给5ms)
  // 如果之后发现还是不稳定，可以增加这个延时
  delay(5);

  return true;
}

// 重置TCA9548A，断开所有通道
bool TCA9548A_Mux::disableAllChannels() {
  // 写入0x00到控制寄存器，断开所有通道
  Wire.beginTransmission(_muxAddress);
  Wire.write(0x00);
  uint8_t result = Wire.endTransmission();

  if (result != 0) {
    Serial.print("[TCA9548A_Mux ERROR] 禁用所有通道失败。I2C 错误代码: ");
    Serial.println(result);
    return false;
  }
  return true;
}
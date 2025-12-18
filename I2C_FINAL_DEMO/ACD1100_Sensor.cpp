#include "ACD1100_Sensor.h"
#include <Arduino.h>

ACD1100_Sensor::ACD1100_Sensor(TCA9548A_Mux* mux_ptr, uint8_t muxChannel) {
    _mux = mux_ptr;
    _muxChannel = muxChannel;
    _lastError = OK;
}

// 切换Mux通道
void ACD1100_Sensor::selectMuxChannel() {
    if (_mux) {
        _mux->selectChannel(_muxChannel);
    }
}

// 初始化
bool ACD1100_Sensor::begin() {
    selectMuxChannel();
    Wire.beginTransmission(ACD1100_I2C_ADDR);
    // 检查设备是否存在
    if (Wire.endTransmission() == 0) {
        Serial.print("ACD1100 (Channel ");
        Serial.print(_muxChannel);
        Serial.println(") 初始化成功。");
        // 手册建议预热120秒 [cite: 368]，我们在此提醒用户
        Serial.println("[ACD1100 Info] 传感器需要120秒预热时间以达到标定精度。");
        return true;
    } else {
        Serial.print("[ACD1100 Error] 传感器 (Channel ");
        Serial.print(_muxChannel);
        Serial.println(") 未响应。");
        return false;
    }
}

// 获取最后错误
ACD1100_Sensor::ACD1100_Error ACD1100_Sensor::getLastError() const {
    return _lastError;
}

// CRC-8 校验 (来自手册第7页 [cite: 486-497])
uint8_t ACD1100_Sensor::crc8(const uint8_t *data, uint8_t len) {
    uint8_t bit, byte, crc = 0xFF; // 初始值 0xFF [cite: 403]
    for (byte = 0; byte < len; byte++) {
        crc ^= (data[byte]);
        for (bit = 8; bit > 0; --bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31; // 多项式 0x31 [cite: 403]
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

// 读取数据
bool ACD1100_Sensor::readData(uint32_t &co2_ppm, int16_t &raw_temp) {
    _lastError = OK;
    selectMuxChannel();
    
    // 1. 发送读取命令 (0x0300) [cite: 409]
    Wire.beginTransmission(ACD1100_I2C_ADDR);
    Wire.write(ACD1100_CMD_READ_CO2_HI);
    Wire.write(ACD1100_CMD_READ_CO2_LO);
    if (Wire.endTransmission() != 0) {
        Serial.println("[ACD1100 Error] 发送读取命令失败。");
        _lastError = ERROR_I2C_READ;
        return false;
    }

    // 2. 请求 9 字节的上行数据 [cite: 419]
    // 传感器数据刷新频率 2s [cite: 368]，发送命令后立即读取即可
    if (Wire.requestFrom(ACD1100_I2C_ADDR, 9) != 9) {
        Serial.println("[ACD1100 Error] 读取数据字节数不足 (应为9)。");
        _lastError = ERROR_I2C_READ;
        return false;
    }

    uint8_t buffer[9];
    for (int i = 0; i < 9; i++) {
        buffer[i] = Wire.read();
    }

    // 3. 严格执行 CRC 校验 ( addressing user's concern)
    // 校验第1组: CO2高位 (PPM3, PPM2) [cite: 419]
    uint8_t crc1_calc = crc8(&buffer[0], 2);
    if (crc1_calc != buffer[2]) {
        Serial.println("[ACD1100 Error] CRC1 (CO2高位) 校验失败。");
        _lastError = ERROR_CRC_CO2_A;
        return false;
    }

    // 校验第2组: CO2低位 (PPM1, PPM0) [cite: 419]
    uint8_t crc2_calc = crc8(&buffer[3], 2);
    if (crc2_calc != buffer[5]) {
        Serial.println("[ACD1100 Error] CRC2 (CO2低位) 校验失败。");
        _lastError = ERROR_CRC_CO2_B;
        return false;
    }

    // 校验第3组: 温度 (TEMP, TEMO) [cite: 419]
    uint8_t crc3_calc = crc8(&buffer[6], 2);
    if (crc3_calc != buffer[8]) {
        Serial.println("[ACD1100 Error] CRC3 (温度) 校验失败。");
        _lastError = ERROR_CRC_TEMP;
        return false;
    }

    // 4. CRC 均通过，组合数据
    
    // CO2 (PPM3, PPM2, PPM1, PPM0) [cite: 484]
    co2_ppm = ((uint32_t)buffer[0] << 24) |
              ((uint32_t)buffer[1] << 16) |
              ((uint32_t)buffer[3] << 8)  |
              ((uint32_t)buffer[4]);

    // 原始温度 (TEMP, TEMO)
    raw_temp = ((int16_t)buffer[6] << 8) | buffer[7];

    return true;
}
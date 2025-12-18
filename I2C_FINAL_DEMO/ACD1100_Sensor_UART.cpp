#include "ACD1100_Sensor_UART.h"
#include <Arduino.h>

ACD1100_Sensor_UART::ACD1100_Sensor_UART(Stream &serialPort) {
    _port = &serialPort;
    _lastError = OK;
}

// 清空缓冲区
void ACD1100_Sensor_UART::flushSerial() {
    while (_port->available()) {
        _port->read();
    }
}

// 计算校验和 
uint8_t ACD1100_Sensor_UART::calculateChecksum(const uint8_t* data, uint8_t len) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

// 发送命令
void ACD1100_Sensor_UART::sendCommand(const uint8_t* cmd, uint8_t len) {
    flushSerial();
    _port->write(cmd, len);
}

// 读取响应 (这是最复杂的部分)
int ACD1100_Sensor_UART::readResponse(uint8_t expectedCmd, uint8_t expectedLen) {
    unsigned long startTime = millis();
    int bytesRead = 0;
    
    // 1. 等待并查找帧头 (FE)
    while (millis() - startTime < 1000) { // 1秒超时
        if (_port->available()) {
            if (_port->read() == ACD1100_UART_HEADER) {
                _buffer[0] = ACD1100_UART_HEADER;
                bytesRead = 1;
                break;
            }
        }
    }
    if (bytesRead == 0) {
        _lastError = ERROR_TIMEOUT;
        return 0; // 超时
    }

    // 2. 查找固定码 (A6)
    startTime = millis();
    while (bytesRead < 2) {
        if (_port->available()) {
            _buffer[1] = _port->read();
            if (_buffer[1] == ACD1100_UART_FIXED_CODE) {
                bytesRead = 2;
                break;
            } else {
                 _lastError = ERROR_BAD_HEADER;
                 return 0; // 帧头错误
            }
        }
        if (millis() - startTime > 100) {
             _lastError = ERROR_TIMEOUT;
             return 0;
        }
    }
    
    // 3. 读取剩余的数据 (长度 + 命令 + Payload + CS)
    // 总长度 = 2 (Header) + 1 (Len) + 1 (Cmd) + expectedLen (Payload) + 1 (CS)
    int totalExpected = 5 + expectedLen;
    startTime = millis();
    while (bytesRead < totalExpected) {
        if (_port->available()) {
            _buffer[bytesRead++] = _port->read();
        }
        if (millis() - startTime > 500) { // 读取剩余数据的超时
            _lastError = ERROR_TIMEOUT;
            return 0;
        }
    }

    // 4. 验证数据包
    uint8_t dataLen = _buffer[2];
    uint8_t cmd = _buffer[3];
    
    if (dataLen != expectedLen) {
        _lastError = ERROR_BAD_HEADER; // 长度不匹配
        return 0;
    }
    if (cmd != expectedCmd) {
        _lastError = ERROR_BAD_COMMAND; // 命令不匹配
        return 0;
    }

    // 5. 验证校验和 
    // 校验和计算范围: A6 + 长度 + 命令 + 数据
    uint8_t lenForCS = 3 + expectedLen; // (A6, Len, Cmd) + Payload
    uint8_t cs_calc = calculateChecksum(&_buffer[1], lenForCS);
    uint8_t cs_recv = _buffer[totalExpected - 1]; // 最后一个字节

    if (cs_calc != cs_recv) {
        _lastError = ERROR_CHECKSUM;
        return 0;
    }
    
    _lastError = OK;
    return bytesRead; // 成功
}

bool ACD1100_Sensor_UART::begin() {
    // 尝试发送 "读取软件版本" 命令 [cite: 195]
    // 发送: FE A6 00 1E C4
    const uint8_t cmd[] = {0xFE, 0xA6, 0x00, 0x1E, 0xC4};
    sendCommand(cmd, sizeof(cmd));

    // 应答: FE A6 0B 1E D1~D11 CS (共 16 字节) [cite: 195]
    // 期望数据 payload 长度为 11
    if (readResponse(0x1E, 11)) { 
        Serial.print("[ACD1100_UART] 初始化成功。版本: ");
        // 打印版本号 (ASCII)
        for(int i=0; i<10; i++) {
            Serial.print((char)_buffer[4+i]);
        }
        Serial.println();
        return true;
    } else {
        Serial.println("[ACD1100_UART] 初始化失败。");
        return false;
    }
}

bool ACD1100_Sensor_UART::readData(uint16_t &co2_ppm) {
    // 1. 发送 "读取CO2浓度" 命令 [cite: 183]
    // 发送: FE A6 00 01 A7
    const uint8_t cmd[] = {0xFE, 0xA6, 0x00, 0x01, 0xA7};
    sendCommand(cmd, sizeof(cmd));

    // 2. 读取应答 [cite: 183]
    // 应答: FE A6 04 01 D1 D2 D3 D4 CS (共 9 字节)
    // 期望数据 payload 长度为 4
    if (readResponse(ACD1100_CMD_READ_CO2, 4)) {
        // CO2浓度值 = D1 * 256 + D2 [cite: 183]
        co2_ppm = (uint16_t)(_buffer[4] << 8) | _buffer[5];
        return true;
    }
    
    return false; // 错误已在 readResponse 中设置
}

bool ACD1100_Sensor_UART::setCalibrationMode(bool autoMode) {
    // 3. 设置校准模式 [cite: 189]
    // 发送: FE A6 02 04 00 D1 CS
    uint8_t cmd[7] = {0xFE, 0xA6, 0x02, 0x04, 0x00, 0x00, 0x00};
    cmd[5] = autoMode ? 0x01 : 0x00; // D1

    // 计算 CS: A6 + 02 + 04 + 00 + D1
    cmd[6] = calculateChecksum(&cmd[1], 5);
    
    sendCommand(cmd, sizeof(cmd));

    // 4. 读取应答 [cite: 189]
    // 应答: FE A6 00 04 CS (共 6 字节)
    // 期望数据 payload 长度为 0
    if (readResponse(ACD1100_CMD_SET_CAL_MODE, 0)) {
        return true;
    }

    _lastError = ERROR_WRITE_FAIL;
    return false;
}

ACD1100_Sensor_UART::ACD1100_UART_Error ACD1100_Sensor_UART::getLastError() const {
    return _lastError;
}
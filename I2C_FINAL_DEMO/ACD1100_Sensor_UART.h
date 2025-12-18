#ifndef ACD1100_SENSOR_UART_H
#define ACD1100_SENSOR_UART_H

#include <Arduino.h>
#include <Stream.h> // 使用Stream基类，使其兼容HardwareSerial

// 协议帧定义
#define ACD1100_UART_HEADER     0xFE
#define ACD1100_UART_FIXED_CODE 0xA6

// 命令码
#define ACD1100_CMD_READ_CO2    0x01
#define ACD1100_CMD_SET_CAL_MODE 0x04

class ACD1100_Sensor_UART {
public:
    enum ACD1100_UART_Error {
        OK = 0,
        ERROR_TIMEOUT,
        ERROR_BAD_HEADER,
        ERROR_BAD_COMMAND,
        ERROR_CHECKSUM,
        ERROR_WRITE_FAIL
    };

    /**
     * @brief 构造函数
     * @param serialPort 对硬件串口的引用 (例如 Serial1)
     */
    ACD1100_Sensor_UART(Stream &serialPort);

    /**
     * @brief 检查传感器连接并读取版本号 (用于初始化)
     * @note 您必须在此函数之前调用 serialPort.begin(1200);
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 读取CO2浓度
     * @param co2_ppm 输出CO2浓度 (ppm)
     * @return true 读取和校验成功, false 失败
     */
    bool readData(uint16_t &co2_ppm);

    /**
     * @brief 设置校准模式 (手册默认自动校准)
     * @param autoMode true: 自动校准 [cite: 189], false: 手动校准 [cite: 189]
     * @return true 设置成功
     */
    bool setCalibrationMode(bool autoMode);

    /**
     * @brief 获取最后一次读取时的错误状态
     * @return ACD1100_UART_Error 错误码
     */
    ACD1100_UART_Error getLastError() const;

private:
    Stream* _port;
    ACD1100_UART_Error _lastError;
    uint8_t _buffer[16]; // 接收缓冲区

    /**
     * @brief 计算UART协议的校验和
     * @param data 指向 "固定码" (0xA6) 的指针
     * @param len 要累加的字节数 (固定码+长度+命令+数据)
     * @return 8位校验和
     */
    uint8_t calculateChecksum(const uint8_t* data, uint8_t len);

    /**
     * @brief 发送命令
     * @param cmd 命令缓冲区
     * @param len 命令长度
     */
    void sendCommand(const uint8_t* cmd, uint8_t len);

    /**
     * @brief 读取并验证响应
     * @param expectedCmd 期望的命令回显
     * @param expectedLen 期望的数据 payload 长度
     * @return 实际读取到的总字节数，0表示失败
     */
    int readResponse(uint8_t expectedCmd, uint8_t expectedLen);
    
    /**
     * @brief 清空串口接收缓冲区
     */
    void flushSerial();
};

#endif // ACD1100_SENSOR_UART_H
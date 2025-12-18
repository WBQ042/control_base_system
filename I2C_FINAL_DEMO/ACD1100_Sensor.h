#ifndef ACD1100_SENSOR_H
#define ACD1100_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "TCA9548A_Mux.h" // 依赖于Mux库

// 传感器的7位I2C地址 (0x54 >> 1)
#define ACD1100_I2C_ADDR        0x2A

// 读取CO2的命令
#define ACD1100_CMD_READ_CO2_HI 0x03
#define ACD1100_CMD_READ_CO2_LO 0x00

class ACD1100_Sensor {
public:
    enum ACD1100_Error {
        OK = 0,
        ERROR_I2C_READ, // I2C读取失败
        ERROR_CRC_CO2_A, // CO2高位CRC校验失败
        ERROR_CRC_CO2_B, // CO2低位CRC校验失败
        ERROR_CRC_TEMP, // 温度CRC校验失败
    };

    /**
     * @brief 构造函数
     * @param mux_ptr 指向TCA9548A_Mux的指针
     * @param muxChannel 连接到TCA9548A的通道号 (0-7)
     */
    ACD1100_Sensor(TCA9548A_Mux* mux_ptr, uint8_t muxChannel);

    /**
     * @brief 初始化传感器 (实际上只是检查连接)
     * @return true 成功，false 失败
     */
    bool begin();

    /**
     * @brief 读取CO2浓度和原始温度数据
     * @param co2_ppm 输出CO2浓度 (ppm)
     * @param raw_temp 输出原始温度数据 (手册未提供换算公式)
     * @return true 数据读取且CRC校验成功, false 失败
     */
    bool readData(uint32_t &co2_ppm, int16_t &raw_temp);

    /**
     * @brief 获取最后一次读取时的错误状态
     * @return ACD1100_Error 错误码
     */
    ACD1100_Error getLastError() const;

private:
    TCA9548A_Mux* _mux;
    uint8_t _muxChannel;
    ACD1100_Error _lastError;

    // 切换到正确的Mux通道
    void selectMuxChannel();

    /**
     * @brief CRC-8 校验 (基于手册 [cite: 486-497])
     * @param data 数据指针
     * @param len 数据长度
     * @return 8位CRC值
     */
    uint8_t crc8(const uint8_t *data, uint8_t len);
};

#endif // ACD1100_SENSOR_H
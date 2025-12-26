#ifndef XGZP6847D_SENSOR_H
#define XGZP6847D_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "TCA9548A_Mux.h" // 依赖于Mux库

// I2C 地址和寄存器定义
#define XGZP6847D_ADDRESS       0x6D    // 传感器从地址
#define REG_CMD                 0x30    // 命令寄存器
#define REG_PRESSURE_MSB        0x06    // 压力数据 MSB
#define REG_TEMP_MSB            0x09    // 温度数据 MSB

// 组合采集命令：SCO=1, Measurement_ctrl=010b -> 0x0A
#define CMD_START_COMBINED_MEAS 0x0A    
// 状态位掩码：SCO位于Bit3
#define STATUS_SCO_BIT          0x08    

// 配置寄存器
#define REG_P_CONFIG            0xA6    // 压力配置寄存器

class XGZP6847D_Sensor {
public:
    /**
     * @brief 构造函数
     * @param mux_ptr 指向TCA9548A_Mux的指针，用于通道切换
     * @param muxChannel 连接到TCA9548A的通道号 (0-7)
     * @param pressure_k 压力换算系数k (取决于传感器量程，例如100kPa量程 k=64) 
     */
    XGZP6847D_Sensor(TCA9548A_Mux* mux_ptr, uint8_t muxChannel, float pressure_k);

    /**
     * @brief 初始化传感器 (设置高过采样率以提高精度)
     * @return true 成功，false 失败
     */
    bool begin();

    /**
     * @brief 启动一次组合采集 (压力和温度)
     * @return true 成功，false 失败
     */
    bool startMeasurement();

    /**
     * @brief 读取并计算校准后的压力和温度
     * @param pressurePa 输出压力值 (Pa)
     * @param tempC 输出温度值 (℃)
     * @return true 成功，false 失败 (例如数据未准备好)
     */
    bool readData(float &pressurePa, float &tempC);

private:
    TCA9548A_Mux* _mux;
    uint8_t _muxChannel;
    float _pressure_k;

    // 私有函数：读写I2C寄存器
    uint8_t readRegister(uint8_t reg);
    bool writeRegister(uint8_t reg, uint8_t data);
    
    // 切换到正确Mux通道的辅助函数
    void selectMuxChannel();
};

#endif // XGZP6847D_SENSOR_H

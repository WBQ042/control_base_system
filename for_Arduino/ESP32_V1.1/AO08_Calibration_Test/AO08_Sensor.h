#ifndef AO08_SENSOR_H
#define AO08_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "I2CMux.h"
#include "AO08_CalibrationStorage.h"

/**
 * @class AO08_Sensor
 * @brief 使用 ADS1115 ADC 读取 AO-08 氧气传感器，支持参数存储
 * 
 * 基于 example_demo 中的 AO08_Sensor_ADS1115，但使用 I2CMux 替代 TCA9548A_Mux，
 * 并集成了校准参数存储功能。
 */
class AO08_Sensor {
public:
    enum AO08_Error {
        OK = 0,
        ERROR_I2C,              // I2C 通信失败
        ERROR_TIMEOUT,          // ADC 转换超时
        ERROR_NOT_CALIBRATED,   // 传感器未校准
        ERROR_CALIBRATION_FAILED // 校准失败
    };

    /**
     * @brief 构造函数
     * @param mux_ptr 指向 I2CMux 多路复用器的指针（可为 nullptr）
     * @param muxChannel ADS1115 连接到的 Mux 通道号
     * @param adsAddress ADS1115 的 I2C 地址 (默认为 0x48)
     */
    AO08_Sensor(I2CMux* mux_ptr, uint8_t muxChannel, uint8_t adsAddress = 0x48);

    /**
     * @brief 初始化 ADS1115 (检查 I2C 连接)
     * @return true 成功, false 失败
     */
    bool begin();

    /**
     * @brief 读取传感器原始的差分电压 (单位: mV)
     * @param voltage_mV 输出读取到的电压值 (毫伏)
     * @return true 读取成功, false 失败
     */
    bool readVoltage(float &voltage_mV);

    /**
     * @brief 执行零点校准并保存参数
     * @note 执行此操作前，必须将 Vsensor+ 和 Vsensor- 短接
     * @param saveToStorage 是否保存到非易失性存储（默认: true）
     * @return true 校准成功
     */
    bool calibrateZero(bool saveToStorage = true);

    /**
     * @brief 执行空气点校准 (20.9% O2) 并保存参数
     * @note 执行此操作前，必须将传感器暴露在新鲜空气中
     * @param saveToStorage 是否保存到非易失性存储（默认: true）
     * @return true 校准成功
     */
    bool calibrateAir(bool saveToStorage = true);

    /**
     * @brief 从存储中加载校准参数
     * @return true 加载成功, false 加载失败或参数无效
     */
    bool loadCalibrationFromStorage();

    /**
     * @brief 读取计算后的氧气百分比
     * @note 必须先执行校准或从存储加载校准参数
     * @param oxygen_percent 输出氧气百分比 (0-100%)
     * @return true 计算成功, false 失败 (通常是未校准)
     */
    bool readOxygenPercentage(float &oxygen_percent);

    /**
     * @brief 获取最后一次的错误代码
     */
    AO08_Error getLastError() const;

    /**
     * @brief 检查是否已校准
     */
    bool isCalibrated() const;

    /**
     * @brief 获取当前校准参数
     */
    void getCalibrationParams(float &voltageZero, float &voltageAir) const;

    /**
     * @brief 手动设置校准参数（不保存到存储）
     */
    void setCalibrationParams(float voltageZero, float voltageAir);

private:
    // 切换到 ADS1115 所在的 Mux 通道
    void selectMuxChannel();

    // ADS1115 I2C 帮助函数
    bool writeRegister(uint8_t reg, uint16_t value);
    uint16_t readRegister(uint8_t reg);
    
    // 获取 ADC 原始读数
    int16_t readConversionResult();
    
    // 将 ADC 原始值根据当前增益转换为毫伏
    float adsValToMillivolts(int16_t ads_val);

    I2CMux* _mux;
    uint8_t _muxChannel;
    uint8_t _adsAddress;
    AO08_Error _lastError;

    // 校准值
    float _voltageZero; // 0% O2 时的电压 (mV)
    float _voltageAir;  // 20.9% O2 时的电压 (mV)
    bool _isCalibratedZero;
    bool _isCalibratedAir;

    // ADS1115 配置
    uint16_t _configWord; // 存储我们的标准配置
    float _mvPerLsb;      // 根据增益计算 LSB 代表的毫伏数
    
    // 参数存储
    AO08_CalibrationStorage _storage;
    
    // ADS1115 寄存器地址
    static const uint8_t ADS1115_REG_POINTER_CONVERT = 0x00;
    static const uint8_t ADS1115_REG_POINTER_CONFIG = 0x01;
};

#endif // AO08_SENSOR_H



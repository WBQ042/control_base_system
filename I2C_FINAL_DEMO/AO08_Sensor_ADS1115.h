#ifndef AO08_SENSOR_ADS1115_H
#define AO08_SENSOR_ADS1115_H

#include <Arduino.h>
#include <Wire.h>
#include "TCA9548A_Mux.h" // 假设 ADS1115 也在 Mux 上

/*
 * ====================================================================================
 * ADS1115 内部寄存器和配置定义
 * ====================================================================================
 */

// 7位 I2C 地址 (ADDR 引脚接地)
#define ADS1115_DEFAULT_ADDRESS       (0x48)

// 寄存器指针
#define ADS1115_REG_POINTER_CONVERT   (0x00) // 转换寄存器
#define ADS1115_REG_POINTER_CONFIG    (0x01) // 配置寄存器

/*
 * 配置寄存器 (16位)
 * 我们将使用以下配置来读取 AO-08:
 *
 * Bit 15:    OS (1) = 开始一次单次转换
 * Bit 14-12: MUX (000) = 差分 AIN0 (Vsensor+) vs AIN1 (Vsensor-)
 * Bit 11-9:  PGA (101) = +/- 0.256V (16倍增益)。
 * AO-08 在 100% O2 时信号约 60-65mV，
 * 此量程 (256mV) 提供了最佳分辨率。
 * Bit 8:     MODE (1) = 单次转换模式
 * Bit 7-5:   DR (100) = 128 SPS (一个不错的默认值)
 * Bit 4:     COMP_MODE (0) = 传统比较器 (默认)
 * Bit 3:     COMP_POL (0) = 低电平有效 (默认)
 * Bit 2:     COMP_LAT (0) = 非锁存 (默认)
 * Bit 1-0:   COMP_QUE (11) = 禁用比较器 (默认)
 *
 * 我们的配置字 (Config Word): 0b 1 000 101 1 100 0 0 0 11 = 0x8B83
 */

// 增益设置 (PGA)
#define ADS1115_PGA_0_256V      (0b101 << 9) // 16x 增益, +/- 0.256V

// 输入通道 (MUX)
#define ADS1115_MUX_DIFF_0_1    (0b000 << 12) // AIN0 (P) vs AIN1 (N)

// 模式
#define ADS1115_MODE_SINGLE     (0b1 << 8) // 单次转换

// 数据采样率
#define ADS1115_DR_128SPS       (0b100 << 5)

// 启动转换
#define ADS1115_OS_SINGLE       (0b1 << 15) // 启动转换

/**
 * @class AO08_Sensor_ADS1115
 * @brief 使用 ADS1115 ADC 读取 AO-08 氧气传感器的库
 */
class AO08_Sensor_ADS1115 {
public:
    enum AO08_Error {
        OK = 0,
        ERROR_I2C,              // I2C 通信失败
        ERROR_TIMEOUT,          // ADC 转换超时
        ERROR_NOT_CALIBRATED,   // 传感器未校准
        ERROR_CALIBRATION_FAILED // 校准失败 (例如空气电压低于零点电压)
    };

    /**
     * @brief 构造函数
     * @param mux_ptr 指向 TCA9548A 多路复用器的指针
     * @param muxChannel ADS1115 连接到的 Mux 通道号
     * @param adsAddress ADS1115 的 I2C 地址 (默认为 0x48)
     */
    AO08_Sensor_ADS1115(TCA9548A_Mux* mux_ptr, uint8_t muxChannel, uint8_t adsAddress = ADS1115_DEFAULT_ADDRESS);

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
     * @brief (重要) 执行零点校准
     * @note 执行此操作前，必须将 Vsensor+ 和 Vsensor- 短接，
     * 或者(更优)将传感器置于 0% 氧气环境 (如纯氮气) 中。
     * @return true 校准成功
     */
    bool calibrateZero();

    /**
     * @brief (重要) 执行空气点校准 (20.9% O2)
     * @note 执行此操作前，必须将传感器暴露在新鲜空气中。
     * @return true 校准成功
     */
    bool calibrateAir();

    /**
     * @brief 读取计算后的氧气百分比
     * @note 必须先执行 calibrateZero() 和 calibrateAir()
     * @param oxygen_percent 输出氧气百分比 (0-100%)
     * @return true 计算成功, false 失败 (通常是未校准)
     */
    bool readOxygenPercentage(float &oxygen_percent);

    /**
     * @brief 获取最后一次的错误代码
     */
    AO08_Error getLastError() const;

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

    TCA9548A_Mux* _mux;
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
};

#endif // AO08_SENSOR_ADS1115_H

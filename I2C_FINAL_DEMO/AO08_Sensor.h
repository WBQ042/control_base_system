#ifndef AO08_SENSOR_H
#define AO08_SENSOR_H

#include "AO08_CalibrationStorage.h"
#include "TCA9548A_Mux.h"
#include <Arduino.h>
#include <Wire.h>


/**
 * @class AO08_Sensor
 * @brief 驱动 AO-08 氧气传感器 (通过 ADS1115 ADC 读取)
 */
class AO08_Sensor {
public:
  /**
   * @brief 错误代码枚举
   */
  enum AO08_Error {
    OK = 0,
    ERROR_I2C = 1,                // I2C 通信失败
    ERROR_TIMEOUT = 2,            // ADC 转换超时
    ERROR_NOT_CALIBRATED = 3,     // 传感器未校准
    ERROR_CALIBRATION_FAILED = 4, // 校准过程失败
    ERROR_INVALID_PARAM = 5       // 参数无效
  };

  /**
   * @brief 构造函数
   * @param mux_ptr 指向 I2C Mux 对象的指针
   * @param muxChannel ADS1115 连接的 Mux 通道 (0-7)
   * @param adsAddress ADS1115 的 I2C 地址 (默认 0x48)
   */
  AO08_Sensor(TCA9548A_Mux *mux_ptr, uint8_t muxChannel,
              uint8_t adsAddress = 0x48);

  /**
   * @brief 初始化传感器
   * @return true 成功, false 失败
   */
  bool begin();

  /**
   * @brief 读取当前氧气百分比
   * @param percent 输出参数: 氧气浓度 (%)
   * @return true 读取成功, false 读取失败
   */
  bool readOxygenPercentage(float &percent);

  /**
   * @brief 读取传感器原始电压 (mV)
   * @param voltage 输出参数: 传感器电压 (mV)
   * @return true 读取成功, false 读取失败
   */
  bool readVoltage(float &voltage);

  /**
   * @brief 执行零点校准 (0% O2)
   * @param force 如果为 true, 即使读数异常也强制保存 (慎用)
   * @return true 校准成功并保存, false 失败
   */
  bool calibrateZero(bool force = false);

  /**
   * @brief 执行空气点校准 (20.9% O2)
   * @param force 如果为 true, 即使读数异常也强制保存 (慎用)
   * @return true 校准成功并保存, false 失败
   */
  bool calibrateAir(bool force = false);

  /**
   * @brief 检查是否已校准
   * @return true 已校准, false 未校准
   */
  bool isCalibrated() const;

  /**
   * @brief 获取最后一次错误代码
   * @return 错误代码
   */
  AO08_Error getLastError() const;

  /**
   * @brief 获取当前校准参数 (用于调试)
   * @param vZero 输出: 零点电压
   * @param vAir  输出: 空气电压
   */
  void getCalibrationParams(float &vZero, float &vAir) const;

  /**
   * @brief 手动设置校准参数 (调试用)
   */
  void setCalibrationParams(float vZero, float vAir);

  // 设置是否启用调试输出
  void setDebug(bool debug) { _debug = debug; }

private:
  TCA9548A_Mux *_mux;
  uint8_t _muxChannel;
  uint8_t _adsAddress;
  AO08_Error _lastError;
  bool _debug = false;

  // ADS1115 配置相关
  uint16_t _configWord;
  float _mvPerLsb;

  // 校准参数 (内存缓存)
  float _voltageZero;
  float _voltageAir;
  bool _isCalibratedZero;
  bool _isCalibratedAir;

  // 存储管理对象
  AO08_CalibrationStorage _storage;

  // 内部辅助函数
  void selectMuxChannel();
  bool writeRegister(uint8_t reg, uint16_t value);
  uint16_t readRegister(uint8_t reg);
  int16_t readConversionResult();
  float adsValToMillivolts(int16_t ads_val);
  bool loadCalibrationFromStorage();

  // ADS1115 寄存器
  static const uint8_t ADS1115_REG_POINTER_CONVERT = 0x00;
  static const uint8_t ADS1115_REG_POINTER_CONFIG = 0x01;
};

#endif // AO08_SENSOR_H

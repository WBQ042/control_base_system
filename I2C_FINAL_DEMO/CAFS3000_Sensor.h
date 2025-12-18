#ifndef CAFS3000_SENSOR_H
#define CAFS3000_SENSOR_H

#include <Arduino.h>

/**
 * @class CAFS3000_Sensor
 * @brief 驱动 CAFS3000 气体质量流量计 (模拟电压模式)
 *
 * 修改说明:
 * 1. 移除了 I2C/Mux 依赖，改为直接读取模拟引脚 (Analog Read)
 * 2. 实现了零点预校正功能
 * 3. 实现了基于电压的流量换算: Flow = ((V_meas - V_zero) / 4.0) * Range
 */
class CAFS3000_Sensor {
public:
  /**
   * @brief 构造函数
   * @param analogPin 连接传感器的模拟引脚编号 (例如 A0)
   * @param max_range_l_min 满量程流量 (单位: L/min，默认 100)
   * @param refVoltage ADC 参考电压 (ESP32通常为 3.3V, 5V板子为 5.0V)
   * @param adcResolution ADC 分辨率 (ESP32默认12位=4095, Arduino默认10位=1023)
   */
  CAFS3000_Sensor(uint8_t analogPin, float max_range_l_min = 100.0f,
                  float refVoltage = 3.3f, int adcResolution = 4095);

  /**
   * @brief 初始化传感器 (配置引脚)
   * @return true 总是返回 true
   */
  bool begin();

  /**
   * @brief 执行零点校正 (读取多次取平均作为新的零点电压)
   * @note 请确保此时管道内无气流通过
   * @param samples 采样次数 (默认 20 次)
   */
  void calibrateZero(int samples = 20);

  /**
   * @brief 读取流量数据
   * @param flow_l_min 输出流量 (L/min)
   * @param useCalibration 是否应用校正数据 (默认 true)
   * @return true 读取成功
   */
  bool readData(float &flow_l_min, bool useCalibration = true);

  /**
   * @brief 获取当前设置的零点电压
   */
  float getZeroOffset() const;

  /**
   * @brief 手动设置零点电压
   */
  void setZeroOffset(float voltage);

private:
  uint8_t _analogPin;
  float _max_range;   // 满量程 (L/min)
  float _refVoltage;  // 参考电压 (V)
  int _adcResolution; // ADC 分辨率值 (例如 4095)

  float
      _zeroOffset; // 零点电压 (V), 默认为 1.0V (1-5V输出标准) 或 0.5V,
                   // 这里的默认值会在构造时设为 1.0 但用户逻辑中采用了动态校准

  float _spanVoltage; // 跨度电压 (V), 用户公式中为 4.0V
};

#endif // CAFS3000_SENSOR_H

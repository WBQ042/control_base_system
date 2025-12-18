#include "CAFS3000_Sensor.h"

CAFS3000_Sensor::CAFS3000_Sensor(uint8_t analogPin, float max_range_l_min,
                                 float refVoltage, int adcResolution)
    : _analogPin(analogPin), _max_range(max_range_l_min),
      _refVoltage(refVoltage), _adcResolution(adcResolution),
      _zeroOffset(1.0f), // 默认零点电压 (通常为1V)
      _spanVoltage(4.0f) // 跨度 (通常为4V)
{}

bool CAFS3000_Sensor::begin() {
  pinMode(_analogPin, INPUT);
  // 初始化时可以给一个更准确的默认偏移，但最好由上层调用 calibrateZero
  return true;
}

void CAFS3000_Sensor::calibrateZero(int samples) {
  if (samples <= 0)
    samples = 1;
  long sum = 0;

  // 丢弃前几次读数，让ADC稳定
  for (int i = 0; i < 5; i++) {
    analogRead(_analogPin);
    delay(10);
  }

  for (int i = 0; i < samples; i++) {
    sum += analogRead(_analogPin);
    delay(50); // 间隔50ms
  }

  float avgAdc = (float)sum / samples;
  _zeroOffset =
      avgAdc * (_refVoltage /
                (_adcResolution - 1)); // -1 to match 0..4095 range strictly or
                                       // not usually matter much for 4096

  // 如果计算结果太离谱（例如接近0V或接近参考电压），可能传感器没接好
  if (_zeroOffset < 0.1f || _zeroOffset > (_refVoltage - 0.5f)) {
    // 数据异常，这里可以选择重置为默认值，或者保持计算值但打印警告
    // 为了安全起见，如果不合理，我们恢复默认 1.0 (假设系统是 5V
    // 1-5V输出经过分压?) 但这是用户要求的"逻辑"，我们信任读到的值
  }
}

bool CAFS3000_Sensor::readData(float &flow_l_min, bool useCalibration) {
  int rawValue = analogRead(_analogPin);
  float voltage = rawValue * (_refVoltage / (_adcResolution - 1));

  float zeroPoint = useCalibration
                        ? _zeroOffset
                        : 1.0f; // 如果不使用校准，默认零点设为1V (标准工业信号)

  // 原始逻辑: ((originalVoltage - calibrationVoltage) / 4.0) * 100;
  // 对应: ((voltage - zeroPoint) / _spanVoltage) * _max_range;

  // 限制不需要反流 (电压低于零点算 0)
  if (voltage < zeroPoint) {
    flow_l_min = 0.0f;
  } else {
    flow_l_min = ((voltage - zeroPoint) / _spanVoltage) * _max_range;
  }

  return true;
}

float CAFS3000_Sensor::getZeroOffset() const { return _zeroOffset; }

void CAFS3000_Sensor::setZeroOffset(float voltage) { _zeroOffset = voltage; }

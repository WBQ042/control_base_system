#include "AO08_Sensor_ADS1115.h"
#include <Arduino.h>

AO08_Sensor_ADS1115::AO08_Sensor_ADS1115(TCA9548A_Mux* mux_ptr, uint8_t muxChannel, uint8_t adsAddress) {
    _mux = mux_ptr;
    _muxChannel = muxChannel;
    _adsAddress = adsAddress;
    _lastError = OK;
    
    _voltageZero = 0.0f;
    _voltageAir = 0.0f;
    _isCalibratedZero = false;
    _isCalibratedAir = false;

    // 计算配置字 (参见 .h 文件中的注释)
    _configWord = ADS1115_OS_SINGLE |     // 启动单次
                  ADS1115_MUX_DIFF_0_1 |  // AIN0 vs AIN1
                  ADS1115_PGA_0_256V |    // 16x 增益
                  ADS1115_MODE_SINGLE |   // 单次模式
                  ADS1115_DR_128SPS |     // 128 SPS
                  0x0003;                 // 禁用比较器

    // 计算 LSB (Least Significant Bit) 代表的毫伏数
    // 量程 = 0.256V = 256mV
    // 16位 ADC (15位 + 符号位), 范围 -32768 到 32767
    // mV/LSB = 256.0 (mV) / 32768.0 (steps)
    _mvPerLsb = 256.0f / 32768.0f;
}

void AO08_Sensor_ADS1115::selectMuxChannel() {
    if (_mux) {
        _mux->selectChannel(_muxChannel);
    }
}

bool AO08_Sensor_ADS1115::begin() {
    selectMuxChannel();
    Wire.beginTransmission(_adsAddress);
    // 检查 I2C 设备是否存在
    if (Wire.endTransmission() == 0) {
        Serial.print("ADS1115 (AO-08) (Channel ");
        Serial.print(_muxChannel);
        Serial.println(") 初始化成功。");
        return true;
    } else {
        Serial.print("[AO08_ADS1115 Error] ADS1115 (Channel ");
        Serial.print(_muxChannel);
        Serial.println(") 未响应。");
        _lastError = ERROR_I2C;
        return false;
    }
}

AO08_Sensor_ADS1115::AO08_Error AO08_Sensor_ADS1115::getLastError() const {
    return _lastError;
}

// 写入16位到 ADS1115 寄存器
bool AO08_Sensor_ADS1115::writeRegister(uint8_t reg, uint16_t value) {
    selectMuxChannel();
    Wire.beginTransmission(_adsAddress);
    Wire.write(reg);
    Wire.write((uint8_t)(value >> 8)); // MSB
    Wire.write((uint8_t)(value & 0xFF)); // LSB
    if (Wire.endTransmission() != 0) {
        _lastError = ERROR_I2C;
        return false;
    }
    return true;
}

// 从 ADS1115 寄存器读取16位
uint16_t AO08_Sensor_ADS1115::readRegister(uint8_t reg) {
    selectMuxChannel();
    Wire.beginTransmission(_adsAddress);
    Wire.write(reg);
    Wire.endTransmission(); // 发送寄存器指针

    if (Wire.requestFrom(_adsAddress, (uint8_t)2) != 2) {
        _lastError = ERROR_I2C;
        return 0; // 读取失败
    }
    uint16_t value = (Wire.read() << 8) | Wire.read(); // MSB, LSB
    return value;
}

// 核心函数：启动一次转换并读取结果
int16_t AO08_Sensor_ADS1115::readConversionResult() {
    _lastError = OK;
    // 1. 写入配置，启动一次转换
    if (!writeRegister(ADS1115_REG_POINTER_CONFIG, _configWord)) {
        // 错误已在 writeRegister 中设置
        return 0;
    }

    // 2. 等待转换完成
    // 128 SPS = 7.8ms 转换时间。我们给 10ms 延迟。
    // 一个更鲁棒的方法是轮询配置寄存器的 OS 位。
    unsigned long startTime = millis();
    bool conversionDone = false;
    
    while (millis() - startTime < 100) { // 100ms 超时
        uint16_t config = readRegister(ADS1115_REG_POINTER_CONFIG);
        if (_lastError != OK) return 0; // I2C 读取失败

        // 检查 OS (Bit 15) 是否为 1 (表示转换完成)
        if (config & 0x8000) {
            conversionDone = true;
            break;
        }
        delay(1); // 稍微等待
    }

    if (!conversionDone) {
        Serial.println("[AO08_ADS1115 Error] ADC 转换超时。");
        _lastError = ERROR_TIMEOUT;
        return 0;
    }

    // 3. 从转换寄存器读取结果
    return (int16_t)readRegister(ADS1115_REG_POINTER_CONVERT);
}

// 将 ADC 原始值转换为毫伏
float AO08_Sensor_ADS1115::adsValToMillivolts(int16_t ads_val) {
    return (float)ads_val * _mvPerLsb;
}

// 读取电压 (mV)
bool AO08_Sensor_ADS1115::readVoltage(float &voltage_mV) {
    int16_t raw_adc = readConversionResult();
    if (_lastError != OK) {
        return false; // 错误 (I2C 或超时)
    }
    
    voltage_mV = adsValToMillivolts(raw_adc);
    return true;
}

// 校准零点
bool AO08_Sensor_ADS1115::calibrateZero() {
    Serial.println("[AO08 Calibrate] 正在校准零点 (0% O2)...");
    Serial.println("...请确保传感器引脚已短接，或置于纯氮气中。");
    delay(2000); // 给用户反应时间

    if (!readVoltage(_voltageZero)) {
        Serial.println("[AO08 Calibrate] 零点校准失败 (无法读取ADC)。");
        _lastError = ERROR_CALIBRATION_FAILED;
        return false;
    }
    
    _isCalibratedZero = true;
    Serial.print("[AO08 Calibrate] 零点电压 (V_zero) 设置为: ");
    Serial.print(_voltageZero, 4);
    Serial.println(" mV");
    return true;
}

// 校准空气点
bool AO08_Sensor_ADS1115::calibrateAir() {
    Serial.println("[AO08 Calibrate] 正在校准空气点 (20.9% O2)...");
    Serial.println("...请确保传感器已充分暴露于新鲜空气中。");
    delay(5000); // 传感器响应需要时间 (T90 < 15s)，等待5秒

    if (!readVoltage(_voltageAir)) {
        Serial.println("[AO08 Calibrate] 空气点校准失败 (无法读取ADC)。");
        _lastError = ERROR_CALIBRATION_FAILED;
        return false;
    }

    _isCalibratedAir = true;
    Serial.print("[AO08 Calibrate] 空气点电压 (V_air) 设置为: ");
    Serial.print(_voltageAir, 4);
    Serial.println(" mV");

    // 检查校准是否合理 (空气电压必须高于零点电压)
    if (_isCalibratedZero && (_voltageAir <= _voltageZero)) {
        Serial.println("[AO08 Calibrate] 警告: 空气电压低于或等于零点电压！");
        _lastError = ERROR_CALIBRATION_FAILED;
        _isCalibratedAir = false; // 标记为失败
        return false;
    }
    return true;
}

// 读取氧气百分比
bool AO08_Sensor_ADS1115::readOxygenPercentage(float &oxygen_percent) {
    // 1. 检查是否已校准
    if (!_isCalibratedZero || !_isCalibratedAir) {
        if (_lastError == OK) { // 避免覆盖更具体的错误
            _lastError = ERROR_NOT_CALIBRATED;
        }
        return false;
    }
    
    // 2. 检查校准值是否有效
    float deltaV = _voltageAir - _voltageZero;
    if (deltaV <= 0.0f) {
        _lastError = ERROR_CALIBRATION_FAILED;
        return false; 
    }

    // 3. 读取当前电压
    float currentVoltage;
    if (!readVoltage(currentVoltage)) {
        // 错误已在 readVoltage 中设置
        return false;
    }

    // 4. 应用 AO-08 手册中的校准公式 (P4)
    // O2 % = ((Ax - A0) * 20.9) / (A1 - A0)
    // Ax = currentVoltage
    // A0 = _voltageZero
    // A1 = _voltageAir
    
    oxygen_percent = ((currentVoltage - _voltageZero) * 20.9f) / deltaV;

    // 限制范围
    if (oxygen_percent < 0.0f) {
        oxygen_percent = 0.0f;
    }
    
    _lastError = OK;
    return true;
}

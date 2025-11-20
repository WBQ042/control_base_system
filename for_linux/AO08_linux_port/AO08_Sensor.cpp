#include "AO08_Sensor.h"
using namespace ArduinoHAL;

AO08_Sensor::AO08_Sensor(I2CMux* mux_ptr, uint8_t muxChannel, uint8_t adsAddress)
    : _mux(mux_ptr), _muxChannel(muxChannel), _adsAddress(adsAddress),
      _lastError(OK), _voltageZero(0.0f), _voltageAir(0.0f),
      _isCalibratedZero(false), _isCalibratedAir(false) {
    
    // 计算配置字
    // Bit 15: OS (1) = 启动单次转换
    // Bit 14-12: MUX (000) = 差分 AIN0 vs AIN1
    // Bit 11-9: PGA (101) = +/- 0.256V (16倍增益)
    // Bit 8: MODE (1) = 单次转换模式
    // Bit 7-5: DR (100) = 128 SPS
    // Bit 1-0: COMP_QUE (11) = 禁用比较器
    _configWord = 0x8B83;
    
    // 计算 LSB 代表的毫伏数
    // 量程 = 0.256V = 256mV
    // 16位 ADC (15位 + 符号位), 范围 -32768 到 32767
    // mV/LSB = 256.0 (mV) / 32768.0 (steps)
    _mvPerLsb = 256.0f / 32768.0f;
}

void AO08_Sensor::selectMuxChannel() {
    if (_mux) {
        _mux->selectChannel(_muxChannel);
    }
}

bool AO08_Sensor::begin() {
    // 初始化参数存储
    if (!_storage.begin()) {
        Serial.println("[AO08] 警告: 参数存储初始化失败，将无法保存校准参数");
    }
    
    selectMuxChannel();
    Wire.beginTransmission(_adsAddress);
    // 检查 I2C 设备是否存在
    if (Wire.endTransmission() == 0) {
        Serial.print("[AO08] ADS1115 (通道 ");
        Serial.print(_muxChannel);
        Serial.println(") 初始化成功");
        
        // 尝试从存储加载校准参数
        if (loadCalibrationFromStorage()) {
            Serial.println("[AO08] 已从存储加载校准参数");
        } else {
            Serial.println("[AO08] 未找到已保存的校准参数，需要重新校准");
        }
        
        return true;
    } else {
        Serial.print("[AO08] 错误: ADS1115 (通道 ");
        Serial.print(_muxChannel);
        Serial.println(") 未响应");
        _lastError = ERROR_I2C;
        return false;
    }
}

AO08_Sensor::AO08_Error AO08_Sensor::getLastError() const {
    return _lastError;
}

bool AO08_Sensor::isCalibrated() const {
    return _isCalibratedZero && _isCalibratedAir;
}

void AO08_Sensor::getCalibrationParams(float &voltageZero, float &voltageAir) const {
    voltageZero = _voltageZero;
    voltageAir = _voltageAir;
}

void AO08_Sensor::setCalibrationParams(float voltageZero, float voltageAir) {
    _voltageZero = voltageZero;
    _voltageAir = voltageAir;
    _isCalibratedZero = true;
    _isCalibratedAir = true;
}

// 写入16位到 ADS1115 寄存器
bool AO08_Sensor::writeRegister(uint8_t reg, uint16_t value) {
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
uint16_t AO08_Sensor::readRegister(uint8_t reg) {
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
int16_t AO08_Sensor::readConversionResult() {
    _lastError = OK;
    // 1. 写入配置，启动一次转换
    if (!writeRegister(ADS1115_REG_POINTER_CONFIG, _configWord)) {
        return 0;
    }

    // 2. 等待转换完成
    unsigned long startTime = millis();
    bool conversionDone = false;
    
    while (millis() - startTime < 100) { // 100ms 超时
        uint16_t config = readRegister(ADS1115_REG_POINTER_CONFIG);
        if (_lastError != OK) return 0;

        // 检查 OS (Bit 15) 是否为 1 (表示转换完成)
        if (config & 0x8000) {
            conversionDone = true;
            break;
        }
        delay(1);
    }

    if (!conversionDone) {
        Serial.println("[AO08] 错误: ADC 转换超时");
        _lastError = ERROR_TIMEOUT;
        return 0;
    }

    // 3. 从转换寄存器读取结果
    return (int16_t)readRegister(ADS1115_REG_POINTER_CONVERT);
}

// 将 ADC 原始值转换为毫伏
float AO08_Sensor::adsValToMillivolts(int16_t ads_val) {
    return (float)ads_val * _mvPerLsb;
}

// 读取电压 (mV)
bool AO08_Sensor::readVoltage(float &voltage_mV) {
    int16_t raw_adc = readConversionResult();
    if (_lastError != OK) {
        return false;
    }
    
    voltage_mV = adsValToMillivolts(raw_adc);
    return true;
}

// 校准零点
bool AO08_Sensor::calibrateZero(bool saveToStorage) {
    Serial.println("\n=== AO08 零点校准 ===");
    Serial.println("请确保传感器引脚已短接，或置于纯氮气中");
    Serial.println("等待 2 秒后开始测量...");
    delay(2000);

    // 读取多次并取平均值
    const int samples = 10;
    float sum = 0.0f;
    int validSamples = 0;
    
    for (int i = 0; i < samples; i++) {
        float voltage;
        if (readVoltage(voltage)) {
            sum += voltage;
            validSamples++;
        }
        delay(100);
    }
    
    if (validSamples == 0) {
        Serial.println("[AO08] 错误: 零点校准失败 (无法读取ADC)");
        _lastError = ERROR_CALIBRATION_FAILED;
        return false;
    }
    
    _voltageZero = sum / validSamples;
    _isCalibratedZero = true;
    
    Serial.print("[AO08] 零点电压 (V_zero) 设置为: ");
    Serial.print(_voltageZero, 4);
    Serial.println(" mV");
    
    // 保存到存储
    if (saveToStorage && _isCalibratedAir) {
        // 如果空气点也已校准，保存完整参数
        AO08_CalibrationStorage::CalibrationParams params;
        params.voltageZero = _voltageZero;
        params.voltageAir = _voltageAir;
        params.isValid = true;
        _storage.saveCalibration(params);
    }
    
    Serial.println("=== 零点校准完成 ===\n");
    return true;
}

// 校准空气点
bool AO08_Sensor::calibrateAir(bool saveToStorage) {
    Serial.println("\n=== AO08 空气点校准 ===");
    Serial.println("请确保传感器已充分暴露于新鲜空气中");
    Serial.println("等待 30 秒让传感器稳定...");
    
    // 显示倒计时
    for (int i = 30; i > 0; i--) {
        Serial.print("等待中: ");
        Serial.print(i);
        Serial.println(" 秒");
        delay(1000);
    }
    
    // 读取多次并取平均值
    const int samples = 10;
    float sum = 0.0f;
    int validSamples = 0;
    
    for (int i = 0; i < samples; i++) {
        float voltage;
        if (readVoltage(voltage)) {
            sum += voltage;
            validSamples++;
        }
        delay(100);
    }
    
    if (validSamples == 0) {
        Serial.println("[AO08] 错误: 空气点校准失败 (无法读取ADC)");
        _lastError = ERROR_CALIBRATION_FAILED;
        return false;
    }
    
    _voltageAir = sum / validSamples;
    _isCalibratedAir = true;
    
    Serial.print("[AO08] 空气点电压 (V_air) 设置为: ");
    Serial.print(_voltageAir, 4);
    Serial.println(" mV");

    // 检查校准是否合理
    if (_isCalibratedZero && (_voltageAir <= _voltageZero)) {
        Serial.println("[AO08] 错误: 空气电压必须大于零点电压！");
        _lastError = ERROR_CALIBRATION_FAILED;
        _isCalibratedAir = false;
        return false;
    }
    
    // 保存到存储
    if (saveToStorage && _isCalibratedZero) {
        // 如果零点也已校准，保存完整参数
        AO08_CalibrationStorage::CalibrationParams params;
        params.voltageZero = _voltageZero;
        params.voltageAir = _voltageAir;
        params.isValid = true;
        _storage.saveCalibration(params);
        Serial.println("[AO08] 校准参数已保存到非易失性存储");
    }
    
    Serial.println("=== 空气点校准完成 ===\n");
    return true;
}

// 从存储加载校准参数
bool AO08_Sensor::loadCalibrationFromStorage() {
    AO08_CalibrationStorage::CalibrationParams params;
    if (_storage.loadCalibration(params)) {
        _voltageZero = params.voltageZero;
        _voltageAir = params.voltageAir;
        _isCalibratedZero = true;
        _isCalibratedAir = true;
        return true;
    }
    return false;
}

// 读取氧气百分比
bool AO08_Sensor::readOxygenPercentage(float &oxygen_percent) {
    // 1. 检查是否已校准
    if (!_isCalibratedZero || !_isCalibratedAir) {
        if (_lastError == OK) {
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
        return false;
    }

    // 4. 应用 AO-08 手册中的校准公式
    // O2 % = ((Ax - A0) * 20.9) / (A1 - A0)
    oxygen_percent = ((currentVoltage - _voltageZero) * 20.9f) / deltaV;

    // 限制范围
    if (oxygen_percent < 0.0f) {
        oxygen_percent = 0.0f;
    }
    
    _lastError = OK;
    return true;
}



#include "AO08_CalibrationStorage.h"

// Preferences 键名定义
const char* AO08_CalibrationStorage::KEY_VOLTAGE_ZERO = "v_zero";
const char* AO08_CalibrationStorage::KEY_VOLTAGE_AIR = "v_air";
const char* AO08_CalibrationStorage::KEY_IS_VALID = "is_valid";

AO08_CalibrationStorage::AO08_CalibrationStorage(const char* namespaceName)
    : _namespace(namespaceName) {
}

bool AO08_CalibrationStorage::begin() {
    if (!_prefs.begin(_namespace, false)) {
        Serial.print("[存储错误] 无法打开命名空间: ");
        Serial.println(_namespace);
        return false;
    }
    
    Serial.print("[存储] 初始化成功，命名空间: ");
    Serial.println(_namespace);
    return true;
}

bool AO08_CalibrationStorage::validateParams(const CalibrationParams& params) {
    // 验证参数合理性
    if (params.voltageAir <= params.voltageZero) {
        Serial.println("[存储错误] 空气电压必须大于零点电压");
        return false;
    }
    
    // 检查电压范围是否合理（AO-08 在空气中约为 60-65mV）
    if (params.voltageAir < 10.0f || params.voltageAir > 200.0f) {
        Serial.println("[存储警告] 空气电压超出正常范围 (10-200mV)");
        // 不返回 false，只警告
    }
    
    if (abs(params.voltageZero) > 50.0f) {
        Serial.println("[存储警告] 零点电压超出正常范围 (-50 到 50mV)");
    }
    
    return true;
}

bool AO08_CalibrationStorage::saveCalibration(const CalibrationParams& params) {
    if (!validateParams(params)) {
        return false;
    }
    
    if (!_prefs.begin(_namespace, false)) {
        Serial.println("[存储错误] 无法打开命名空间进行写入");
        return false;
    }
    
    // 保存参数
    _prefs.putFloat(KEY_VOLTAGE_ZERO, params.voltageZero);
    _prefs.putFloat(KEY_VOLTAGE_AIR, params.voltageAir);
    _prefs.putBool(KEY_IS_VALID, true);
    
    _prefs.end();
    
    Serial.println("[存储] 校准参数已保存:");
    Serial.print("  零点电压: ");
    Serial.print(params.voltageZero, 4);
    Serial.println(" mV");
    Serial.print("  空气电压: ");
    Serial.print(params.voltageAir, 4);
    Serial.println(" mV");
    
    return true;
}

bool AO08_CalibrationStorage::loadCalibration(CalibrationParams& params) {
    if (!_prefs.begin(_namespace, true)) { // 只读模式
        Serial.println("[存储错误] 无法打开命名空间进行读取");
        return false;
    }
    
    // 检查是否存在有效校准
    if (!_prefs.getBool(KEY_IS_VALID, false)) {
        _prefs.end();
        Serial.println("[存储] 未找到有效的校准参数");
        return false;
    }
    
    // 读取参数
    params.voltageZero = _prefs.getFloat(KEY_VOLTAGE_ZERO, 0.0f);
    params.voltageAir = _prefs.getFloat(KEY_VOLTAGE_AIR, 0.0f);
    params.isValid = true;
    
    _prefs.end();
    
    // 验证读取的参数
    if (!validateParams(params)) {
        params.isValid = false;
        return false;
    }
    
    Serial.println("[存储] 校准参数已加载:");
    Serial.print("  零点电压: ");
    Serial.print(params.voltageZero, 4);
    Serial.println(" mV");
    Serial.print("  空气电压: ");
    Serial.print(params.voltageAir, 4);
    Serial.println(" mV");
    
    return true;
}

bool AO08_CalibrationStorage::hasCalibration() {
    if (!_prefs.begin(_namespace, true)) {
        return false;
    }
    
    bool hasValid = _prefs.getBool(KEY_IS_VALID, false);
    _prefs.end();
    
    return hasValid;
}

bool AO08_CalibrationStorage::clearCalibration() {
    if (!_prefs.begin(_namespace, false)) {
        return false;
    }
    
    _prefs.remove(KEY_VOLTAGE_ZERO);
    _prefs.remove(KEY_VOLTAGE_AIR);
    _prefs.remove(KEY_IS_VALID);
    
    _prefs.end();
    
    Serial.println("[存储] 校准参数已清除");
    return true;
}

void AO08_CalibrationStorage::printCalibration() {
    CalibrationParams params;
    if (loadCalibration(params)) {
        Serial.println("=== 当前校准参数 ===");
        Serial.print("零点电压 (V_zero): ");
        Serial.print(params.voltageZero, 4);
        Serial.println(" mV");
        Serial.print("空气电压 (V_air): ");
        Serial.print(params.voltageAir, 4);
        Serial.println(" mV");
        Serial.println("===================");
    } else {
        Serial.println("未找到有效的校准参数");
    }
}



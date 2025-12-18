#include "AO08_CalibrationStorage.h"

// Preferences 键名定义
const char *AO08_CalibrationStorage::KEY_VOLTAGE_ZERO = "v_zero";
const char *AO08_CalibrationStorage::KEY_VOLTAGE_AIR = "v_air";
const char *AO08_CalibrationStorage::KEY_IS_VALID = "is_valid";

AO08_CalibrationStorage::AO08_CalibrationStorage(const char *namespaceName)
    : _namespace(namespaceName) {}

// 修改 1: begin 只做测试，不保持打开状态
bool AO08_CalibrationStorage::begin() {
  // 尝试打开一下以确保分区正常
  if (!_prefs.begin(_namespace, true)) { // 只读模式测试
    // 如果只读失败，尝试读写（针对某些特殊情况）
    if (!_prefs.begin(_namespace, false)) {
      Serial.print("[存储错误] 无法打开命名空间: ");
      Serial.println(_namespace);
      return false;
    }
  }

  // 关键点：测试完立刻关闭！
  _prefs.end();

  Serial.print("[存储] 存储系统初始化成功: ");
  Serial.println(_namespace);
  return true;
}

bool AO08_CalibrationStorage::validateParams(const CalibrationParams &params) {
  if (params.voltageAir <= params.voltageZero) {
    Serial.println("[存储错误] 空气电压必须大于零点电压");
    return false;
  }
  // ... 其他检查保持不变 ...
  return true;
}

// 修改 2: saveCalibration 内部完整的 打开 -> 写入 -> 关闭 流程
bool AO08_CalibrationStorage::saveCalibration(const CalibrationParams &params) {
  if (!validateParams(params)) {
    return false;
  }

  // 1. 打开 (读写模式)
  // 为了安全，先调用一次 end() 确保之前的状态被清除
  _prefs.end();
  if (!_prefs.begin(_namespace, false)) {
    Serial.println("[存储错误] 无法打开命名空间进行写入");
    return false;
  }

  // 2. 写入
  size_t written = 0;
  written += _prefs.putFloat(KEY_VOLTAGE_ZERO, params.voltageZero);
  written += _prefs.putFloat(KEY_VOLTAGE_AIR, params.voltageAir);
  written += _prefs.putBool(KEY_IS_VALID, true);

  // 3. 关闭 (释放资源)
  _prefs.end();

  if (written > 0) {
    Serial.println("[存储] 校准参数写入成功");
    return true;
  } else {
    Serial.println("[存储错误] 写入 NVS 失败");
    return false;
  }
}

// 修改 3: loadCalibration 内部完整的 打开 -> 读取 -> 关闭 流程
bool AO08_CalibrationStorage::loadCalibration(CalibrationParams &params) {
  // 1. 打开 (只读模式)
  _prefs.end(); // 安全清除
  if (!_prefs.begin(_namespace, true)) {
    // 如果只读打开失败，尝试读写打开
    if (!_prefs.begin(_namespace, false)) {
      Serial.println("[存储错误] 无法打开命名空间进行读取");
      return false;
    }
  }

  // 2. 读取
  if (!_prefs.isKey(KEY_IS_VALID) || !_prefs.getBool(KEY_IS_VALID, false)) {
    Serial.println("[存储] 未找到有效的校准参数");
    _prefs.end(); // 记得关闭
    return false;
  }

  params.voltageZero = _prefs.getFloat(KEY_VOLTAGE_ZERO, 0.0f);
  params.voltageAir = _prefs.getFloat(KEY_VOLTAGE_AIR, 0.0f);
  params.isValid = true;

  // 3. 关闭
  _prefs.end();

  // 4. 验证
  if (!validateParams(params)) {
    params.isValid = false;
    return false;
  }

  Serial.println("[存储] 校准参数已加载成功");
  return true;
}

// 修改 4: 其他函数也要遵循 打开-关闭 原则
bool AO08_CalibrationStorage::hasCalibration() {
  _prefs.end();
  if (!_prefs.begin(_namespace, true)) {
    return false;
  }
  bool exists = _prefs.getBool(KEY_IS_VALID, false);
  _prefs.end();
  return exists;
}

bool AO08_CalibrationStorage::clearCalibration() {
  _prefs.end();
  if (!_prefs.begin(_namespace, false)) {
    return false;
  }
  bool success = _prefs.clear();
  _prefs.end();

  if (success)
    Serial.println("[存储] 参数已清除");
  return success;
}

void AO08_CalibrationStorage::printCalibration() {
  CalibrationParams params;
  if (loadCalibration(params)) {
    Serial.println("=== 当前校准参数 (NVS) ===");
    Serial.print("零点电压 (V_zero): ");
    Serial.print(params.voltageZero, 4);
    Serial.println(" mV");
    Serial.print("空气电压 (V_air): ");
    Serial.print(params.voltageAir, 4);
    Serial.println(" mV");
    Serial.println("=========================");
  } else {
    Serial.println("未找到有效的校准参数");
  }
}

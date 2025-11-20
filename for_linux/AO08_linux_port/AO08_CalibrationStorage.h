#ifndef AO08_CALIBRATION_STORAGE_H
#define AO08_CALIBRATION_STORAGE_H

#include "LuckfoxArduino.h"

/**
 * @class AO08_CalibrationStorage
 * @brief 管理 AO08 氧气传感器校准参数的存储和读取
 * 
 * 使用 ESP32 的 Preferences 库将校准参数保存到非易失性存储中，
 * 避免每次重启都需要重新校准传感器。
 */
class AO08_CalibrationStorage {
public:
    /**
     * @brief 校准参数结构体
     */
    struct CalibrationParams {
        float voltageZero;  // 0% O2 时的电压 (mV)
        float voltageAir;   // 20.9% O2 时的电压 (mV)
        bool isValid;       // 参数是否有效
        
        CalibrationParams() : voltageZero(0.0f), voltageAir(0.0f), isValid(false) {}
    };
    
    /**
     * @brief 构造函数
     * @param namespaceName Preferences 命名空间名称（默认: "ao08_cal"）
     */
    AO08_CalibrationStorage(const char* namespaceName = "ao08_cal");
    
    /**
     * @brief 初始化存储系统
     * @return true 成功, false 失败
     */
    bool begin();
    
    /**
     * @brief 保存校准参数到非易失性存储
     * @param params 校准参数结构体
     * @return true 保存成功, false 保存失败
     */
    bool saveCalibration(const CalibrationParams& params);
    
    /**
     * @brief 从非易失性存储读取校准参数
     * @param params 输出参数，用于存储读取到的校准参数
     * @return true 读取成功且参数有效, false 读取失败或参数无效
     */
    bool loadCalibration(CalibrationParams& params);
    
    /**
     * @brief 检查是否存在已保存的校准参数
     * @return true 存在有效校准参数, false 不存在或无效
     */
    bool hasCalibration();
    
    /**
     * @brief 清除已保存的校准参数
     * @return true 清除成功, false 清除失败
     */
    bool clearCalibration();
    
    /**
     * @brief 打印当前存储的校准参数（用于调试）
     */
    void printCalibration();

private:
    ArduinoHAL::Preferences _prefs;
    const char* _namespace;
    
    // Preferences 键名
    static const char* KEY_VOLTAGE_ZERO;
    static const char* KEY_VOLTAGE_AIR;
    static const char* KEY_IS_VALID;
    
    // 验证参数有效性
    bool validateParams(const CalibrationParams& params);
};

#endif // AO08_CALIBRATION_STORAGE_H



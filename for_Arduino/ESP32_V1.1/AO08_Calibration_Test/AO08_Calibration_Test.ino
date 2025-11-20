/*
 * ===================================================================
 * AO08 氧气传感器校准与参数存储测试程序
 * 
 * 硬件: Arduino Nano ESP32
 * 
 * 功能:
 * 1. 初始化 I2C Mux (TCA9548A)
 * 2. 初始化 ADS1115 ADC 和 AO08 氧气传感器
 * 3. 自动从非易失性存储加载校准参数（如果存在）
 * 4. 如果未找到校准参数，执行校准流程
 * 5. 定期读取并显示氧气浓度
 * 6. 支持 OLED 显示（可选）
 * 
 * 依赖库:
 * - Wire (Arduino 内置)
 * - Preferences (ESP32 内置)
 * - Adafruit GFX Library (如果使用 OLED)
 * - Adafruit SSD1306 (如果使用 OLED)
 * ===================================================================
 */

#include <Wire.h>
#include "I2CMux.h"
#include "AO08_Sensor.h"
#include "AO08_CalibrationStorage.h"

// --- 硬件配置 ---

// I2C Mux 配置
const uint8_t MUX_ADDRESS = 0x70;        // TCA9548A 默认地址
const uint8_t MUX_CHANNEL_ADS1115 = 6;   // ADS1115 连接的 Mux 通道（根据实际连接修改）
const uint8_t MUX_CHANNEL_OLED = 3;      // OLED 连接的 Mux 通道（如果使用，根据实际连接修改）

// ADS1115 配置
const uint8_t ADS1115_ADDRESS = 0x4A;    // ADS1115 I2C 地址

// OLED 配置（可选）
const bool USE_OLED = false;              // 是否使用 OLED 显示
const uint8_t OLED_ADDRESS = 0x3C;       // OLED I2C 地址

// --- 全局对象 ---

I2CMux mux(MUX_ADDRESS);
AO08_Sensor oxygenSensor(&mux, MUX_CHANNEL_ADS1115, ADS1115_ADDRESS);
AO08_CalibrationStorage calibrationStorage;

// 读取间隔
const unsigned long READ_INTERVAL = 2000; // 每2秒读取一次 (ms)
unsigned long lastReadTime = 0;

// --- 辅助函数 ---

/**
 * @brief 执行氧气传感器校准流程
 */
void performCalibration() {
    Serial.println("\n========================================");
    Serial.println("    AO08 氧气传感器校准流程");
    Serial.println("========================================\n");
    
    // 零点校准
    Serial.println("【步骤 1/2: 零点校准】");
    Serial.println("请将 AO08 传感器的 Vsensor+ 和 Vsensor- 引脚短接");
    Serial.println("或者将传感器置于纯氮气环境中");
    Serial.println("完成后，请在串口监视器中输入 'z' 并按回车键...");
    
    while (Serial.available() == 0) {
        delay(100);
    }
    
    // 读取并清空输入缓冲区
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'z' || c == 'Z') {
            break;
        }
    }
    while (Serial.available()) Serial.read(); // 清空缓冲区
    
    if (oxygenSensor.calibrateZero(true)) {
        Serial.println("✓ 零点校准成功！");
    } else {
        Serial.println("✗ 零点校准失败！");
        return;
    }
    
    delay(1000);
    
    // 空气点校准
    Serial.println("\n【步骤 2/2: 空气点校准】");
    Serial.println("请移除短接线，将传感器充分暴露在新鲜空气中");
    Serial.println("完成后，请在串口监视器中输入 'a' 并按回车键...");
    
    while (Serial.available() == 0) {
        delay(100);
    }
    
    // 读取并清空输入缓冲区
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'a' || c == 'A') {
            break;
        }
    }
    while (Serial.available()) Serial.read(); // 清空缓冲区
    
    if (oxygenSensor.calibrateAir(true)) {
        Serial.println("✓ 空气点校准成功！");
        Serial.println("\n========================================");
        Serial.println("    校准完成！参数已保存到存储");
        Serial.println("========================================\n");
    } else {
        Serial.println("✗ 空气点校准失败！");
    }
}

/**
 * @brief 显示当前校准参数
 */
void printCalibrationInfo() {
    if (oxygenSensor.isCalibrated()) {
        float vZero, vAir;
        oxygenSensor.getCalibrationParams(vZero, vAir);
        Serial.println("\n=== 当前校准参数 ===");
        Serial.print("零点电压 (V_zero): ");
        Serial.print(vZero, 4);
        Serial.println(" mV");
        Serial.print("空气电压 (V_air): ");
        Serial.print(vAir, 4);
        Serial.println(" mV");
        Serial.println("===================\n");
    } else {
        Serial.println("\n警告: 传感器未校准！\n");
    }
}

/**
 * @brief 处理串口命令
 */
void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "cal" || command == "calibrate") {
            // 执行校准
            performCalibration();
        } else if (command == "info" || command == "status") {
            // 显示校准信息
            printCalibrationInfo();
            calibrationStorage.printCalibration();
        } else if (command == "clear") {
            // 清除校准参数
            Serial.println("\n清除校准参数...");
            if (calibrationStorage.clearCalibration()) {
                Serial.println("校准参数已清除");
                Serial.println("请重新校准传感器");
            }
        } else if (command == "test" || command == "voltage") {
            // 测试电压读取
            Serial.println("\n=== 测试电压读取 ===");
            float voltage;
            if (oxygenSensor.readVoltage(voltage)) {
                Serial.print("读取成功！当前电压: ");
                Serial.print(voltage, 4);
                Serial.println(" mV");
                
                // 显示原始ADC值（如果可能）
                Serial.println("传感器连接正常");
            } else {
                Serial.println("读取失败！");
                AO08_Sensor::AO08_Error error = oxygenSensor.getLastError();
                Serial.print("错误代码: ");
                Serial.println((int)error);
                switch (error) {
                    case AO08_Sensor::ERROR_I2C:
                        Serial.println("I2C 通信失败");
                        break;
                    case AO08_Sensor::ERROR_TIMEOUT:
                        Serial.println("ADC 转换超时");
                        break;
                    default:
                        Serial.println("未知错误");
                        break;
                }
            }
            Serial.println("==================\n");
        } else if (command == "help") {
            // 显示帮助信息
            Serial.println("\n=== 可用命令 ===");
            Serial.println("cal / calibrate  - 执行校准流程");
            Serial.println("info / status   - 显示校准参数");
            Serial.println("test / voltage  - 测试电压读取");
            Serial.println("clear           - 清除已保存的校准参数");
            Serial.println("help            - 显示此帮助信息");
            Serial.println("================\n");
        }
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    
    // 等待串口连接（Arduino Nano ESP32 需要）
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  AO08 氧气传感器校准测试程序");
    Serial.println("========================================\n");
    
    // 初始化 I2C
    Wire.begin();
    Serial.println("[I2C] I2C 总线已初始化");
    
    // 初始化 Mux
    mux.begin();
    mux.addChannel(MUX_CHANNEL_ADS1115, ADS1115_ADDRESS, "ADS1115");
    if (USE_OLED) {
        mux.addChannel(MUX_CHANNEL_OLED, OLED_ADDRESS, "OLED");
    }
    Serial.println("[Mux] I2C 多路复用器已初始化");
    
    // 初始化参数存储
    if (!calibrationStorage.begin()) {
        Serial.println("[存储] 警告: 参数存储初始化失败");
    }
    
    // 初始化 AO08 传感器
    Serial.println("\n[AO08] 正在初始化传感器...");
    if (!oxygenSensor.begin()) {
        Serial.println("[AO08] 错误: 传感器初始化失败！");
        Serial.println("请检查:");
        Serial.println("1. ADS1115 是否正确连接");
        Serial.println("2. I2C 地址是否正确");
        Serial.println("3. Mux 通道配置是否正确");
        while (1) {
            delay(1000);
        }
    }
    
    // 检查是否已校准
    if (oxygenSensor.isCalibrated()) {
        Serial.println("\n[AO08] ✓ 传感器已校准，使用已保存的参数");
        printCalibrationInfo();
    } else {
        Serial.println("\n[AO08] ⚠ 传感器未校准");
        Serial.println("请执行校准流程:");
        Serial.println("1. 在串口监视器中输入 'cal' 并按回车");
        Serial.println("2. 按照提示完成校准步骤\n");
    }
    
    Serial.println("\n=== 系统就绪 ===");
    Serial.println("输入 'help' 查看可用命令");
    Serial.println("输入 'cal' 执行校准");
    Serial.println("输入 'info' 查看校准参数");
    Serial.println("================\n");
    
    lastReadTime = millis();
}

// --- LOOP ---
void loop() {
    // 处理串口命令
    handleSerialCommands();
    
    // 定期读取传感器数据
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        lastReadTime = now;
        
        // 读取氧气浓度
        float oxygenPercent;
        if (oxygenSensor.readOxygenPercentage(oxygenPercent)) {
            // 读取当前电压（用于调试）
            float voltage;
            oxygenSensor.readVoltage(voltage);
            
            // 打印到串口
            Serial.println("--- 传感器读数 ---");
            Serial.print("氧气浓度: ");
            Serial.print(oxygenPercent, 2);
            Serial.println(" %");
            Serial.print("传感器电压: ");
            Serial.print(voltage, 4);
            Serial.println(" mV");
            Serial.println("------------------");
        } else {
            Serial.println("--- 传感器读数 ---");
            Serial.println("错误: 无法读取氧气浓度");
            AO08_Sensor::AO08_Error error = oxygenSensor.getLastError();
            Serial.print("错误代码: ");
            Serial.println((int)error);
            
            switch (error) {
                case AO08_Sensor::ERROR_NOT_CALIBRATED:
                    Serial.println("原因: 传感器未校准");
                    Serial.println("解决方案: 输入 'cal' 执行校准");
                    break;
                case AO08_Sensor::ERROR_I2C:
                    Serial.println("原因: I2C 通信失败");
                    Serial.println("请检查:");
                    Serial.println("  - I2C 连接是否正确");
                    Serial.println("  - ADS1115 地址是否正确 (当前: 0x4A)");
                    Serial.println("  - Mux 通道是否正确 (当前: 通道 6)");
                    break;
                case AO08_Sensor::ERROR_TIMEOUT:
                    Serial.println("原因: ADC 转换超时");
                    Serial.println("请检查 ADS1115 是否正常工作");
                    break;
                case AO08_Sensor::ERROR_CALIBRATION_FAILED:
                    Serial.println("原因: 校准参数无效");
                    Serial.println("可能的问题:");
                    Serial.println("  - 校准参数异常 (空气电压 <= 零点电压)");
                    Serial.println("  - 校准参数未正确加载");
                    Serial.println("解决方案: 输入 'cal' 重新校准");
                    // 显示当前校准参数用于调试
                    if (oxygenSensor.isCalibrated()) {
                        float vZero, vAir;
                        oxygenSensor.getCalibrationParams(vZero, vAir);
                        Serial.print("当前零点电压: ");
                        Serial.print(vZero, 4);
                        Serial.println(" mV");
                        Serial.print("当前空气电压: ");
                        Serial.print(vAir, 4);
                        Serial.println(" mV");
                    }
                    break;
                default:
                    Serial.print("原因: 未知错误 (代码: ");
                    Serial.print((int)error);
                    Serial.println(")");
                    // 尝试读取电压以获取更多信息
                    float voltage;
                    if (oxygenSensor.readVoltage(voltage)) {
                        Serial.print("当前传感器电压: ");
                        Serial.print(voltage, 4);
                        Serial.println(" mV");
                    } else {
                        Serial.println("无法读取传感器电压");
                    }
                    break;
            }
            Serial.println("------------------");
        }
    }
    
    delay(10); // 避免 CPU 占用过高
}



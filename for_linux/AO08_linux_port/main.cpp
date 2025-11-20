/*
 * ===================================================================
 * AO08 氧气传感器校准与参数存储程序 (Luckfox Linux 版本)
 * 
 * 硬件: Luckfox 嵌入式 Linux 开发板
 * 
 * 功能:
 * 1. 初始化 I2C Mux (TCA9548A)
 * 2. 初始化 ADS1115 ADC 和 AO08 氧气传感器
 * 3. 自动从非易失性存储加载校准参数（如果存在）
 * 4. 如果未找到校准参数，执行校准流程
 * 5. 定期读取并显示氧气浓度
 * 6. 支持交互式命令控制
 * ===================================================================
 */

#include "LuckfoxArduino.h"
#include "I2CMux.h"
#include "AO08_Sensor.h"
#include "AO08_CalibrationStorage.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

using namespace ArduinoHAL;

// --- 硬件配置 ---

// I2C Mux 配置
const uint8_t MUX_ADDRESS = 0x70;        // TCA9548A 默认地址
const uint8_t MUX_CHANNEL_ADS1115 = 6;   // ADS1115 连接的 Mux 通道（根据实际连接修改）

// ADS1115 配置
const uint8_t ADS1115_ADDRESS = 0x4A;    // ADS1115 I2C 地址

// --- 全局对象 ---

I2CMux mux(MUX_ADDRESS);
AO08_Sensor oxygenSensor(&mux, MUX_CHANNEL_ADS1115, ADS1115_ADDRESS);
AO08_CalibrationStorage calibrationStorage;

// 读取间隔
const unsigned long READ_INTERVAL = 2000; // 每2秒读取一次 (ms)
unsigned long lastReadTime = 0;

// --- 辅助函数 ---

/**
 * @brief 从标准输入读取用户输入（非阻塞，带超时）
 * @param prompt 提示信息
 * @param timeout_ms 超时时间（毫秒），0表示无限等待
 * @return 用户输入的字符串
 */
std::string readUserInput(const std::string& prompt, unsigned long timeout_ms = 0) {
    std::cout << prompt << std::flush;
    
    if (timeout_ms == 0) {
        // 阻塞等待输入
        std::string input;
        std::getline(std::cin, input);
        return input;
    } else {
        // TODO: 实现带超时的非阻塞输入
        // 由于在Linux终端下实现非阻塞输入比较复杂，这里简化为阻塞输入
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
}

/**
 * @brief 等待用户按键确认
 * @param expected_char 期望的字符（小写）
 */
bool waitForKey(char expected_char) {
    std::cout << "请输入 '" << expected_char << "' 并按回车键继续..." << std::endl;
    
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            char c = std::tolower(input[0]);
            if (c == expected_char) {
                return true;
            }
        }
    }
}

/**
 * @brief 执行氧气传感器校准流程
 */
void performCalibration() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    AO08 氧气传感器校准流程" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 零点校准
    std::cout << "【步骤 1/2: 零点校准】" << std::endl;
    std::cout << "请将 AO08 传感器的 Vsensor+ 和 Vsensor- 引脚短接" << std::endl;
    std::cout << "或者将传感器置于纯氮气环境中" << std::endl;
    
    waitForKey('z');
    
    if (oxygenSensor.calibrateZero(true)) {
        std::cout << "✓ 零点校准成功！" << std::endl;
    } else {
        std::cout << "✗ 零点校准失败！" << std::endl;
        return;
    }
    
    delay(1000);
    
    // 空气点校准
    std::cout << "\n【步骤 2/2: 空气点校准】" << std::endl;
    std::cout << "请移除短接线，将传感器充分暴露在新鲜空气中" << std::endl;
    
    waitForKey('a');
    
    if (oxygenSensor.calibrateAir(true)) {
        std::cout << "✓ 空气点校准成功！" << std::endl;
        std::cout << "\n========================================" << std::endl;
        std::cout << "    校准完成！参数已保存到存储" << std::endl;
        std::cout << "========================================\n" << std::endl;
    } else {
        std::cout << "✗ 空气点校准失败！" << std::endl;
    }
}

/**
 * @brief 显示当前校准参数
 */
void printCalibrationInfo() {
    if (oxygenSensor.isCalibrated()) {
        float vZero, vAir;
        oxygenSensor.getCalibrationParams(vZero, vAir);
        std::cout << "\n=== 当前校准参数 ===" << std::endl;
        std::cout << "零点电压 (V_zero): " << vZero << " mV" << std::endl;
        std::cout << "空气电压 (V_air): " << vAir << " mV" << std::endl;
        std::cout << "===================\n" << std::endl;
    } else {
        std::cout << "\n警告: 传感器未校准！\n" << std::endl;
    }
}

/**
 * @brief 测试电压读取
 */
void testVoltageReading() {
    std::cout << "\n=== 测试电压读取 ===" << std::endl;
    float voltage;
    if (oxygenSensor.readVoltage(voltage)) {
        std::cout << "读取成功！当前电压: " << voltage << " mV" << std::endl;
        std::cout << "传感器连接正常" << std::endl;
    } else {
        std::cout << "读取失败！" << std::endl;
        AO08_Sensor::AO08_Error error = oxygenSensor.getLastError();
        std::cout << "错误代码: " << (int)error << std::endl;
        switch (error) {
            case AO08_Sensor::ERROR_I2C:
                std::cout << "I2C 通信失败" << std::endl;
                break;
            case AO08_Sensor::ERROR_TIMEOUT:
                std::cout << "ADC 转换超时" << std::endl;
                break;
            default:
                std::cout << "未知错误" << std::endl;
                break;
        }
    }
    std::cout << "==================\n" << std::endl;
}

/**
 * @brief 显示帮助信息
 */
void showHelp() {
    std::cout << "\n=== 可用命令 ===" << std::endl;
    std::cout << "cal / calibrate  - 执行校准流程" << std::endl;
    std::cout << "info / status   - 显示校准参数" << std::endl;
    std::cout << "test / voltage  - 测试电压读取" << std::endl;
    std::cout << "clear           - 清除已保存的校准参数" << std::endl;
    std::cout << "help            - 显示此帮助信息" << std::endl;
    std::cout << "exit / quit     - 退出程序" << std::endl;
    std::cout << "================\n" << std::endl;
}

/**
 * @brief 处理用户命令（非阻塞检查）
 * @return true: 继续运行, false: 退出程序
 */
bool handleCommands() {
    // 在Linux下实现非阻塞输入比较复杂，这里简化为在后台持续读取
    // 实际应用中可以使用select()或单独线程来处理输入
    
    // 检查是否有输入可用（简化版本，实际会阻塞）
    std::cout << "\n> " << std::flush;
    
    std::string command;
    std::getline(std::cin, command);
    
    // 转换为小写
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    // 去除首尾空格
    command.erase(command.begin(), std::find_if(command.begin(), command.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    command.erase(std::find_if(command.rbegin(), command.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), command.end());
    
    if (command.empty()) {
        return true; // 空命令，继续
    }
    
    if (command == "cal" || command == "calibrate") {
        performCalibration();
    } else if (command == "info" || command == "status") {
        printCalibrationInfo();
        calibrationStorage.printCalibration();
    } else if (command == "clear") {
        std::cout << "\n清除校准参数..." << std::endl;
        if (calibrationStorage.clearCalibration()) {
            std::cout << "校准参数已清除" << std::endl;
            std::cout << "请重新校准传感器" << std::endl;
        }
    } else if (command == "test" || command == "voltage") {
        testVoltageReading();
    } else if (command == "exit" || command == "quit") {
        std::cout << "\n退出程序..." << std::endl;
        return false;
    } else if (command == "help") {
        showHelp();
    } else {
        std::cout << "未知命令: " << command << std::endl;
        std::cout << "输入 'help' 查看可用命令" << std::endl;
    }
    
    return true;
}

/**
 * @brief 读取并显示传感器数据
 */
void readSensorData() {
    float oxygenPercent;
    if (oxygenSensor.readOxygenPercentage(oxygenPercent)) {
        // 读取当前电压（用于调试）
        float voltage;
        oxygenSensor.readVoltage(voltage);
        
        // 打印到串口
        std::cout << "--- 传感器读数 ---" << std::endl;
        std::cout << "氧气浓度: " << oxygenPercent << " %" << std::endl;
        std::cout << "传感器电压: " << voltage << " mV" << std::endl;
        std::cout << "------------------" << std::endl;
    } else {
        std::cout << "--- 传感器读数 ---" << std::endl;
        std::cout << "错误: 无法读取氧气浓度" << std::endl;
        AO08_Sensor::AO08_Error error = oxygenSensor.getLastError();
        std::cout << "错误代码: " << (int)error << std::endl;
        
        switch (error) {
            case AO08_Sensor::ERROR_NOT_CALIBRATED:
                std::cout << "原因: 传感器未校准" << std::endl;
                std::cout << "解决方案: 输入 'cal' 执行校准" << std::endl;
                break;
            case AO08_Sensor::ERROR_I2C:
                std::cout << "原因: I2C 通信失败" << std::endl;
                std::cout << "请检查:" << std::endl;
                std::cout << "  - I2C 连接是否正确" << std::endl;
                std::cout << "  - ADS1115 地址是否正确 (当前: 0x4A)" << std::endl;
                std::cout << "  - Mux 通道是否正确 (当前: 通道 6)" << std::endl;
                break;
            case AO08_Sensor::ERROR_TIMEOUT:
                std::cout << "原因: ADC 转换超时" << std::endl;
                std::cout << "请检查 ADS1115 是否正常工作" << std::endl;
                break;
            case AO08_Sensor::ERROR_CALIBRATION_FAILED:
                std::cout << "原因: 校准参数无效" << std::endl;
                std::cout << "可能的问题:" << std::endl;
                std::cout << "  - 校准参数异常 (空气电压 <= 零点电压)" << std::endl;
                std::cout << "  - 校准参数未正确加载" << std::endl;
                std::cout << "解决方案: 输入 'cal' 重新校准" << std::endl;
                if (oxygenSensor.isCalibrated()) {
                    float vZero, vAir;
                    oxygenSensor.getCalibrationParams(vZero, vAir);
                    std::cout << "当前零点电压: " << vZero << " mV" << std::endl;
                    std::cout << "当前空气电压: " << vAir << " mV" << std::endl;
                }
                break;
            default:
                std::cout << "原因: 未知错误 (代码: " << (int)error << ")" << std::endl;
                float voltage;
                if (oxygenSensor.readVoltage(voltage)) {
                    std::cout << "当前传感器电压: " << voltage << " mV" << std::endl;
                } else {
                    std::cout << "无法读取传感器电压" << std::endl;
                }
                break;
        }
        std::cout << "------------------" << std::endl;
    }
}

// --- Arduino 兼容接口 ---

void setup() {
    Serial.begin(115200);
    
    delay(1000);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  AO08 氧气传感器校准测试程序" << std::endl;
    std::cout << "  (Luckfox Linux 版本)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Initialize I2C
    Wire.begin();
    std::cout << "[I2C] I2C bus initialized (/dev/i2c-0)" << std::endl;
    
    // Initialize Mux
    mux.begin();
    mux.addChannel(MUX_CHANNEL_ADS1115, ADS1115_ADDRESS, "ADS1115");
    std::cout << "[Mux] I2C 多路复用器已初始化" << std::endl;
    
    // 初始化参数存储
    if (!calibrationStorage.begin()) {
        std::cout << "[存储] 警告: 参数存储初始化失败" << std::endl;
    }
    
    // 初始化 AO08 传感器
    std::cout << "\n[AO08] 正在初始化传感器..." << std::endl;
    if (!oxygenSensor.begin()) {
        std::cout << "[AO08] 错误: 传感器初始化失败！" << std::endl;
        std::cout << "请检查:" << std::endl;
        std::cout << "1. ADS1115 是否正确连接" << std::endl;
        std::cout << "2. I2C 地址是否正确" << std::endl;
        std::cout << "3. Mux 通道配置是否正确" << std::endl;
        exit(1);
    }
    
    // 检查是否已校准
    if (oxygenSensor.isCalibrated()) {
        std::cout << "\n[AO08] ✓ 传感器已校准，使用已保存的参数" << std::endl;
        printCalibrationInfo();
    } else {
        std::cout << "\n[AO08] ⚠ 传感器未校准" << std::endl;
        std::cout << "请执行校准流程:" << std::endl;
        std::cout << "1. 输入 'cal' 并按回车" << std::endl;
        std::cout << "2. 按照提示完成校准步骤\n" << std::endl;
    }
    
    std::cout << "\n=== 系统就绪 ===" << std::endl;
    std::cout << "输入 'help' 查看可用命令" << std::endl;
    std::cout << "输入 'cal' 执行校准" << std::endl;
    std::cout << "输入 'info' 查看校准参数" << std::endl;
    std::cout << "================\n" << std::endl;
    
    lastReadTime = millis();
}

void loop() {
    // 定期读取传感器数据
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        lastReadTime = now;
        readSensorData();
    }
    
    // 处理用户命令
    if (!handleCommands()) {
        exit(0); // 用户请求退出
    }
    
    delay(10); // 避免 CPU 占用过高
}

// --- 主程序入口 ---

int main() {
    setup();
    
    while (true) {
        loop();
    }
    
    return 0;
}

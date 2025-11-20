/*
 * 医用呼吸机边缘控制系统 - Luckfox Linux版本
 * 
 * 移植说明：
 * 1. 使用 LuckfoxArduino.h 提供的硬件抽象层
 * 2. setup() 和 loop() 函数保持Arduino风格
 * 3. main() 函数负责初始化并循环调用
 */

#include "LuckfoxArduino.h"
#include "BreathController.h"
#include "gas_concentration.h"
#include "I2CMux.h"

// 使用 ArduinoHAL 命名空间
using namespace ArduinoHAL;

// ===== I2C 配置 =====
// Luckfox I2C0 对应 /dev/i2c-0 (GPIO0_A7=SDA, GPIO0_B0=SCL)
// 运行 'i2cdetect -y 0' 可以扫描连接的设备
#define I2C_DEVICE "/dev/i2c-0"  // 使用 I2C0

// 创建 I2C 实例（使用自定义设备）
I2C CustomWire(I2C_DEVICE);

// 创建多路复用器实例
I2CMux i2cMux(0x70); // TCA9548地址为0x70

// 创建呼吸控制器，传入多路复用器
BreathController breathController(&i2cMux);

// Arduino风格的setup函数
void setup() {
    Serial.begin(115200);
    delay(100); // 等待串口稳定
    
    Serial.println("\n=================================================");
    Serial.println("医用呼吸机边缘控制系统启动 (Luckfox Linux版本)");
    Serial.println("=================================================\n");

    // 初始化I2C总线
    // 注意：根据 luckfox-config 的配置，可能需要修改设备号
    // 可用设备: /dev/i2c-0, /dev/i2c-1, /dev/i2c-2, /dev/i2c-3
    // 运行 'i2cdetect -l' 和 'i2cdetect -y X' 来确定正确的设备
    Serial.println("[初始化] 启动I2C总线...");
    Serial.println("[提示] 如果连接失败，请运行 'i2cdetect -y 0/1/2/3' 找到设备");
    Wire.begin();  // 默认使用 /dev/i2c-2
    delay(100);

    // 如果要使用UART模式，请在下面取消注释：
    // 1. 先设置通信模式
    // breathController.setACD1100CommunicationMode(COMM_UART);
    // delay(200);
    // 2. 然后初始化对应的串口
    // Serial1.begin(1200);  // 或 Serial2.begin(1200)
    // delay(500);
    // breathController.setACD1100UartPort(&Serial1);
    // delay(200);
    
    // 配置ADS1115和氧传感器（使用I2C多路复用器通道4）
    // ADS1115地址为0x4A
    Serial.println("[配置] 设置ADS1115通道...");
    breathController.setADS1115Channel(4);
    
    // 配置多路复用器通道
    Serial.println("[配置] 配置I2C多路复用器通道...");
    i2cMux.addChannel(0, 0x50, "流量传感器");         // 流量传感器在通道0
    i2cMux.addChannel(1, 0x6D, "SENSOR");             // 主气压传感器在通道1
    i2cMux.addChannel(2, 0x3C, "OLED Display");       // OLED在通道2
    i2cMux.addChannel(3, 0x6D, "备用气压传感器");     // 备用气压传感器在通道3
    i2cMux.addChannel(4, 0x4A, "ADS1115 ADC");        // ADS1115在通道4
    i2cMux.addChannel(5, 0x2A, "ACD1100气体传感器");  // ACD1100在通道5
    
    // 启用需要的通道
    Serial.println("[配置] 启用传感器通道...");
    i2cMux.enableChannel(0, false);  // 流量传感器
    i2cMux.enableChannel(1, true);   // 主气压传感器
    i2cMux.enableChannel(2, true);   // OLED
    i2cMux.enableChannel(3, true);   // 启用备用气压传感器
    i2cMux.enableChannel(4, false);  // 启用ADS1115 ADC
    i2cMux.enableChannel(5, true);   // 启用ACD1100气体传感器
    
    // 打印通道信息
    Serial.println("");
    i2cMux.printChannelInfo();
    
    // 测试ACD1100通道
    Serial.println("\n=== ACD1100通道测试 ===");
    Serial.println("测试通道5上的ACD1100...");
    
    if (i2cMux.selectChannel(5)) {
        Serial.println("通道5选择成功");
        
        // 测试I2C通信
        Wire.beginTransmission(0x2A);  // 7位地址
        uint8_t result = Wire.endTransmission();
        
        Serial.print("传感器地址0x2A测试结果: ");
        Serial.println(result);
        
        if (result == 0) {
            Serial.println("✓ ACD1100在通道5上响应正常！");
        } else {
            Serial.println("✗ ACD1100在通道5上无响应");
            
            // 扫描通道5上的所有I2C设备
            Serial.println("扫描通道5上的I2C设备...");
            int deviceCount = 0;
            for (uint8_t addr = 1; addr < 127; addr++) {
                Wire.beginTransmission(addr);
                uint8_t error = Wire.endTransmission();
                if (error == 0) {
                    Serial.print("找到设备，地址: 0x");
                    if (addr < 16) Serial.print("0");
                    Serial.print(addr);
                    Serial.print(" (");
                    Serial.print(addr);
                    Serial.println(")");
                    deviceCount++;
                }
            }
            if (deviceCount == 0) {
                Serial.println("通道5上未找到任何I2C设备");
            }
        }
    } else {
        Serial.println("✗ 通道5选择失败");
    }
    Serial.println("=== ACD1100测试完成 ===\n");
    
    // 初始化气压、温度以及控制器
    Serial.println("[初始化] 启动呼吸控制器...");
    breathController.begin();
    
    // 初始化氧传感器
    Serial.println("\n=== 初始化氧传感器 ===");
    breathController.initializeOxygenSensor();
    
    Serial.println("\n=== 系统初始化完成 ===");
    Serial.println("开始主循环...");
    Serial.println("ACD1100当前通信模式: I2C");
    Serial.println("========================\n");
}

// Arduino风格的loop函数
void loop() {
    // 更新气压、温度以及控制器状态（包含ACD1100）
    breathController.update();
    
    // 适当延时，避免过快刷新
    delay(10);
}

// Linux标准main函数
int main(int argc, char* argv[]) {
    // 打印启动信息
    std::cout << "医用呼吸机边缘控制系统 - Luckfox Linux版本" << std::endl;
    std::cout << "编译时间: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << std::endl;

    // 调用Arduino风格的setup函数（仅执行一次）
    try {
        setup();
    } catch (const std::exception& e) {
        std::cerr << "Setup failed with exception: " << e.what() << std::endl;
        return 1;
    }

    // 主循环：不断调用Arduino风格的loop函数
    std::cout << "\n进入主循环 (按Ctrl+C退出)...\n" << std::endl;
    
    try {
        while (true) {
            loop();
        }
    } catch (const std::exception& e) {
        std::cerr << "Loop terminated with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

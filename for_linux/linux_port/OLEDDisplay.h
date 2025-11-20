#ifndef OLEDDisplay_h
#define OLEDDisplay_h

#include "LuckfoxArduino.h"
#include "I2CMux.h"  // 包含多路复用器库

// 使用 ArduinoHAL 命名空间
using namespace ArduinoHAL;

// 注意: Linux版本暂时禁用OLED显示功能
// 需要移植 Adafruit_SSD1306 库或使用其他显示方案
#define OLED_DISABLED  // 定义此宏以禁用OLED功能

// OLED 配置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

class OLEDDisplay {
public:
    OLEDDisplay(I2CMux* mux = nullptr, uint8_t channel = 0); // 可传入多路复用器和通道
    bool begin();
    void update(float pressure, float temperature, const std::string& state, float valvePercent, float flow);
    void clearGraphs();
    void testDisplay();
    void resetDisplay();
    void simpleTest();
    void stabilizeDisplay();

    // 多路复用器设置
    void setMuxChannel(I2CMux* mux, uint8_t channel);

private:
    // Adafruit_SSD1306 display;  // 暂时禁用，需要移植库
    void* display;  // 占位符
    unsigned long lastUpdate = 0;
    I2CMux* _mux;
    uint8_t _channel;
    
    void selectDisplayChannel();
};

#endif
#include "OLEDDisplay.h"

// Linux 版本 - OLED 功能暂时禁用
// 需要移植 Adafruit_SSD1306 库或使用其他 Linux 显示方案

OLEDDisplay::OLEDDisplay(I2CMux* mux, uint8_t channel) 
    : display(nullptr), _mux(mux), _channel(channel) {
    // OLED 功能暂时禁用
}

void OLEDDisplay::setMuxChannel(I2CMux* mux, uint8_t channel) {
    _mux = mux;
    _channel = channel;
}

void OLEDDisplay::selectDisplayChannel() {
#ifdef OLED_DISABLED
    return;  // OLED 功能已禁用
#endif
    if (_mux) {
        _mux->selectChannel(_channel);
    }
}

bool OLEDDisplay::begin() {
#ifdef OLED_DISABLED
    Serial.println("[OLED] 显示功能已禁用 - 需要移植 Adafruit_SSD1306 库");
    Serial.println("[OLED] 传感器数据将通过串口输出");
    return false;
#else
    Serial.print("开始初始化OLED，通道: ");
    Serial.println(_channel);
    // TODO: 实现 Linux 版本的 OLED 初始化
    return false;
#endif
}

void OLEDDisplay::testDisplay() {
#ifdef OLED_DISABLED
    Serial.println("[OLED] 测试显示 - 功能已禁用");
    return;
#else
    // TODO: 实现测试显示
#endif
}

void OLEDDisplay::update(float pressure, float temperature, const std::string& state, float valvePercent, float flow) {
#ifdef OLED_DISABLED
    // 通过串口输出数据代替 OLED 显示
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint < 1000) return;  // 每秒打印一次
    lastPrint = millis();
    
    Serial.println("================================");
    Serial.print("压力: ");
    Serial.print(pressure, 2);
    Serial.println(" kPa");
    
    Serial.print("温度: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");
    
    Serial.print("流量: ");
    Serial.print(flow, 0);
    Serial.println(" ml/min");
    
    Serial.print("阀门: ");
    Serial.print(valvePercent, 0);
    Serial.println(" %");
    
    Serial.print("状态: ");
    Serial.println(state.c_str());
    Serial.println("================================");
#else
    // TODO: 实现真实的 OLED 更新
#endif
}

void OLEDDisplay::clearGraphs() {
#ifdef OLED_DISABLED
    return;
#else
    // TODO: 实现清除图形
#endif
}

void OLEDDisplay::resetDisplay() {
#ifdef OLED_DISABLED
    Serial.println("[OLED] 重置显示 - 功能已禁用");
    return;
#else
    // TODO: 实现重置显示
#endif
}

void OLEDDisplay::simpleTest() {
#ifdef OLED_DISABLED
    Serial.println("[OLED] 简单测试 - 功能已禁用");
    return;
#else
    // TODO: 实现简单测试
#endif
}

void OLEDDisplay::stabilizeDisplay() {
#ifdef OLED_DISABLED
    return;
#else
    // TODO: 实现稳定显示
#endif
}

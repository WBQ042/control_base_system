#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "TCA9548A_Mux.h"

// 屏幕尺寸
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// I2C OLED 通常没有 RESET 引脚
#define OLED_RESET_PIN -1

/**
 * @struct SensorData
 * @brief 一个用于汇总所有传感器数据的结构体，方便传递给显示函数
 */
struct SensorData {
    // XGZP6847D (压力)
    float pressure;
    float pressure_temp;
    bool pressure_ok;

    // ACD1100 (I2C CO2)
    uint32_t co2_i2c;
    int16_t co2_temp_raw; // ACD1100 I2C 的原始温度
    bool co2_i2c_ok;

    // ACD1100 (UART CO2)
    uint16_t co2_uart;
    bool co2_uart_ok;

    // AO-08 (O2 via ADS1115)
    float oxygen;
    bool oxygen_ok;

    // CAFS3000 (Gas Flow)
    float gas_flow;
    bool gas_flow_ok;

    // 构造函数，初始化所有值为 'false' 或 0
    SensorData() : 
        pressure(0.0f), pressure_temp(0.0f), pressure_ok(false),
        co2_i2c(0), co2_temp_raw(0), co2_i2c_ok(false),
        co2_uart(0), co2_uart_ok(false),
        oxygen(0.0f), oxygen_ok(false),
        gas_flow(0.0f), gas_flow_ok(false)
    {}
};


/**
 * @class OLED_Display
 * @brief 封装了 SSD1306 128x64 OLED 显示屏，
 * 专用于显示多传感器数据。
 */
class OLED_Display {
public:
    /**
     * @brief 构造函数
     * @param mux_ptr 指向 TCA9548A 多路复用器的指针
     * @param muxChannel OLED 连接到的 Mux 通道号
     * @param i2c_addr OLED 的 I2C 地址 (默认为 0x3C)
     */
    OLED_Display(TCA9548A_Mux* mux_ptr, uint8_t muxChannel, uint8_t i2c_addr = 0x3C);

    /**
     * @brief 初始化 OLED 显示屏
     * @return true 成功, false 失败
     */
    bool begin();

    /**
     * @brief (核心) 更新并刷新屏幕
     * 这是您请求的“上传函数”，它会显示 SensorData 结构体中的所有内容。
     * @param data 包含所有传感器读数的 SensorData 结构体
     */
    void update(const SensorData& data);

    /**
     * @brief 在屏幕上显示一条初始化消息 (用于 setup)
     * @param message 要显示的消息
     * @param delay_ms 显示后等待的时间
     */
    void showInitMessage(const char* message, uint16_t delay_ms = 1000);

    /**
     * @brief 在屏幕上显示一条永久的错误消息
     * @param errorMsg 错误信息
     */
    void showError(const char* errorMsg);

private:
    // 切换到 OLED 所在的 Mux 通道
    void selectMuxChannel();

    TCA9548A_Mux* _mux;
    uint8_t _muxChannel;
    uint8_t _i2c_addr;

    // Adafruit 库的实例
    Adafruit_SSD1306 display;
};

#endif // OLED_DISPLAY_H

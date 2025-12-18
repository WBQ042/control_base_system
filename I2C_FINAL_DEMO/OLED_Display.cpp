#include "OLED_Display.h"
#include <Arduino.h>

// 构造函数：在初始化列表中创建 Adafruit_SSD1306 对象
OLED_Display::OLED_Display(TCA9548A_Mux *mux_ptr, uint8_t muxChannel,
                           uint8_t i2c_addr)
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN) {
  _mux = mux_ptr;
  _muxChannel = muxChannel;
  _i2c_addr = i2c_addr;
}

void OLED_Display::selectMuxChannel() {
  if (_mux) {
    _mux->selectChannel(_muxChannel);
  }
}

bool OLED_Display::begin() {
  selectMuxChannel(); // 确保在正确的 Mux 通道上

  // 尝试初始化 SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, _i2c_addr)) {
    Serial.print("[OLED Error] SSD1306 (Channel ");
    Serial.print(_muxChannel);
    Serial.println(") 初始化失败。");
    return false;
  }

  Serial.print("OLED (Channel ");
  Serial.print(_muxChannel);
  Serial.println(") 初始化成功。");

  // 初始化显示
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED Display OK");
  display.display();
  delay(1000);
  return true;
}

void OLED_Display::showInitMessage(const char *message, uint16_t delay_ms) {
  selectMuxChannel();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(message);
  display.display();
  delay(delay_ms);
}

void OLED_Display::showError(const char *errorMsg) {
  selectMuxChannel();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("--- ERROR ---");
  display.println(errorMsg);
  display.display();
}

// 核心函数：更新显示
void OLED_Display::update(const SensorData &data) {
  selectMuxChannel();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  // 1. CO2 (优先显示 I2C, 否则显示 UART)
  display.print("CO2: ");
  if (data.co2_i2c_ok) {
    display.print(data.co2_i2c);
    display.println(" ppm (I2C)");
  } else if (data.co2_uart_ok) {
    display.print(data.co2_uart);
    display.println(" ppm (UART)");
  } else {
    display.println("ERR");
  }

  // 2. O2 (Oxygen)
  display.print("O2:  "); // 对齐
  if (data.oxygen_ok) {
    display.print(data.oxygen, 2); // 2位小数
    display.println(" %");
  } else {
    display.println("ERR");
  }

  // 3. Flow (Gas Flow)
  display.print("Flow:");
  if (data.gas_flow_ok) {
    display.print(data.gas_flow, 1);
    display.println(" L/min");
  } else {
    display.println("ERR");
  }

  // 4. Pressure (XGZP)
  display.print("Pres:");
  if (data.pressure_ok) {
    display.print(data.pressure, 0); // 气压不需要小数位
    display.println(" Pa");
  } else {
    display.println("ERR");
  }

  // 5. Temperature (仅显示气压计温度)
  display.print("Temp:");
  if (data.pressure_ok) {
    display.print(data.pressure_temp, 1);
    display.println(" C");
  } else {
    display.println("ERR");
  }

  // 刷新到屏幕
  display.display();
}

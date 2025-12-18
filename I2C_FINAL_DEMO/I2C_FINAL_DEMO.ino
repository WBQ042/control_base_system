/*
 * ===================================================================
 * 多传感器采集与显示 - 主程序
 * * 硬件: Arduino Nano ESP32
 * * 功能:
 * 1. 初始化 I2C Mux (TCA9548A)
 * 2. 初始化所有 I2C 传感器 (压力, CO2-I2C, 氧气, 流量, OLED)
 * 3. 初始化 UART 传感器 (CO2-UART)
 * 4. (关键) 在 Setup 中执行氧气传感器校准
 * 5. 在 Loop 中定期读取所有传感器数据
 * 6. 将数据汇总到 SensorData 结构体
 * 7. 调用 oled.update() 刷新显示
 * 8. 将数据打印到串口监视器
 * ===================================================================
 */

// 包含所有传感器库
#include "ACD1100_Sensor.h"
#include "ACD1100_Sensor_UART.h"
#include "AO08_Sensor.h"
#include "CAFS3000_Sensor.h"
#include "OLED_Display.h" // (这个 .h 文件包含了 SensorData 结构体)
#include "TCA9548A_Mux.h"
#include "XGZP6847D_Sensor.h"
#include <Wire.h>

// --- 1. 定义硬件连接和 Mux 通道 ---

// I2C Mux
TCA9548A_Mux mux(TCA9548A_DEFAULT_ADDRESS); // 0x70

// I2C 设备通道
const uint8_t MUX_CHANNEL_PRESSURE = 1; // XGZP6847D
const uint8_t MUX_CHANNEL_CO2_I2C = 5;  // ACD1100
const uint8_t MUX_CHANNEL_OXYGEN = 6;   // AO-08 (ADS1115)
const uint8_t MUX_CHANNEL_FLOW =
    0; // CAFS3000 (虽为Analog，但Mux通道定义保留备用)
const uint8_t MUX_CHANNEL_OLED = 2; // SSD1306

// UART CO2 传感器
#define UART_CO2_BAUD 1200
#define UART_CO2_RX_PIN 10
#define UART_CO2_TX_PIN 11

// --- 2. 实例化所有对象 ---

// 数据中心 & 标志位
SensorData allData;
bool co2_use_uart =
    false; // CO2 传感器模式标志: false=I2C(优先), true=UART(后备)

// 压力 (XGZP6847D)
XGZP6847D_Sensor pressureSensor(&mux, MUX_CHANNEL_PRESSURE, 16.0f);

// CO2 (I2C ACD1100)
ACD1100_Sensor co2_I2C_Sensor(&mux, MUX_CHANNEL_CO2_I2C);

// CO2 (UART ACD1100)
ACD1100_Sensor_UART co2_UART_Sensor(Serial1);

// 氧气 (AO-08 via ADS1115)
// 使用我们最新的驱动，显式指定 ADS1115 地址 (0x4A)
AO08_Sensor oxygenSensor(&mux, MUX_CHANNEL_OXYGEN, 0x4A);

// 流量 (CAFS3000)
// (量程 100 L/MIN, 使用模拟引脚 A0 读取电压)
CAFS3000_Sensor flowSensor(A0, 100.0f);

// OLED 显示
OLED_Display oled(&mux, MUX_CHANNEL_OLED, 0x3C);

// Loop 计时器
unsigned long lastReadTime = 0;
const long readInterval = 2000; // 每2秒读取一次 (ms)

// --- 3. 辅助函数 ---

// 氧气校准流程 (在 Setup 中调用)
void handleOxygenCalibration() {
  oled.showInitMessage("O2 CALIBRATION:\n打开串口监视器", 0);
  Serial.println("\n--- AO-08 氧气传感器校准 ---");
  Serial.println("请确保 AO-08 (ADS1115) 连接正确。");

  // 零点校准
  Serial.println("\n[步骤 1: 零点校准]");
  Serial.println("请将 AO-08 的 Vsensor+ 和 Vsensor- 引脚短接。");
  Serial.println("完成后，在下方输入 'z' 并按回车键...");
  oled.showInitMessage("O2 CAL (1/2):\n短接引脚\n(查看串口)", 0);

  while (Serial.available() == 0 || Serial.read() != 'z') {
    delay(100);
  }
  while (Serial.available())
    Serial.read(); // 清空缓冲区

  if (oxygenSensor.calibrateZero()) {
    Serial.println("零点校准成功！");
  } else {
    oled.showError("O2 零点校准\n失败！");
  }

  // 空气点校准
  Serial.println("\n[步骤 2: 空气点校准]");
  Serial.println("请移除短接线，将 AO-08 传感器暴露在新鲜空气中。");
  Serial.println("等待约 30 秒让传感器稳定。");
  Serial.println("完成后，在下方输入 'a' 并按回车键...");
  oled.showInitMessage("O2 CAL (2/2):\n暴露在空气中\n(查看串口)", 0);

  while (Serial.available() == 0 || Serial.read() != 'a') {
    delay(100);
  }
  while (Serial.available())
    Serial.read(); // 清空缓冲区

  if (oxygenSensor.calibrateAir()) {
    Serial.println("空气点校准成功！");
  } else {
    oled.showError("O2 空气点校准\n失败！");
  }
  Serial.println("--- 校准完成 ---");
}

// 逐个读取传感器并更新 allData
void readAllSensors() {
  // 1. 压力
  if (pressureSensor.startMeasurement()) {
    delay(20);
    allData.pressure_ok =
        pressureSensor.readData(allData.pressure, allData.pressure_temp);
  } else {
    allData.pressure_ok = false;
  }

  // 2. CO2 (根据初始化时的检测结果选择读取方式)
  if (!co2_use_uart) {
    // 优先尝试 I2C
    allData.co2_i2c_ok =
        co2_I2C_Sensor.readData(allData.co2_i2c, allData.co2_temp_raw);
    allData.co2_uart_ok = false;
  } else {
    // 后备使用 UART
    allData.co2_uart_ok = co2_UART_Sensor.readData(allData.co2_uart);
    allData.co2_i2c_ok = false;
  }

  // 3. 氧气
  allData.oxygen_ok = oxygenSensor.readOxygenPercentage(allData.oxygen);

  // 4. 流量
  allData.gas_flow_ok = flowSensor.readData(allData.gas_flow);
}

// 打印数据到串口监视器
void printToSerial(const SensorData &data) {
  Serial.println("--- Sensor Data ---");

  Serial.print("O2: \t\t");
  if (data.oxygen_ok)
    Serial.print(data.oxygen);
  else
    Serial.print("ERR");
  Serial.println(" %");

  Serial.print("Flow: \t\t");
  if (data.gas_flow_ok)
    Serial.print(data.gas_flow);
  else
    Serial.print("ERR");
  Serial.println(" ML/min");

  Serial.print("CO2 (I2C): \t");
  if (data.co2_i2c_ok)
    Serial.print(data.co2_i2c);
  else
    Serial.print("ERR");
  Serial.println(" ppm");

  Serial.print("CO2 (UART): \t");
  if (data.co2_uart_ok)
    Serial.print(data.co2_uart);
  else
    Serial.print("ERR");
  Serial.println(" ppm");

  Serial.print("Pressure: \t");
  if (data.pressure_ok)
    Serial.print(data.pressure);
  else
    Serial.print("ERR");
  Serial.println(" Pa");

  Serial.print("Temp (Press): \t");
  if (data.pressure_ok)
    Serial.print(data.pressure_temp);
  else
    Serial.print("ERR");
  Serial.println(" C");

  Serial.println("---------------------");
}

// --- 4. SETUP ---
void setup() {
  Serial.begin(115200);
  while (!Serial)
    ; // 等待串口

  Serial.println("系统启动中...");
  Wire.begin();
  Serial1.begin(UART_CO2_BAUD, SERIAL_8N1, UART_CO2_RX_PIN, UART_CO2_TX_PIN);
  mux.begin();

  if (!oled.begin()) {
    Serial.println("OLED 初始化失败! 停止。");
    while (1)
      ;
  }
  oled.showInitMessage("OLED OK\n正在初始化...", 1000);

  // 初始化传感器
  if (!pressureSensor.begin()) {
    oled.showError("Pressure Snsr\nFailed!");
    delay(1000); // 让用户看到错误
  }

  // CO2 初始化逻辑：优先 I2C
  Serial.println("正在初始化 CO2 (I2C)...");
  if (co2_I2C_Sensor.begin()) {
    Serial.println("CO2 (I2C) 初始化成功。使用 I2C 模式。");
    co2_use_uart = false;
  } else {
    Serial.println("CO2 (I2C) 初始化失败。尝试 fallback 到 UART...");
    co2_use_uart = true;
    if (!co2_UART_Sensor.begin()) {
      Serial.println("[Warning] CO2-UART 传感器也未响应。");
      oled.showError("CO2 Snsr\nAll Failed!");
      delay(1000);
    } else {
      Serial.println("CO2 (UART) 初始化成功。使用 UART 模式。");
    }
  }

  if (!oxygenSensor.begin()) {
    oled.showError("Oxygen(ADS1115)\nFailed!");
    delay(1000);
  }

  if (!flowSensor.begin()) {
    oled.showError("Gas Flow Snsr\nFailed!");
    delay(1000);
  }

  // 氧气校准
  handleOxygenCalibration();

  oled.showInitMessage("初始化完成！", 2000);
  lastReadTime = millis();
}

// --- 5. LOOP ---
void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;
    readAllSensors();
    oled.update(allData);
    printToSerial(allData);
  }
}

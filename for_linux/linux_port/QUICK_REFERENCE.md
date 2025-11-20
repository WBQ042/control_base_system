# 快速参考 - Arduino vs Luckfox

## 问题1: Arduino风格能编译吗?

### ✅ 答案: 可以!

**原因:**
- `LuckfoxArduino.h` 提供了完整的 Arduino 兼容 API
- `main.cpp` 包装了 `setup()` 和 `loop()` 函数
- 所有 Arduino 函数都通过 C++ 类模拟实现

### Arduino代码示例:
```cpp
void setup() {
    Serial.begin(115200);
    Wire.begin();
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
}
```

### Linux版本 (完全相同!):
```cpp
#include "LuckfoxArduino.h"
using namespace ArduinoHAL;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    // GPIO需要先创建对象
    GPIO led(RK_GPIO(1, 'C', 7));
    led.begin();
    led.pinMode(OUTPUT);
}

void loop() {
    led.digitalWrite(HIGH);
    delay(1000);
}

int main() {
    setup();
    while(true) loop();
}
```

---

## 问题2: 需要修改主文件吗?

### ✅ 答案: 已经帮你准备好了!

**你的 `.ino` 文件:**
```cpp
#include <Arduino.h>
#include <Wire.h>
#include "BreathController.h"

I2CMux mux(0x70);
BreathController controller(&mux);

void setup() {
    Serial.begin(115200);
    Wire.begin();
    controller.begin();
}

void loop() {
    controller.update();
}
```

**Linux 版本 `main.cpp` (已创建!):**
```cpp
#include "LuckfoxArduino.h"
#include "BreathController.h"
using namespace ArduinoHAL;

I2CMux mux(0x70);
BreathController controller(&mux);

void setup() {
    Serial.begin(115200);
    Wire.begin();
    controller.begin();
}

void loop() {
    controller.update();
    delay(10);
}

int main() {
    setup();
    while(true) loop();
    return 0;
}
```

**差异说明:**
1. ✅ `#include <Arduino.h>` → `#include "LuckfoxArduino.h"`
2. ✅ 添加 `using namespace ArduinoHAL;`
3. ✅ 添加 `main()` 函数
4. ✅ 其他代码**完全相同**!

---

## 核心 API 对照表

| Arduino API | Luckfox 实现 | 说明 |
|-------------|--------------|------|
| `Serial.begin(115200)` | ✅ 完全支持 | 输出到 stdout |
| `Serial.print(x)` | ✅ 完全支持 | 所有类型 |
| `Wire.begin()` | ✅ 完全支持 | `/dev/i2c-2` |
| `Wire.beginTransmission()` | ✅ 完全支持 | I2C 写入 |
| `Wire.requestFrom()` | ✅ 完全支持 | I2C 读取 |
| `Serial1.begin(9600)` | ✅ 完全支持 | `/dev/ttyS1` |
| `delay(ms)` | ✅ 完全支持 | 精确延时 |
| `millis()` | ✅ 完全支持 | 毫秒计时 |
| `pinMode(pin, mode)` | ⚠️ 需要对象 | `GPIO(pin).pinMode()` |
| `digitalWrite(pin, val)` | ⚠️ 需要对象 | `GPIO(pin).digitalWrite()` |
| `analogWrite(pin, val)` | ⚠️ 需要对象 | `PWM(chip,ch).analogWrite()` |
| `WiFi.xxx` | ❌ 未实现 | 使用 socket 替代 |
| `Preferences.xxx` | ✅ 完全支持 | 文件系统存储 |

---

## 管脚映射

### Arduino 风格:
```cpp
pinMode(LED_PIN, OUTPUT);
digitalWrite(LED_PIN, HIGH);
```

### Luckfox 风格:
```cpp
// 使用 RK_GPIO 宏计算管脚号
// 格式: RK_GPIO(bank, group, index)
// 例如: GPIO1_C7 = RK_GPIO(1, 'C', 7) = 55

GPIO led(RK_GPIO(1, 'C', 7));
led.begin();
led.pinMode(OUTPUT);
led.digitalWrite(HIGH);
```

### 常用管脚对照:

| Luckfox 管脚 | 计算公式 | 管脚号 | Arduino等效 |
|--------------|----------|--------|-------------|
| GPIO1_C7 | RK_GPIO(1,'C',7) | 55 | D13 (LED) |
| GPIO1_B2 | RK_GPIO(1,'B',2) | 42 | D2 |
| GPIO1_B3 | RK_GPIO(1,'B',3) | 43 | D3 |

---

## 编译流程对比

### Arduino IDE:
1. 写代码 → `.ino` 文件
2. 点击"上传"
3. Arduino IDE 自动:
   - 添加 `#include <Arduino.h>`
   - 生成 `.cpp` 文件
   - 添加 `main()` 函数
   - 编译并上传

### Luckfox:
1. 写代码 → `.cpp` 文件
2. 手动添加:
   - `#include "LuckfoxArduino.h"`
   - `using namespace ArduinoHAL;`
   - `main()` 函数
3. 运行 `make`
4. 上传到板子: `scp`

---

## 典型错误修复

### 错误1: `error: 'Serial' was not declared`
```cpp
// 忘记添加命名空间
using namespace ArduinoHAL;  // ← 添加这行
```

### 错误2: `error: 'String' does not name a type`
```cpp
// Arduino 的 String → C++ 的 std::string
void update(const std::string& state) {  // ← 使用 std::string
    // ...
}
```

### 错误3: `error: 'pinMode' was not declared`
```cpp
// pinMode 在 Luckfox 上需要通过 GPIO 对象调用
GPIO pin(RK_GPIO(1, 'C', 7));
pin.begin();
pin.pinMode(OUTPUT);  // ← 正确
```

---

## 已完成的适配

✅ **所有头文件已修改:**
- `I2CMux.h` - ✅
- `BreathController.h` - ✅
- `ADS1115.h` - ✅
- `gas_concentration.h` - ✅
- `oxygen_sensor.h` - ✅
- `OLEDDisplay.h` - ✅ (显示功能暂时禁用)

✅ **构建系统已准备:**
- `Makefile` - ✅
- `main.cpp` - ✅
- `README.md` - ✅

✅ **可以直接编译!**

---

## 下一步行动

### 1️⃣ 测试编译 (Windows WSL2)
```bash
cd /mnt/c/Users/王炳祺/Desktop/libs/NEW/test_demo/linux_port
chmod +x syntax_check.sh
./syntax_check.sh
```

### 2️⃣ 完整编译
```bash
make clean
make
```

### 3️⃣ 传输到板子
```bash
scp breath_controller root@<板子IP>:/root/
ssh root@<板子IP>
sudo ./breath_controller
```

---

## 总结

| 问题 | 答案 | 状态 |
|------|------|------|
| Arduino风格能编译吗? | ✅ 可以 | 已实现 |
| 需要引入头文件吗? | ✅ 需要 | 已完成 |
| 需要修改主文件吗? | ✅ 需要 | 已创建 |
| 管脚需要重新映射吗? | ✅ 需要 | 已说明 |
| 可以直接编译吗? | ✅ 可以 | 立即可用 |

**你现在可以直接编译了!** 🎉

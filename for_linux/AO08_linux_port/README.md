# AO08 氧气传感器校准工具 - 部署说明

## 项目简介

这是一个专门用于 AO08 电化学氧气传感器校准的独立工具，移植到 Luckfox 嵌入式 Linux 平台。

## 功能特性

- **两点校准**：零点校准（短接）+ 空气点校准（20.95% O2）
- **参数持久化**：校准参数保存到非易失性存储
- **交互式命令**：支持多种命令控制传感器读取和校准
- **实时监测**：定期读取并显示氧气浓度和传感器电压

## 硬件配置

### I2C 连接
- **I2C 总线**：`/dev/i2c-0` (GPIO0_A7=SDA, GPIO0_B0=SCL)
- **TCA9548A Multiplexer**：地址 0x70
- **ADS1115 ADC**：地址 0x4A，连接到 Mux 通道 6
- **AO08 氧气传感器**：连接到 ADS1115 的 AIN0(+) 和 AIN1(-)

### 修改硬件配置

如果你的硬件配置不同，请修改 `main.cpp` 中的以下常量：

```cpp
const uint8_t MUX_ADDRESS = 0x70;        // TCA9548A 地址
const uint8_t MUX_CHANNEL_ADS1115 = 6;   // ADS1115 连接的 Mux 通道
const uint8_t ADS1115_ADDRESS = 0x4A;    // ADS1115 I2C 地址
```

## 编译步骤

### 1. 在 Linux 主机或 WSL 中编译

```bash
# 进入项目目录
cd AO08_linux_port

# 确保交叉编译工具链已安装
# arm-none-linux-gnueabihf-g++ 应该在 PATH 中

# 编译
make

# 检查生成的二进制文件
ls -lh ao08_calibration
```

### 2. 传输到 Luckfox 板

```bash
# 使用 scp 传输（假设板子 IP 是 192.168.1.100）
scp ao08_calibration root@192.168.1.100:/root/

# 或使用 TFTP、USB 等其他传输方式
```

## 板上部署

### 1. 设置 I2C 设备权限

```bash
# 临时方法：修改权限
chmod 666 /dev/i2c-0

# 永久方法：创建 udev 规则
echo 'KERNEL=="i2c-[0-9]*", MODE="0666"' > /etc/udev/rules.d/99-i2c.rules
udevadm control --reload-rules
udevadm trigger
```

### 2. 验证 I2C 设备

```bash
# 安装 i2c-tools（如果未安装）
# apt-get install i2c-tools   # Debian/Ubuntu
# opkg install i2c-tools       # OpenWrt

# 检测 I2C 设备
i2cdetect -y 0

# 应该看到：
# - 0x70: TCA9548A multiplexer
# - 0x4A: ADS1115 ADC (在选择 Mux 通道 6 后)
```

### 3. 运行程序

```bash
# 赋予执行权限
chmod +x ao08_calibration

# 运行校准工具
./ao08_calibration
```

## 使用说明

### 交互式命令

程序启动后，支持以下命令：

| 命令 | 说明 |
|------|------|
| `cal` / `calibrate` | 执行完整的两点校准流程 |
| `info` / `status` | 显示当前校准参数 |
| `test` / `voltage` | 测试电压读取，检查硬件连接 |
| `clear` | 清除已保存的校准参数 |
| `help` | 显示帮助信息 |
| `exit` / `quit` | 退出程序 |

### 校准流程

#### 第一步：零点校准
1. 输入命令 `cal` 并按回车
2. 将 AO08 传感器的 **Vsensor+** 和 **Vsensor-** 引脚短接
3. 输入 `z` 并按回车确认
4. 系统自动采集零点电压（约2秒）

#### 第二步：空气点校准
1. 移除短接线
2. 将传感器完全暴露在新鲜空气中（室外最佳）
3. 等待传感器稳定（建议1-2分钟）
4. 输入 `a` 并按回车确认
5. 系统自动采集空气点电压（约2秒）

#### 完成
- 校准参数自动保存到 `/tmp/ao08_calibration.conf`
- 下次启动程序会自动加载保存的参数

### 实时监测

程序每 **2 秒**自动读取一次传感器数据，显示：
- 氧气浓度（百分比）
- 传感器电压（mV）

## 故障排除

### 错误：传感器初始化失败

**症状**：
```
[AO08] 错误: 传感器初始化失败！
```

**可能原因**：
1. I2C 设备路径错误
2. ADS1115 未正确连接或地址错误
3. Mux 通道配置错误

**解决方法**：
```bash
# 检查 I2C 设备
i2cdetect -y 0

# 如果使用其他 I2C 总线（如 i2c-1），修改 LuckfoxArduino.h：
# I2C::I2C(const std::string& device) : device_path("/dev/i2c-1") { ... }
```

### 错误：I2C 通信失败

**症状**：
```
原因: I2C 通信失败
```

**可能原因**：
1. I2C 设备权限不足
2. 硬件连接问题
3. I2C 总线冲突

**解决方法**：
```bash
# 检查权限
ls -l /dev/i2c-0
# 应该显示：crw-rw-rw- 或类似

# 修改权限
sudo chmod 666 /dev/i2c-0

# 检查硬件连接
i2cdetect -y 0
```

### 错误：传感器未校准

**症状**：
```
原因: 传感器未校准
解决方案: 输入 'cal' 执行校准
```

**解决方法**：
执行校准流程（见上文"校准流程"）

### 错误：校准参数无效

**症状**：
```
原因: 校准参数无效
可能的问题: 校准参数异常 (空气电压 <= 零点电压)
```

**可能原因**：
1. 零点校准时传感器未正确短接
2. 空气点校准时传感器未暴露在空气中
3. 传感器老化或损坏

**解决方法**：
1. 输入 `clear` 清除错误的校准参数
2. 重新执行校准流程，确保按照步骤正确操作
3. 使用 `test` 命令检查原始电压读数是否正常

### 错误：ADC 转换超时

**症状**：
```
原因: ADC 转换超时
```

**可能原因**：
1. ADS1115 未正确连接
2. I2C 时序问题
3. 硬件故障

**解决方法**：
```bash
# 检查 ADS1115 是否在正确的 Mux 通道上
i2cdetect -y 0

# 降低 I2C 总线速度（在 LuckfoxArduino.h 中修改）
# 或检查硬件连接
```

## 配置文件

校准参数保存在 `/tmp/ao08_calibration.conf`：

```ini
[AO08_Calibration]
voltageZero=0.0234
voltageAir=12.3456
```

**注意**：`/tmp` 目录在重启后会清空，如需永久保存，修改 `AO08_CalibrationStorage.cpp` 中的文件路径：

```cpp
// 修改为持久化目录
const std::string CONFIG_FILE_PATH = "/etc/ao08_calibration.conf";
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.cpp` | 主程序入口，包含交互式命令处理 |
| `AO08_Sensor.h/cpp` | 氧气传感器驱动，封装 ADS1115 读取和校准逻辑 |
| `AO08_CalibrationStorage.h/cpp` | 校准参数持久化存储 |
| `I2CMux.h/cpp` | TCA9548A I2C 多路复用器驱动 |
| `LuckfoxArduino.h` | Arduino 兼容层（I2C、Serial、Preferences 等） |
| `Makefile` | 交叉编译配置 |
| `README.md` | 本文档 |

## 技术细节

### 校准算法

**线性模型**：
```
O2% = (V_sensor - V_zero) / (V_air - V_zero) × 20.95%
```

其中：
- `V_sensor`：当前传感器电压（mV）
- `V_zero`：零点电压（短接时的电压）
- `V_air`：空气点电压（20.95% O2 时的电压）

### ADC 配置

**ADS1115 设置**：
- **增益**：±0.256V (16x gain)
- **采样率**：128 SPS
- **模式**：单次转换
- **输入**：差分 AIN0(+) vs AIN1(-)
- **分辨率**：16-bit (15-bit + sign)

**电压转换**：
```
mV = ADC_value × (256.0 mV / 32768 steps) = ADC_value × 0.0078125 mV
```

## 作者与许可

基于原 Arduino/ESP32 版本移植到 Luckfox 平台。

## 更新日志

- **v1.0** (2024): 初始版本，完成 Linux 平台移植

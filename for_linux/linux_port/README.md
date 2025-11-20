# Luckfox Linux 移植指南

## 📋 概述

本文件夹包含医用呼吸机边缘控制系统的 Luckfox Linux 移植版本。

## ✅ 已完成的准备工作

### 1. 硬件抽象层 (LuckfoxArduino.h)
提供了完整的 Arduino 兼容 API:
- ✅ **I2C 通信** - 使用 `/dev/i2c-2`
- ✅ **UART 串口** - 支持 Serial1/Serial2
- ✅ **GPIO 控制** - 通过 sysfs
- ✅ **PWM 输出** - 通过 sysfs
- ✅ **时间函数** - millis(), delay()
- ✅ **Preferences 存储** - 文件系统模拟

### 2. 源码适配
所有传感器模块已复制并修改:
- ✅ 替换 `#include <Arduino.h>` 为 `#include "LuckfoxArduino.h"`
- ✅ 添加 `using namespace ArduinoHAL;`
- ✅ 移除 ESP32/WiFi 相关依赖

### 3. 构建系统
- ✅ **Makefile** - 完整的编译脚本
- ✅ **main.cpp** - Linux 入口程序 (setup/loop 风格)

## 🔧 编译前的准备

### 硬件配置

#### 1. 启用 I2C
```bash
# 在 Luckfox 板上运行
sudo luckfox-config
# 选择: Interface Options -> I2C -> Enable
# 重启后检查
ls /dev/i2c-*
# 应该看到: /dev/i2c-2
```

#### 2. 启用 UART (可选,如果使用ACD1100的UART模式)
```bash
sudo luckfox-config
# 选择: Interface Options -> Serial -> Enable
ls /dev/ttyS*
# 应该看到: /dev/ttyS1, /dev/ttyS2
```

#### 3. 用户权限
```bash
# 添加当前用户到 i2c 和 dialout 组
sudo usermod -a -G i2c,dialout $USER
# 注销并重新登录使权限生效
```

### 软件依赖

#### 如果在板上本地编译
```bash
# 安装 g++ 和 make (通常已预装)
sudo apt-get update
sudo apt-get install build-essential
```

#### 如果交叉编译 (推荐)
```bash
# 在开发机上安装交叉编译工具链
# 根据你的 Luckfox 板子架构选择:
# - RV1103/RV1106: ARM Cortex-A7
# - 其他型号请查看官方文档

# 修改 Makefile 第一行:
# CXX = arm-linux-gnueabihf-g++
```

## 🚀 编译步骤

### 方法1: 在 Luckfox 板上本地编译 (推荐新手)

1. **传输文件到板子**
```bash
# 在 Windows PowerShell 中 (使用 scp)
scp -r linux_port/* root@<板子IP>:/root/breath_controller/
```

2. **SSH 登录到板子**
```bash
ssh root@<板子IP>
cd /root/breath_controller
```

3. **编译**
```bash
make clean    # 清理旧文件
make info     # 查看编译信息
make          # 开始编译
```

4. **运行**
```bash
sudo ./breath_controller
```

### 方法2: 交叉编译 (推荐高级用户)

1. **修改 Makefile**
```makefile
# 第8行改为:
CXX = arm-linux-gnueabihf-g++
```

2. **在 Windows 上编译 (WSL2)**
```bash
# 进入 WSL2
cd /mnt/c/Users/王炳祺/Desktop/libs/NEW/test_demo/linux_port
make clean
make
```

3. **传输到板子**
```bash
scp breath_controller root@<板子IP>:/root/
```

4. **SSH 登录运行**
```bash
ssh root@<板子IP>
cd /root
sudo ./breath_controller
```

## 📝 编译问题排查

### 问题1: 找不到 `/dev/i2c-2`
```bash
# 检查 I2C 是否启用
ls /dev/i2c-*
# 如果没有,运行:
sudo luckfox-config
# 启用 I2C,然后重启
```

### 问题2: 权限错误 (Permission denied)
```bash
# 方案1: 使用 sudo 运行
sudo ./breath_controller

# 方案2: 添加用户权限
sudo usermod -a -G i2c,dialout $USER
# 注销后重新登录
```

### 问题3: 编译错误 - 缺少头文件
```bash
# 确保所有源文件都在 linux_port 目录
ls -la *.h *.cpp

# 应该看到:
# LuckfoxArduino.h
# main.cpp
# I2CMux.h/cpp
# BreathController.h/cpp
# ADS1115.h/cpp
# gas_concentration.h/cpp
# oxygen_sensor.h/cpp
# OLEDDisplay.h/cpp
# Makefile
```

### 问题4: I2C 通信失败
```bash
# 扫描 I2C 设备
sudo i2cdetect -y 2

# 应该看到类似:
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- -- 
# 30: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- -- 
# ...
# 70: 70 -- -- -- -- -- -- --
```

## ⚠️ 已知限制

### 当前版本限制:
1. **OLED 显示暂未移植** - 需要移植 Adafruit_SSD1306 库
2. **WiFi 功能已移除** - 需要使用其他网络方案
3. **Preferences 存储在 /tmp** - 重启会丢失,可改为 `/etc` 持久化

### 未来改进计划:
- [ ] 移植 OLED 显示库 (SSD1306)
- [ ] 添加网络功能 (TCP/HTTP)
- [ ] 优化 I2C 性能
- [ ] 添加看门狗支持

## 📊 性能对比

| 特性 | Arduino/ESP32 | Luckfox Linux |
|------|---------------|---------------|
| CPU | 240MHz 双核 | 1.2GHz ARM |
| 内存 | 520KB | 64MB+ |
| I2C速度 | 100-400kHz | 100-400kHz |
| 启动时间 | <1秒 | 2-5秒 |
| 功耗 | ~80mA | ~200-500mA |
| 扩展性 | 有限 | 强大 |

## 🔗 相关文档

- [LuckfoxArduino.h 硬件抽象层文档](LuckfoxArduino.h)
- [Luckfox 官方文档](https://wiki.luckfox.com)
- [项目主 README](../README.md)

## 💡 使用技巧

### 调试输出
```cpp
// 所有 Serial.print() 输出到标准输出
// 可以重定向到文件:
./breath_controller > log.txt 2>&1
```

### 后台运行
```bash
# 使用 nohup 后台运行
nohup sudo ./breath_controller > output.log 2>&1 &

# 查看进程
ps aux | grep breath_controller

# 停止程序
sudo killall breath_controller
```

### 开机自启动
```bash
# 创建 systemd 服务
sudo nano /etc/systemd/system/breath-controller.service

# 添加内容:
[Unit]
Description=Breath Controller System
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/root/breath_controller
ExecStart=/root/breath_controller/breath_controller
Restart=always

[Install]
WantedBy=multi-user.target

# 启用服务
sudo systemctl enable breath-controller.service
sudo systemctl start breath-controller.service
```

## 📞 支持

如有问题,请检查:
1. 硬件连接是否正确
2. I2C/UART 是否已启用
3. 用户权限是否配置
4. 传感器地址是否正确

---
**编译时间**: 自动生成  
**目标平台**: Luckfox Pico / Luckfox Pro  
**最低内核版本**: Linux 4.19+

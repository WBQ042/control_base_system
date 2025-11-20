# Qt5 Ventilator Control GUI

## 项目简介

这是一个基于 Qt5 的图形化界面程序，用于 Luckfox 嵌入式 Linux 平台的呼吸机传感器控制和氧气传感器校准。

## 功能特性

### 1. 呼吸控制器监控
- **实时数据显示**：压力、流量、CO2、O2 浓度
- **数据可视化**：压力和流量的实时曲线图
- **控制按钮**：启动/停止监控、重置数据

### 2. 氧气传感器校准
- **两点校准**：零点校准（短接）+ 空气点校准（20.95% O2）
- **实时读数**：显示当前电压和氧气浓度
- **校准管理**：保存/加载/清除校准参数
- **电压测试**：测试传感器连接状态

### 3. 界面特性
- **标签页切换**：在呼吸控制和氧气校准之间切换
- **状态栏**：显示 I2C 连接状态和系统时间
- **暗色主题**：适合嵌入式设备显示
- **响应式布局**：适配不同屏幕尺寸

## 编译和部署

### 环境要求

- **Qt5 开发库**（5.9 或更高版本）
- **Qt Charts 模块**
- **ARM 交叉编译工具链**

### 在 Linux 主机上编译

#### 1. 安装 Qt5 交叉编译环境

```bash
# 如果还没有安装 Qt5 for ARM
# 需要从 Qt 官网下载或自行编译 Qt5 for ARM
```

#### 2. 配置交叉编译

编辑 `VentilatorGUI.pro`，确保交叉编译器配置正确：

```qmake
unix {
    # 交叉编译配置
    QMAKE_CXX = arm-none-linux-gnueabihf-g++
    QMAKE_LINK = arm-none-linux-gnueabihf-g++
}
```

#### 3. 生成 Makefile 并编译

```bash
# 进入项目目录
cd qt5_gui

# 使用 qmake 生成 Makefile
qmake VentilatorGUI.pro

# 编译
make

# 生成的可执行文件
ls -lh VentilatorGUI
```

### 在板上运行

#### 1. 传输程序到板子

```bash
# 使用 scp 传输（假设板子 IP 是 192.168.1.100）
scp VentilatorGUI root@192.168.1.100:/root/

# 或使用 USB/TFTP 等其他传输方式
```

#### 2. 配置运行环境

```bash
# 在板子上执行

# 设置 Qt 环境变量（根据实际路径调整）
export QT_QPA_PLATFORM=linuxfb
export QT_QPA_FONTDIR=/usr/share/fonts

# 设置 I2C 设备权限
chmod 666 /dev/i2c-0

# 运行程序
./VentilatorGUI
```

#### 3. 可选：使用 X11 或 Wayland

如果板上有 X Server 或 Wayland：

```bash
export DISPLAY=:0
./VentilatorGUI
```

## 项目结构

```
qt5_gui/
├── VentilatorGUI.pro          # Qt 项目文件
├── main.cpp                   # 程序入口
├── MainWindow.h/cpp           # 主窗口（标签页容器）
├── BreathControlWidget.h/cpp  # 呼吸控制器界面
├── OxygenCalibrationWidget.h/cpp # 氧气校准界面
├── SensorDataPlot.h/cpp       # 数据图表组件
├── resources.qrc              # 资源文件
├── README.md                  # 本文档
└── build/                     # 编译产物目录
    ├── obj/
    ├── moc/
    ├── ui/
    └── rcc/
```

## 使用说明

### 呼吸控制器监控

1. 切换到 **"Breath Controller"** 标签页
2. 点击 **"Start Monitoring"** 开始监控
3. 实时查看：
   - LCD 显示：压力、流量、CO2、O2 浓度
   - 实时曲线：压力和流量变化趋势
4. 点击 **"Stop"** 停止监控
5. 点击 **"Reset"** 清空图表数据

### 氧气传感器校准

#### 零点校准

1. 切换到 **"O2 Calibration"** 标签页
2. 将 AO08 传感器的 **Vsensor+** 和 **Vsensor-** 引脚短接
3. 点击 **"Zero Point (Short Circuit)"** 按钮
4. 在弹出对话框中确认已短接，点击 **"Yes"**
5. 等待校准完成（约 2 秒）

#### 空气点校准

1. 移除短接线
2. 将传感器完全暴露在新鲜空气中
3. 等待 1-2 分钟使传感器稳定
4. 点击 **"Air Point (20.95% O2)"** 按钮
5. 在弹出对话框中确认已稳定，点击 **"Yes"**
6. 等待校准完成（约 2 秒）

#### 其他功能

- **Test Voltage**：测试当前传感器电压读数，验证连接
- **Clear Calibration**：清除已保存的校准参数

## 硬件配置

### I2C 设备地址

- **TCA9548A Multiplexer**: 0x70
- **ADS1115 ADC**: 0x4A（连接到 Mux 通道 6）
- **ACD1100 CO2 Sensor**: 0x2A
- **Pressure/Flow Sensors**: 通过 Mux 其他通道连接

### 修改配置

如果硬件配置不同，修改对应 Widget 的初始化代码：

```cpp
// BreathControlWidget.cpp
controller = new BreathController();

// OxygenCalibrationWidget.cpp
mux = new I2CMux(0x70);  // Mux 地址
oxygenSensor = new AO08_Sensor(mux, 6, 0x4A);  // 通道和 ADS1115 地址
```

## 故障排除

### 错误：无法初始化传感器

**症状**：启动时弹出 "Initialization Error"

**解决方法**：
```bash
# 检查 I2C 设备
i2cdetect -y 0

# 确认权限
ls -l /dev/i2c-0
chmod 666 /dev/i2c-0

# 检查硬件连接
```

### 错误：Qt platform plugin 未找到

**症状**：
```
This application failed to start because no Qt platform plugin could be initialized.
```

**解决方法**：
```bash
# 设置平台插件
export QT_QPA_PLATFORM=linuxfb

# 或指定插件路径
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt5/plugins
```

### 错误：字体缺失

**症状**：界面文字显示异常

**解决方法**：
```bash
# 安装字体包
opkg install dejavu-fonts
# 或
apt-get install fonts-dejavu-core

# 设置字体目录
export QT_QPA_FONTDIR=/usr/share/fonts
```

### 界面显示不正常

**症状**：界面布局错误或控件显示异常

**解决方法**：
```bash
# 尝试不同的 Qt 平台插件
export QT_QPA_PLATFORM=eglfs     # OpenGL ES
export QT_QPA_PLATFORM=linuxfb   # Linux Framebuffer
export QT_QPA_PLATFORM=xcb       # X11（需要 X Server）
```

## 性能优化

### 降低 CPU 占用

修改更新频率：

```cpp
// BreathControlWidget.cpp
updateTimer->start(200);  // 从 100ms 改为 200ms

// OxygenCalibrationWidget.cpp
readingTimer->start(1000);  // 从 500ms 改为 1000ms
```

### 减少图表数据点

```cpp
// SensorDataPlot.cpp
maxDataPoints(50)  // 从 100 改为 50
```

### 禁用动画和抗锯齿

```cpp
// SensorDataPlot.cpp
chart->setAnimationOptions(QChart::NoAnimation);
chartView->setRenderHint(QPainter::Antialiasing, false);
```

## 开发说明

### 添加新传感器

1. 在 `BreathControlWidget` 中添加新的 LCD 显示
2. 在 `updateSensorData()` 中读取并显示新传感器数据
3. 可选：添加新的图表显示

### 修改界面布局

- 编辑 `.ui` 文件（需要 Qt Designer）
- 或直接修改 `setupUI()` 函数中的布局代码

### 调试技巧

```bash
# 启用 Qt 调试输出
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES='*.debug=true'

# 运行程序
./VentilatorGUI
```

## 依赖项

- Qt5 Core
- Qt5 Widgets
- Qt5 Charts
- pthread
- libm (数学库)

## 作者与许可

基于 Luckfox 平台医疗设备传感器系统的 Qt5 图形化界面。

## 更新日志

- **v1.0** (2024): 初始版本，支持呼吸控制器监控和氧气传感器校准

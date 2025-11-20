# Qt5 GUI Compilation Guide

## Quick Start

### 1. Native Compilation (for testing on PC)

```bash
cd qt5_gui
qmake VentilatorGUI.pro
make
./VentilatorGUI
```

### 2. Cross-Compilation for Luckfox

```bash
# Set up Qt cross-compilation environment
export PATH=/path/to/qt5-arm/bin:$PATH
export QMAKESPEC=/path/to/qt5-arm/mkspecs/linux-arm-gnueabihf-g++

# Build
cd qt5_gui
qmake VentilatorGUI.pro
make

# Deploy
scp VentilatorGUI root@<board-ip>:/root/
```

## Notes

1. Requires Qt5 with Charts module
2. Needs ARM cross-compiled Qt5 libraries for embedded target
3. Ensure I2C device permissions on target: `chmod 666 /dev/i2c-0`
4. Set Qt platform: `export QT_QPA_PLATFORM=linuxfb`

See README.md for detailed instructions.

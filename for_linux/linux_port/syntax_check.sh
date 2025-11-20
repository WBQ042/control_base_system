#!/bin/bash
# 快速语法检查脚本 (在 WSL2 或 Linux 上运行)

echo "======================================"
echo "Luckfox Breath Controller - 语法检查"
echo "======================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查编译器
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ 错误: 未找到 g++ 编译器${NC}"
    echo "请安装: sudo apt-get install build-essential"
    exit 1
fi

echo -e "${GREEN}✓ 找到编译器: $(g++ --version | head -n1)${NC}"
echo ""

# 检查源文件
echo "检查源文件..."
FILES=(
    "LuckfoxArduino.h"
    "main.cpp"
    "I2CMux.h"
    "I2CMux.cpp"
    "BreathController.h"
    "BreathController.cpp"
    "ADS1115.h"
    "ADS1115.cpp"
    "gas_concentration.h"
    "gas_concentration.cpp"
    "oxygen_sensor.h"
    "oxygen_sensor.cpp"
    "OLEDDisplay.h"
    "OLEDDisplay.cpp"
    "Makefile"
)

MISSING=0
for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $file"
    else
        echo -e "${RED}✗${NC} $file (缺失)"
        MISSING=$((MISSING + 1))
    fi
done

if [ $MISSING -gt 0 ]; then
    echo -e "\n${RED}错误: 缺少 $MISSING 个文件${NC}"
    exit 1
fi

echo ""
echo "======================================"
echo "开始语法检查..."
echo "======================================"
echo ""

# 仅做语法检查 (不链接)
CPP_FILES=(
    "main.cpp"
    "I2CMux.cpp"
    "BreathController.cpp"
    "ADS1115.cpp"
    "gas_concentration.cpp"
    "oxygen_sensor.cpp"
    "OLEDDisplay.cpp"
)

ERRORS=0
for cpp in "${CPP_FILES[@]}"; do
    echo -n "检查 $cpp ... "
    if g++ -std=c++11 -fsyntax-only -I. "$cpp" 2>/tmp/error_$$.txt; then
        echo -e "${GREEN}✓ 通过${NC}"
    else
        echo -e "${RED}✗ 失败${NC}"
        cat /tmp/error_$$.txt
        ERRORS=$((ERRORS + 1))
    fi
    rm -f /tmp/error_$$.txt
done

echo ""
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}======================================"
    echo "✓ 语法检查全部通过!"
    echo "======================================${NC}"
    echo ""
    echo "下一步:"
    echo "1. 运行 'make clean && make' 完整编译"
    echo "2. 传输到 Luckfox 板子测试"
    exit 0
else
    echo -e "${RED}======================================"
    echo "✗ 发现 $ERRORS 个文件有语法错误"
    echo "======================================${NC}"
    exit 1
fi

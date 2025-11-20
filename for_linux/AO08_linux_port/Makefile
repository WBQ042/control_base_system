# ============================================================================
# Makefile for AO08 Oxygen Sensor Calibration Tool (Luckfox Linux Port)
# ============================================================================

# 交叉编译工具链
CXX = arm-none-linux-gnueabihf-g++

# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -I.

# 链接选项
LDFLAGS = -lpthread -lstdc++ -lm

# 目标可执行文件
TARGET = ao08_calibration

# 源文件
SOURCES = main.cpp \
          AO08_Sensor.cpp \
          AO08_CalibrationStorage.cpp \
          I2CMux.cpp

# 头文件
HEADERS = LuckfoxArduino.h \
          AO08_Sensor.h \
          AO08_CalibrationStorage.h \
          I2CMux.h

# 对象文件
OBJECTS = $(SOURCES:.cpp=.o)

# ============================================================================
# 构建规则
# ============================================================================

# 默认目标：构建可执行文件
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

# 编译 C++ 源文件为目标文件
%.o: %.cpp $(HEADERS)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# 清理规则
# ============================================================================

# 清除编译产物
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete."

# ============================================================================
# 其他规则
# ============================================================================

# 显示帮助信息
help:
	@echo "Available targets:"
	@echo "  all      - Build the executable (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Display this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build the program"
	@echo "  make clean     # Clean build files"
	@echo ""
	@echo "Cross-compiler: $(CXX)"
	@echo "Target binary:  $(TARGET)"

# 伪目标声明
.PHONY: all clean help

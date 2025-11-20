# Makefile for Luckfox Breath Controller
# 医用呼吸机边缘控制系统 - 编译脚本

# ============= 编译器配置 =============
# 交叉编译器 (根据你的工具链调整)
# 如果在Luckfox板上本地编译，使用: CXX = g++
# 如果交叉编译，使用: CXX = arm-none-linux-gnueabihf-g++
CXX = arm-none-linux-gnueabihf-g++

# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
CXXFLAGS += -I. 

# 链接选项
LDFLAGS = -lpthread -lstdc++ -lm

# ============= 源文件配置 =============
# 主程序
MAIN_SRC = main.cpp

# 传感器模块源文件
SENSOR_SRCS = \
	ADS1115.cpp \
	oxygen_sensor.cpp \
	gas_concentration.cpp \
	I2CMux.cpp \
	OLEDDisplay.cpp \
	BreathController.cpp

# 所有源文件
SRCS = $(MAIN_SRC) $(SENSOR_SRCS)

# 生成的目标文件 (.o)
OBJS = $(SRCS:.cpp=.o)

# 可执行文件名
TARGET = breath_controller

# ============= 编译规则 =============
.PHONY: all clean info install test

# 默认目标：编译可执行文件
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS)
	@echo "======================================"
	@echo "链接可执行文件: $@"
	@echo "======================================"
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ 编译成功！"
	@echo "可执行文件: $(TARGET)"
	@echo ""

# 编译 .cpp 文件为 .o 文件
%.o: %.cpp
	@echo "编译: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译产物
clean:
	@echo "清理编译文件..."
	rm -f $(OBJS) $(TARGET)
	@echo "✓ 清理完成"

# 显示编译信息
info:
	@echo "======================================"
	@echo "Luckfox Breath Controller 编译信息"
	@echo "======================================"
	@echo "编译器: $(CXX)"
	@echo "编译选项: $(CXXFLAGS)"
	@echo "链接选项: $(LDFLAGS)"
	@echo ""
	@echo "源文件:"
	@for src in $(SRCS); do echo "  - $$src"; done
	@echo ""
	@echo "目标文件: $(TARGET)"
	@echo "======================================"

# 安装到系统目录 (需要root权限)
install: $(TARGET)
	@echo "安装 $(TARGET) 到 /usr/local/bin/"
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✓ 安装完成"

# 快速测试编译
test: clean all
	@echo ""
	@echo "======================================"
	@echo "运行测试..."
	@echo "======================================"
	@echo "注意: 需要硬件连接才能正常运行"
	@echo "按Ctrl+C退出"
	@echo ""
	./$(TARGET)

# ============= 依赖关系 =============
# 自动生成头文件依赖
-include $(OBJS:.o=.d)

%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< > $@

.PHONY: deps
deps:
	@echo "生成依赖文件..."
	@for src in $(SRCS); do \
		$(CXX) $(CXXFLAGS) -MM -MT $${src%.cpp}.o $$src > $${src%.cpp}.d; \
	done
	@echo "✓ 依赖生成完成"

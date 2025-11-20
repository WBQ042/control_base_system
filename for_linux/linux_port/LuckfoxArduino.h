#ifndef LUCKFOX_ARDUINO_H
#define LUCKFOX_ARDUINO_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <cstring>
#include <termios.h>
#include <map>
#include <cmath>    // 数学函数: isnan, fabs 等

// I2C ioctl 命令
#ifndef I2C_SLAVE
#define I2C_SLAVE 0x0703
#endif

// --- 基础常量定义 ---
#define HIGH 1
#define LOW 0
#define INPUT "in"
#define OUTPUT "out"

// --- Serial 打印格式常量 ---
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

// --- 数学常量 ---
#ifndef NAN
#define NAN (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif

// --- GPIO 计算宏 (Bank, Group, Index) ---
// 用法: RK_GPIO(1, 'C', 7) -> 计算出 55
#define RK_GPIO_GRP(g) ((g) - 'A')
#define RK_GPIO(bank, grp, idx) ((bank) * 32 + RK_GPIO_GRP(grp) * 8 + (idx))

// --- 引入标准数学函数到全局命名空间 ---
using std::isnan;
using std::isinf;
using std::isfinite;
using std::fabs;
using std::abs;
using std::sqrt;
using std::pow;
using std::sin;
using std::cos;
using std::tan;
using std::round;
using std::floor;
using std::ceil;

namespace ArduinoHAL {

    // --- 时间函数 ---
    inline unsigned long millis() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    inline unsigned long micros() {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }

    inline void delay(unsigned long ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    inline void delayMicroseconds(unsigned int us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

    // --- 数学和工具函数 ---
    template<typename T>
    inline T constrain(T value, T min_val, T max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }

    // 重载版本：处理混合类型（自动转换为 float）
    inline float constrain(float value, float min_val, float max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }

    inline double constrain(double value, double min_val, double max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }

    template<typename T>
    inline T map(T x, T in_min, T in_max, T out_min, T out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    template<typename T>
    inline T min(T a, T b) {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline T max(T a, T b) {
        return (a > b) ? a : b;
    }

    inline long random(long min_val, long max_val) {
        return min_val + (std::rand() % (max_val - min_val));
    }

    inline long random(long max_val) {
        return std::rand() % max_val;
    }

    // --- 全局函数：pinMode/digitalWrite (简化版本) ---
    // 注意：简化实现，建议创建 GPIO 对象进行控制
    inline void pinMode(int pin, const char* mode) {
        // 占位符实现
        static bool warned = false;
        if (!warned) {
            std::cout << "[pinMode] 警告: GPIO 功能简化实现，建议使用 GPIO 类" << std::endl;
            warned = true;
        }
        // TODO: 实际应该创建全局 GPIO 对象映射表
    }

    inline void digitalWrite(int pin, int value) {
        // 占位符实现
        static bool warned = false;
        if (!warned) {
            std::cout << "[digitalWrite] 警告: GPIO 功能简化实现，建议使用 GPIO 类" << std::endl;
            warned = true;
        }
        // TODO: 实际应该使用全局 GPIO 对象映射表
    }

    inline int digitalRead(int pin) {
        // 占位符实现
        return 0;
    }

    // --- 全局函数：analogWrite (PWM 输出占位符) ---
    // 注意：简化实现，仅用于编译通过
    // 实际使用时需要根据硬件配置 PWM
    inline void analogWrite(int pin, int value) {
        // 占位符实现：实际需要配置 /sys/class/pwm/
        // 这里仅打印调试信息
        static bool warned = false;
        if (!warned) {
            std::cout << "[analogWrite] 警告: PWM 功能未完全实现，需要手动配置硬件" << std::endl;
            warned = true;
        }
        // TODO: 实现真实的 PWM 控制
    }

    // --- GPIO 控制类 (模拟 pinMode/digitalWrite) ---
    class GPIO {
    private:
        int pin;
        bool exported = false;

        void write_sysfs(const std::string& path, const std::string& value) {
            std::ofstream fs(path);
            if (fs.is_open()) {
                fs << value;
                fs.close();
            }
        }
        
        // 读取文件内容
        std::string read_sysfs(const std::string& path) {
            std::ifstream fs(path);
            std::string value;
            if (fs.is_open()) {
                fs >> value;
                fs.close();
            }
            return value;
        }

    public:
        GPIO(int gpio_pin) : pin(gpio_pin) {}

        ~GPIO() {
           // 可选：析构时 unexport，但通常保留以供后续使用
           // if (exported) write_sysfs("/sys/class/gpio/unexport", std::to_string(pin));
        }

        void begin() {
            // 检查是否已经存在，不存在则导出
            std::string pin_path = "/sys/class/gpio/gpio" + std::to_string(pin);
            if (access(pin_path.c_str(), F_OK) != 0) {
                write_sysfs("/sys/class/gpio/export", std::to_string(pin));
                exported = true;
                 // 等待udev创建权限，有时需要一点时间
                delay(50);
            }
        }

        void pinMode(const std::string& mode) {
            begin();
            write_sysfs("/sys/class/gpio/gpio" + std::to_string(pin) + "/direction", mode);
        }

        void digitalWrite(int value) {
            write_sysfs("/sys/class/gpio/gpio" + std::to_string(pin) + "/value", std::to_string(value));
        }

        int digitalRead() {
            std::string val = read_sysfs("/sys/class/gpio/gpio" + std::to_string(pin) + "/value");
            try {
                return std::stoi(val);
            } catch (...) { return 0; }
        }
    };

    // --- PWM 控制类 (模拟 analogWrite) ---
    // Linux PWM需要指定 芯片号(chip) 和 通道号(channel)
    // 例如: PWM0_CH1 -> chip 0, channel 1 (具体取决于设备树配置，可能需要要在 /sys/class/pwm/ 下查看)
    class PWM {
    private:
        int chip;
        int channel;
        unsigned long period_ns = 1000000; // 默认周期 1ms (1kHz)

        void write_pwm(const std::string& file, const std::string& value) {
            std::string path = "/sys/class/pwm/pwmchip" + std::to_string(chip) + "/pwm" + std::to_string(channel) + "/" + file;
            std::ofstream fs(path);
            if (fs.is_open()) {
                fs << value;
                fs.close();
            }
        }

    public:
        PWM(int pwm_chip, int pwm_channel) : chip(pwm_chip), channel(pwm_channel) {}

        void begin() {
             // 导出PWM通道
             std::string export_path = "/sys/class/pwm/pwmchip" + std::to_string(chip) + "/export";
             std::string channel_path = "/sys/class/pwm/pwmchip" + std::to_string(chip) + "/pwm" + std::to_string(channel);
             
             if (access(channel_path.c_str(), F_OK) != 0) {
                 std::ofstream fs(export_path);
                 if (fs.is_open()) {
                     fs << channel;
                     fs.close();
                     delay(50); 
                 }
             }
        }

        // 设置频率 (Hz)
        void setFrequency(unsigned long freq_hz) {
            period_ns = 1000000000UL / freq_hz;
            write_pwm("period", std::to_string(period_ns));
        }

        // 启动PWM
        void enable() {
            write_pwm("enable", "1");
        }

        // 停止PWM
        void disable() {
            write_pwm("enable", "0");
        }

        // 模拟Arduino的 analogWrite (0-255)
        void analogWrite(int duty) {
            if (duty < 0) duty = 0;
            if (duty > 255) duty = 255;
            
            unsigned long duty_ns = (period_ns * duty) / 255;
            write_pwm("duty_cycle", std::to_string(duty_ns));
        }
        
        // 更精确的占空比设置 (0.0 - 1.0)
        void setDutyPercentage(float percent) {
             if (percent < 0.0) percent = 0.0;
             if (percent > 1.0) percent = 1.0;
             unsigned long duty_ns = (unsigned long)(period_ns * percent);
             write_pwm("duty_cycle", std::to_string(duty_ns));
        }
    };

    // --- ADC 控制类 (模拟 analogRead) ---
    // 通常位于 /sys/bus/iio/devices/iio:device0/in_voltageX_raw
    class ADC {
    private:
        int channel;
        std::string device_path;

    public:
        ADC(int adc_channel, int device_idx = 0) : channel(adc_channel) {
            device_path = "/sys/bus/iio/devices/iio:device" + std::to_string(device_idx) + "/";
        }

        int analogRead() {
            std::string path = device_path + "in_voltage" + std::to_string(channel) + "_raw";
            std::ifstream fs(path);
            std::string value;
            if (fs.is_open()) {
                fs >> value;
                fs.close();
                try {
                    return std::stoi(value);
                } catch (...) { return -1; }
            }
            return -1; // 读取失败
        }
    };

    // --- I2C 控制类 (核心传感器通信) ---
    class I2C {
    private:
        int fd;
        std::string device;
        uint8_t current_addr;
        std::vector<uint8_t> tx_buffer;
        std::vector<uint8_t> rx_buffer;

        void write_sysfs(const std::string& path, const std::string& value) {
            std::ofstream fs(path);
            if (fs.is_open()) {
                fs << value;
                fs.close();
            }
        }

    public:
        I2C(const std::string& dev = "/dev/i2c-0") : device(dev), fd(-1), current_addr(0) {}

        ~I2C() {
            end();
        }

        // 初始化 I2C (可选设置 SDA/SCL 引脚,主要用于记录)
        void begin(int sda_pin = -1, int scl_pin = -1) {
            fd = open(device.c_str(), O_RDWR);
            if (fd < 0) {
                std::cerr << "[I2C] Failed to open device: " << device << std::endl;
                std::cerr << "[I2C] Make sure I2C is enabled via luckfox-config" << std::endl;
            } else {
                std::cout << "[I2C] Opened " << device << " successfully" << std::endl;
            }
        }

        void end() {
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
        }

        // 开始传输到指定地址
        void beginTransmission(uint8_t addr) {
            current_addr = addr;
            tx_buffer.clear();
            
            if (fd >= 0 && ioctl(fd, I2C_SLAVE, addr) < 0) {
                std::cerr << "[I2C] Failed to set slave address 0x" 
                          << std::hex << (int)addr << std::dec << std::endl;
            }
        }

        // 结束传输并发送数据
        uint8_t endTransmission(bool sendStop = true) {
            if (fd < 0) return 4; // 其他错误
            
            if (tx_buffer.empty()) {
                return 0; // 成功
            }

            ssize_t result = ::write(fd, tx_buffer.data(), tx_buffer.size());
            tx_buffer.clear();
            
            if (result < 0) {
                std::cerr << "[I2C] Write failed to address 0x" 
                          << std::hex << (int)current_addr << std::dec << std::endl;
                return 2; // NACK on address
            }
            
            return 0; // 成功
        }

        // 写入单字节到缓冲区
        size_t write(uint8_t data) {
            tx_buffer.push_back(data);
            return 1;
        }

        // 写入多字节到缓冲区
        size_t write(const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; i++) {
                tx_buffer.push_back(data[i]);
            }
            return len;
        }

        // 从设备读取数据
        size_t requestFrom(uint8_t addr, size_t len, bool sendStop = true) {
            if (fd < 0) return 0;
            
            if (ioctl(fd, I2C_SLAVE, addr) < 0) {
                std::cerr << "[I2C] Failed to set slave address for read 0x" 
                          << std::hex << (int)addr << std::dec << std::endl;
                return 0;
            }

            rx_buffer.clear();
            rx_buffer.resize(len);
            
            ssize_t result = ::read(fd, rx_buffer.data(), len);
            if (result < 0) {
                std::cerr << "[I2C] Read failed from address 0x" 
                          << std::hex << (int)addr << std::dec << std::endl;
                rx_buffer.clear();
                return 0;
            }
            
            rx_buffer.resize(result);
            return result;
        }

        // 读取接收缓冲区中的一个字节
        int read() {
            if (rx_buffer.empty()) return -1;
            uint8_t data = rx_buffer.front();
            rx_buffer.erase(rx_buffer.begin());
            return data;
        }

        // 查看接收缓冲区中有多少字节可读
        int available() {
            return rx_buffer.size();
        }

        // 设置时钟频率 (可选实现)
        void setClock(uint32_t frequency) {
            // Linux I2C 驱动通常在设备树中配置频率
            // 这里仅作记录
            std::cout << "[I2C] Clock frequency set to " << frequency << " Hz (may require DT config)" << std::endl;
        }
    };

    // --- HardwareSerial 类 (UART 串口通信) ---
    class HardwareSerial {
    private:
        int fd;
        std::string device;

        bool configure_port(int baud) {
            struct termios tty;
            memset(&tty, 0, sizeof(tty));

            if (tcgetattr(fd, &tty) != 0) {
                std::cerr << "[UART] Error getting port attributes" << std::endl;
                return false;
            }

            // 设置波特率
            speed_t speed;
            switch(baud) {
                case 9600:   speed = B9600; break;
                case 19200:  speed = B19200; break;
                case 38400:  speed = B38400; break;
                case 57600:  speed = B57600; break;
                case 115200: speed = B115200; break;
                default:     speed = B9600; break;
            }
            cfsetospeed(&tty, speed);
            cfsetispeed(&tty, speed);

            // 8N1 模式
            tty.c_cflag &= ~PARENB;        // 无奇偶校验
            tty.c_cflag &= ~CSTOPB;        // 1 停止位
            tty.c_cflag &= ~CSIZE;
            tty.c_cflag |= CS8;            // 8 数据位
            tty.c_cflag &= ~CRTSCTS;       // 禁用硬件流控
            tty.c_cflag |= CREAD | CLOCAL; // 启用读取,忽略调制解调器控制线

            // 原始模式
            tty.c_lflag &= ~ICANON;
            tty.c_lflag &= ~ECHO;
            tty.c_lflag &= ~ECHOE;
            tty.c_lflag &= ~ECHONL;
            tty.c_lflag &= ~ISIG;

            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

            tty.c_oflag &= ~OPOST;
            tty.c_oflag &= ~ONLCR;

            // 非阻塞读取
            tty.c_cc[VTIME] = 0;
            tty.c_cc[VMIN] = 0;

            if (tcsetattr(fd, TCSANOW, &tty) != 0) {
                std::cerr << "[UART] Error setting port attributes" << std::endl;
                return false;
            }

            return true;
        }

    public:
        HardwareSerial(const std::string& dev = "/dev/ttyS1") : device(dev), fd(-1) {}

        ~HardwareSerial() {
            end();
        }

        void begin(unsigned long baud, uint32_t config = 0) {
            fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
            if (fd < 0) {
                std::cerr << "[UART] Failed to open device: " << device << std::endl;
                std::cerr << "[UART] Make sure UART is enabled via luckfox-config" << std::endl;
                return;
            }

            if (configure_port(baud)) {
                std::cout << "[UART] Opened " << device << " at " << baud << " baud" << std::endl;
            } else {
                close(fd);
                fd = -1;
            }
        }

        void end() {
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
        }

        int available() {
            if (fd < 0) return 0;
            int bytes = 0;
            ioctl(fd, FIONREAD, &bytes);
            return bytes;
        }

        int read() {
            if (fd < 0) return -1;
            uint8_t data;
            ssize_t n = ::read(fd, &data, 1);
            return (n == 1) ? data : -1;
        }

        size_t readBytes(uint8_t* buffer, size_t length) {
            if (fd < 0) return 0;
            ssize_t n = ::read(fd, buffer, length);
            return (n > 0) ? n : 0;
        }

        size_t write(uint8_t data) {
            if (fd < 0) return 0;
            return ::write(fd, &data, 1);
        }

        size_t write(const uint8_t* buffer, size_t size) {
            if (fd < 0) return 0;
            return ::write(fd, buffer, size);
        }

        size_t print(const char* str) {
            return write((const uint8_t*)str, strlen(str));
        }

        size_t print(const std::string& str) {
            return write((const uint8_t*)str.c_str(), str.length());
        }

        size_t println(const char* str) {
            size_t n = print(str);
            n += write((uint8_t)'\n');
            return n;
        }

        size_t println(const std::string& str) {
            size_t n = print(str);
            n += write((uint8_t)'\n');
            return n;
        }

        void flush() {
            if (fd >= 0) {
                tcdrain(fd);
            }
        }
    };

    // --- Preferences 类 (模拟 ESP32 NVS 存储) ---
    class Preferences {
    private:
        std::string namespace_name;
        std::string config_file;
        std::map<std::string, std::string> data;
        bool readonly_mode;

        std::string get_config_path() {
            // 使用 /tmp 或 /etc (需要权限)
            return "/tmp/preferences_" + namespace_name + ".conf";
        }

        void load_from_file() {
            std::ifstream fs(config_file);
            if (!fs.is_open()) return;

            std::string line;
            while (std::getline(fs, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    data[key] = value;
                }
            }
            fs.close();
        }

        void save_to_file() {
            if (readonly_mode) return;
            
            std::ofstream fs(config_file);
            if (!fs.is_open()) {
                std::cerr << "[Preferences] Failed to save to " << config_file << std::endl;
                return;
            }

            for (const auto& pair : data) {
                fs << pair.first << "=" << pair.second << std::endl;
            }
            fs.close();
        }

    public:
        Preferences() : readonly_mode(false) {}

        bool begin(const char* name, bool readonly = false) {
            namespace_name = name;
            readonly_mode = readonly;
            config_file = get_config_path();
            
            load_from_file();
            std::cout << "[Preferences] Opened namespace '" << name 
                      << "' (" << (readonly ? "RO" : "RW") << ")" << std::endl;
            return true;
        }

        void end() {
            save_to_file();
            data.clear();
        }

        bool clear() {
            if (readonly_mode) return false;
            data.clear();
            save_to_file();
            return true;
        }

        bool remove(const char* key) {
            if (readonly_mode) return false;
            data.erase(key);
            save_to_file();
            return true;
        }

        bool isKey(const char* key) {
            return data.find(key) != data.end();
        }

        // Float 类型
        float getFloat(const char* key, float defaultValue = 0.0f) {
            if (!isKey(key)) return defaultValue;
            try {
                return std::stof(data[key]);
            } catch (...) {
                return defaultValue;
            }
        }

        bool putFloat(const char* key, float value) {
            if (readonly_mode) return false;
            data[key] = std::to_string(value);
            save_to_file();
            return true;
        }

        // Int 类型
        int getInt(const char* key, int defaultValue = 0) {
            if (!isKey(key)) return defaultValue;
            try {
                return std::stoi(data[key]);
            } catch (...) {
                return defaultValue;
            }
        }

        bool putInt(const char* key, int value) {
            if (readonly_mode) return false;
            data[key] = std::to_string(value);
            save_to_file();
            return true;
        }

        // String 类型
        std::string getString(const char* key, const std::string& defaultValue = "") {
            if (!isKey(key)) return defaultValue;
            return data[key];
        }

        bool putString(const char* key, const std::string& value) {
            if (readonly_mode) return false;
            data[key] = value;
            save_to_file();
            return true;
        }

        // Bool 类型
        bool getBool(const char* key, bool defaultValue = false) {
            if (!isKey(key)) return defaultValue;
            return data[key] == "1" || data[key] == "true";
        }

        bool putBool(const char* key, bool value) {
            if (readonly_mode) return false;
            data[key] = value ? "1" : "0";
            save_to_file();
            return true;
        }
    };
    
    // --- 简易的 Serial 模拟 (用于打印调试信息到终端) ---
    class SerialMock {
    public:
        void begin(int baud) {
            std::cout << "[Serial] Init at " << baud << " (Mocked to stdout)" << std::endl;
        }
        void print(const char* str) { std::cout << str; }
        void print(const std::string& str) { std::cout << str; }
        void print(int val) { std::cout << val; }
        void print(unsigned int val) { std::cout << val; }
        void print(long val) { std::cout << val; }
        void print(unsigned long val) { std::cout << val; }
        void print(int val, int format) { 
            if (format == HEX) {
                std::cout << "0x" << std::hex << val << std::dec;
            } else if (format == BIN) {
                // 二进制输出
                std::cout << "0b";
                for (int i = 31; i >= 0; i--) {
                    std::cout << ((val >> i) & 1);
                }
            } else if (format == OCT) {
                std::cout << "0" << std::oct << val << std::dec;
            } else {
                std::cout << val;
            }
        }
        void print(unsigned int val, int format) {
            print((int)val, format);
        }
        void print(uint8_t val, int format) {
            print((int)val, format);
        }
        void print(float val, int decimals = 2) { 
            std::cout << std::fixed;
            std::cout.precision(decimals);
            std::cout << val; 
        }
        void println(const char* str) { std::cout << str << std::endl; }
        void println(const std::string& str) { std::cout << str << std::endl; }
        void println(int val) { std::cout << val << std::endl; }
        void println(unsigned int val) { std::cout << val << std::endl; }
        void println(long val) { std::cout << val << std::endl; }
        void println(unsigned long val) { std::cout << val << std::endl; }
        void println(int val, int format) { 
            print(val, format);
            std::cout << std::endl;
        }
        void println(unsigned int val, int format) {
            print((int)val, format);
            std::cout << std::endl;
        }
        void println(uint8_t val, int format) {
            print((int)val, format);
            std::cout << std::endl;
        }
        void println(float val, int decimals = 2) { 
            print(val, decimals);
            std::cout << std::endl;
        }
        void println() { std::cout << std::endl; }
    };
    
    // --- 全局实例定义 ---
    static SerialMock Serial;
    static I2C Wire;
    static HardwareSerial Serial1("/dev/ttyS1");  // UART1
    static HardwareSerial Serial2("/dev/ttyS2");  // UART2

} // namespace ArduinoHAL

#endif
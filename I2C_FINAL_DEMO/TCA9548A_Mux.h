#ifndef TCA9548A_MUX_H
#define TCA9548A_MUX_H

#include <Wire.h> // 需要用到Arduino的Wire库进行I2C通信

// 默认的I2C地址，用户可以根据A0/A1/A2的设置来修改
#define TCA9548A_DEFAULT_ADDRESS 0x70

class TCA9548A_Mux {
public:
    /**
     * @brief 构造函数，初始化TCA9548A对象
     * @param addr TCA9548A自身的I2C地址 (默认为0x70)
     */
    TCA9548A_Mux(uint8_t addr = TCA9548A_DEFAULT_ADDRESS);

    /**
     * @brief 初始化I2C总线。
     * 由于Arduino Nano ESP32只有一个Wire对象，此函数仅设置内部地址。
     */
    void begin();

    /**
     * @brief 切换到指定的I2C通道。
     * @param channel 要切换的通道号 (0到7)
     * @return true 切换成功, false 切换失败 (例如通道号无效)
     */
    bool selectChannel(uint8_t channel);

    /**
     * @brief 重置TCA9548A，断开所有通道。
     * @return true 操作成功
     */
    bool disableAllChannels();

private:
    uint8_t _muxAddress; // TCA9548A多路复用器的I2C地址
};

#endif // TCA9548A_MUX_H
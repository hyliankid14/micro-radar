#pragma once

#include <Wire.h>

#define ES8311_ADDR_7BIT        0x18

#define ES8311_REG00_RESET      0x00
#define ES8311_REG01_CLK        0x01
#define ES8311_REG02_CLK        0x02
#define ES8311_REG03_CLK        0x03
#define ES8311_REG04_CLK        0x04
#define ES8311_REG05_CLK        0x05
#define ES8311_REG06_CLK        0x06
#define ES8311_REG07_CLK        0x07
#define ES8311_REG08_CLK        0x08
#define ES8311_REG09_SDPIN      0x09
#define ES8311_REG0A_SDPOUT     0x0A
#define ES8311_REG0D_SYSTEM     0x0D
#define ES8311_REG0E_SYSTEM     0x0E
#define ES8311_REG12_SYSTEM     0x12
#define ES8311_REG13_SYSTEM     0x13
#define ES8311_REG14_SYSTEM     0x14
#define ES8311_REG1C_ADC        0x1C
#define ES8311_REG31_DAC        0x31
#define ES8311_REG32_DAC        0x32
#define ES8311_REG37_DAC        0x37

class ES8311Config {
public:
    ES8311Config(TwoWire* wire = &Wire);

    bool init();
    bool setVolume(uint8_t percent);
    bool setMute(bool mute);
    bool enablePA(int paPin);
    bool disablePA(int paPin);
    bool isReady() const { return codecReady; }

private:
    TwoWire* i2c;
    bool codecReady;
    uint8_t currentVolume;

    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
};

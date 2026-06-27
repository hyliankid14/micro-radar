#include "ES8311Config.h"
#include <Arduino.h>

ES8311Config::ES8311Config(TwoWire* wire)
    : i2c(wire), codecReady(false), currentVolume(75) {}

void ES8311Config::writeReg(uint8_t reg, uint8_t value) {
    i2c->beginTransmission((uint8_t)ES8311_ADDR_7BIT);
    i2c->write(reg);
    i2c->write(value);
    uint8_t err = i2c->endTransmission();
    if (err != 0) {
        Serial.printf("[ES8311] Write FAILED reg=0x%02X val=0x%02X i2c_err=%d\n", reg, value, err);
    }
}

uint8_t ES8311Config::readReg(uint8_t reg) {
    i2c->beginTransmission((uint8_t)ES8311_ADDR_7BIT);
    i2c->write(reg);
    uint8_t err = i2c->endTransmission(false);
    if (err != 0) {
        Serial.printf("[ES8311] Read NACK reg=0x%02X err=%d\n", reg, err);
        return 0xFF;
    }
    i2c->requestFrom((uint8_t)ES8311_ADDR_7BIT, (uint8_t)1);
    if (i2c->available()) {
        return i2c->read();
    }
    return 0xFF;
}

bool ES8311Config::init() {
    if (!i2c) {
        Serial.println("[ES8311] No I2C bus provided");
        return false;
    }

    i2c->beginTransmission((uint8_t)ES8311_ADDR_7BIT);
    if (i2c->endTransmission() != 0) {
        Serial.printf("[ES8311] No device at I2C 0x%02X\n", ES8311_ADDR_7BIT);

        Serial.println("[ES8311] Scanning I2C bus:");
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            i2c->beginTransmission(addr);
            if (i2c->endTransmission() == 0) {
                Serial.printf("[ES8311]   Found device at 0x%02X\n", addr);
            }
        }
        return false;
    }

    Serial.printf("[ES8311] Device detected at I2C 0x%02X\n", ES8311_ADDR_7BIT);

    readReg(0x00);
    readReg(0x0D);
    readReg(0x12);

    writeReg(ES8311_REG00_RESET, 0x1F);
    delay(10);
    writeReg(ES8311_REG00_RESET, 0x00);
    writeReg(ES8311_REG00_RESET, 0x80);

    // Clock config: use BCLK as clock source (BIT7=1 of REG01)
    // In this mode, ES8311 derives all internal clocks from BCLK
    // No separate MCLK signal needed - simpler and avoids MCLK frequency issues
    writeReg(ES8311_REG01_CLK, 0xBF);
    writeReg(ES8311_REG02_CLK, 0x08);
    writeReg(ES8311_REG03_CLK, 0x10);
    writeReg(ES8311_REG04_CLK, 0x10);
    writeReg(ES8311_REG05_CLK, 0x00);

    uint8_t reg06 = readReg(ES8311_REG06_CLK);
    Serial.printf("[ES8311] REG06 read: 0x%02X\n", reg06);
    reg06 &= 0xE0;
    reg06 |= 0x03;
    writeReg(ES8311_REG06_CLK, reg06);

    uint8_t reg07 = readReg(ES8311_REG07_CLK);
    Serial.printf("[ES8311] REG07 read: 0x%02X\n", reg07);
    reg07 &= 0xC0;
    writeReg(ES8311_REG07_CLK, reg07);

    writeReg(ES8311_REG08_CLK, 0xFF);

    // I2S format: slave mode (BIT6=0 of REG00), 16-bit
    uint8_t reg00 = readReg(ES8311_REG00_RESET);
    Serial.printf("[ES8311] REG00 read: 0x%02X\n", reg00);
    reg00 &= 0xBF;
    writeReg(ES8311_REG00_RESET, reg00);

    // SDP_IN: 16-bit word length (3 << 2 = 0x0C)
    writeReg(ES8311_REG09_SDPIN,  0x0C);
    // SDP_OUT: 16-bit word length
    writeReg(ES8311_REG0A_SDPOUT, 0x0C);

    // Power up DAC path (from Waveshare BSP sequence)
    writeReg(ES8311_REG0D_SYSTEM, 0x01);
    writeReg(ES8311_REG0E_SYSTEM, 0x02);
    writeReg(ES8311_REG12_SYSTEM, 0x00);
    writeReg(ES8311_REG13_SYSTEM, 0x10);
    writeReg(ES8311_REG14_SYSTEM, 0x1A);
    writeReg(ES8311_REG1C_ADC,    0x6A);

    // Unmute DAC
    writeReg(ES8311_REG31_DAC,    0x00);

    // DAC volume
    uint8_t vol = ((currentVolume * 256) / 100) - 1;
    writeReg(ES8311_REG32_DAC, vol);

    // Bypass DAC equalizer
    writeReg(ES8311_REG37_DAC,    0x08);

    Serial.println("[ES8311] Register verification:");
    for (uint8_t r = 0x00; r < 0x38; r++) {
        if (r == 0x0B || r == 0x0F || r == 0x10 || r == 0x11 ||
            r == 0x15 || r == 0x16 || r == 0x17 || r == 0x18 ||
            r == 0x19 || r == 0x1A || r == 0x1B) continue;
        Serial.printf("  REG[%02X] = %02X\n", r, readReg(r));
    }

    codecReady = true;
    Serial.println("[ES8311] Codec initialised (BCLK-derived, slave, 16-bit)");
    return true;
}

bool ES8311Config::setVolume(uint8_t percent) {
    if (!codecReady) return false;
    currentVolume = percent > 100 ? 100 : percent;

    uint8_t vol = (currentVolume == 0) ? 0 : ((currentVolume * 256) / 100) - 1;
    writeReg(ES8311_REG32_DAC, vol);

    Serial.printf("[ES8311] Volume: %d%% => reg=0x%02X\n", currentVolume, vol);
    return true;
}

bool ES8311Config::setMute(bool mute) {
    if (!codecReady) return false;
    uint8_t reg = readReg(ES8311_REG31_DAC);
    if (mute) {
        reg |= 0x60;
    } else {
        reg &= ~0x60;
    }
    writeReg(ES8311_REG31_DAC, reg);
    Serial.printf("[ES8311] Mute %s (reg=0x%02X)\n", mute ? "ON" : "OFF", reg);
    return true;
}

bool ES8311Config::enablePA(int paPin) {
    if (paPin >= 0) {
        pinMode(paPin, OUTPUT);
        digitalWrite(paPin, HIGH);
        Serial.println("[ES8311] PA enabled (GPIO7 HIGH)");
        return true;
    }
    return false;
}

bool ES8311Config::disablePA(int paPin) {
    if (paPin >= 0) {
        pinMode(paPin, OUTPUT);
        digitalWrite(paPin, LOW);
        Serial.println("[ES8311] PA disabled (GPIO7 LOW)");
        return true;
    }
    return false;
}

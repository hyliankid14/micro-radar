#pragma once

#include <Wire.h>

namespace TouchManager
{
#ifdef CROWPANEL_7
    struct TouchPoint {
        int x;
        int y;
        bool valid;
    };

    static uint8_t address = 0;
    static TouchPoint lastTouch = {0, 0, false};

    static bool writeRegister(uint8_t device, uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(device);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    static bool probe(uint8_t device)
    {
        Wire.beginTransmission(device);
        return Wire.endTransmission() == 0;
    }

    static void Initialise()
    {
        Wire.begin(19, 20);
        if (probe(0x18)) {
            bool ok = writeRegister(0x18, 0x01, 0xfc) &&
                      writeRegister(0x18, 0x03, 0xfc);
            delay(20);
            ok = writeRegister(0x18, 0x01, 0xfd) && ok;
            delay(100);
            ok = writeRegister(0x18, 0x03, 0xfe) && ok;
            Serial.println(ok ? "[TOUCH] V3 reset complete" : "[TOUCH] V3 reset FAILED");
        }
        if (probe(0x5d)) address = 0x5d;
        else if (probe(0x14)) address = 0x14;
        if (address) Serial.printf("[TOUCH] GT911 found at 0x%02x\n", address);
        else Serial.println("[TOUCH] GT911 not found");
    }

    static TouchPoint ConsumeTouch()
    {
        if (!address) return {0, 0, false};
        Wire.beginTransmission(address);
        Wire.write(0x81);
        Wire.write(0x4e);
        if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address, (uint8_t)1) != 1) {
            Serial.println("[TOUCH] GT911 status read failed");
            return {0, 0, false};
        }
        uint8_t status = Wire.read();
        if (!(status & 0x80)) return lastTouch;
        int count = status & 0x0f;
        TouchPoint point = {0, 0, false};
        if (count > 0 && count <= 5) {
            Wire.beginTransmission(address);
            Wire.write(0x81);
            Wire.write(0x4f);
            if (Wire.endTransmission(false) == 0 && Wire.requestFrom(address, (uint8_t)5) == 5) {
                Wire.read();
                int x = Wire.read();
                x |= Wire.read() << 8;
                int y = Wire.read();
                y |= Wire.read() << 8;
                if (x >= 0 && x < 800 && y >= 0 && y < 480) {
                    point = {x, y, true};
                }
            } else {
                Serial.println("[TOUCH] GT911 point read failed");
            }
        }
        Wire.beginTransmission(address);
        Wire.write(0x81);
        Wire.write(0x4e);
        Wire.write(0);
        if (Wire.endTransmission() != 0) Serial.println("[TOUCH] GT911 status clear failed");
        lastTouch = point;
        return point;
    }
#else
    constexpr uint8_t CST816_ADDR = 0x15;
    constexpr int I2C_SDA = 42;
    constexpr int I2C_SCL = 41;
    constexpr int TOUCH_RST = 47;

    struct TouchPoint {
        int x;
        int y;
        bool valid;
    };

    static TouchPoint lastTouch = {0, 0, false};
    static bool deviceFound = false;

    static void Initialise()
    {
        pinMode(TOUCH_RST, OUTPUT);
        digitalWrite(TOUCH_RST, LOW);
        delay(10);
        digitalWrite(TOUCH_RST, HIGH);
        delay(200);

        Wire.begin(I2C_SDA, I2C_SCL);

        Wire.beginTransmission(CST816_ADDR);
        if (Wire.endTransmission() == 0) {
            Serial.println("[TOUCH] CST816 found at 0x15");
            deviceFound = true;
        } else {
            Serial.println("[TOUCH] CST816 not found at 0x15");
            deviceFound = false;
        }
    }

    static TouchPoint Read()
    {
        if (!deviceFound) {
            lastTouch.valid = false;
            return lastTouch;
        }

        Wire.beginTransmission(CST816_ADDR);
        Wire.write(0x02);
        Wire.endTransmission(false);
        Wire.requestFrom(CST816_ADDR, (uint8_t)5);

        if (Wire.available() >= 5) {
            uint8_t numFingers = Wire.read();
            uint8_t xh = Wire.read();
            uint8_t xl = Wire.read();
            uint8_t yh = Wire.read();
            uint8_t yl = Wire.read();

            if (numFingers > 0 && numFingers < 10) {
                int x = ((xh & 0x0F) << 8) | xl;
                int y = ((yh & 0x0F) << 8) | yl;
                if (x >= 0 && x <= 240 && y >= 0 && y <= 240) {
                    lastTouch.x = x;
                    lastTouch.y = y;
                    lastTouch.valid = true;
                } else {
                    lastTouch.valid = false;
                }
            } else {
                lastTouch.valid = false;
            }
        } else {
            lastTouch.valid = false;
        }

        return lastTouch;
    }

    static TouchPoint ConsumeTouch()
    {
        Read();
        TouchPoint t = lastTouch;
        lastTouch.valid = false;
        return t;
    }

    static bool HasTouch()
    {
        return lastTouch.valid;
    }
#endif
}

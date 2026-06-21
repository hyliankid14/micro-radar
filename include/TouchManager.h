#pragma once

#include <Wire.h>

namespace TouchManager
{
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
}

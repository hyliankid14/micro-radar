#pragma once

#include <Arduino.h>
#include <cmath>

// PWM backlight with gamma correction and smooth fades.
// Level is a perceptual percentage 0..100; 0 = fully off.
namespace Backlight
{
    constexpr int PIN = 2;
    constexpr int LEDC_CHANNEL = 7;
    constexpr int LEDC_FREQ_HZ = 5000;
    constexpr int LEDC_BITS = 8;
    constexpr int MIN_USER_PCT = 5;      // slider floor: the screen is never accidentally black
    constexpr float FADE_PCT_PER_TICK = 4.0f;

    inline float current = 0.0f;
    inline float target = 0.0f;
    inline int lastDuty = -1;

    inline void WriteDuty(float pct)
    {
        if (pct < 0.0f) pct = 0.0f;
        if (pct > 100.0f) pct = 100.0f;
        int duty = 0;
        if (pct > 0.5f) {
            duty = static_cast<int>(std::lround(255.0f * std::pow(pct / 100.0f, 2.2f)));
            if (duty < 2) duty = 2;
        }
        if (duty == lastDuty) return;
        lastDuty = duty;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(PIN, duty);
#else
        ledcWrite(LEDC_CHANNEL, duty);
#endif
    }

    inline void Begin(float pct)
    {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(PIN, LEDC_FREQ_HZ, LEDC_BITS);
#else
        ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_BITS);
        ledcAttachPin(PIN, LEDC_CHANNEL);
#endif
        current = target = pct;
        lastDuty = -1;
        WriteDuty(current);
    }

    inline void SetTarget(float pct) { target = pct; }
    inline void SetImmediate(float pct) { current = target = pct; WriteDuty(current); }
    inline float Target() { return target; }
    inline bool IsOff() { return current <= 0.5f && target <= 0.5f; }

    // Call once per loop iteration; fades ~300 ms end-to-end
    inline void Tick()
    {
        if (current == target) return;
        if (current < target) current = std::min(target, current + FADE_PCT_PER_TICK);
        else current = std::max(target, current - FADE_PCT_PER_TICK);
        WriteDuty(current);
    }
}

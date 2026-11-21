#pragma once

#include "LGFX.h"

inline void DrawScanLines(LGFX_Sprite& buf, const int x0, const int y0, const int x1, const int y1, const int thickness, const int trailBrightness, const int spacing, const int radius, uint8_t baseR = 0, uint8_t baseG = 200, uint8_t baseB = 0)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;

    // perpendicular unit vector
    float px = -dy / len;
    float py = dx / len;

    auto clampToRadius = [&](float ex, float ey, int &outX, int &outY) {
        float ddx = ex - x0;
        float ddy = ey - y0;
        float dist = sqrt(ddx * ddx + ddy * ddy);
        if (dist > radius) {
            float scale = (float)radius / dist;
            outX = x0 + (int)(ddx * scale);
            outY = y0 + (int)(ddy * scale);
        } else {
            outX = (int)ex;
            outY = (int)ey;
        }
    };

    for (int i = 0; i <= thickness; i++) {
        float t = i / (float)(thickness);
        float factor = (t * trailBrightness) / 200.0f;
        if (factor > 1.0f) factor = 1.0f;
        uint8_t r = (uint8_t)(baseR * factor);
        uint8_t g = (uint8_t)(baseG * factor);
        uint8_t b = (uint8_t)(baseB * factor);

        float ex = x1 + (px * (i * spacing));
        float ey = y1 + (py * (i * spacing));
        int clampedX, clampedY;
        clampToRadius(ex, ey, clampedX, clampedY);

        buf.drawLine(
            x0, y0,
            clampedX, clampedY,
            lgfx::color888(r, g, b)
        );
    }

    {
        float ex = x1 + (px * (thickness * spacing));
        float ey = y1 + (py * (thickness * spacing));
        int clampedX, clampedY;
        clampToRadius(ex, ey, clampedX, clampedY);

        buf.drawLine(
            x0, y0,
            clampedX, clampedY,
            lgfx::color888(baseR, baseG, baseB)
        );
    }
}
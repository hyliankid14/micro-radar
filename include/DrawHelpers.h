#pragma once

#include "LGFX.h"

void DrawScanLines(LGFX_Sprite& buf, const int x0, const int y0, const int x1, const int y1, const int thickness, const int trailBrightness, const int spacing, const int radius)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrt(dx * dx + dy * dy);

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
        uint8_t brightness = (uint8_t)(t * trailBrightness);

        float ex = x1 + (px * (i * spacing));
        float ey = y1 + (py * (i * spacing));
        int clampedX, clampedY;
        clampToRadius(ex, ey, clampedX, clampedY);

        buf.drawLine(
            x0, y0,
            clampedX, clampedY,
            lgfx::color888(0, brightness, 0)
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
            lgfx::color888(0, 200, 0)
        );
    }
}
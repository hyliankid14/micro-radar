#pragma once

#include <Arduino.h>
#include <cmath>
#include "LGFX.h"
#include "ThemePalette.h"
#include "AviationCatalog.h"

namespace VectorArt
{
    inline void DrawCarrierLogoBadge(LGFX_Sprite& buf, int x, int y, int w, int h, const AirlineInfo& airline, const ThemePalette& theme)
    {
        // Card base in carrier primary livery color
        buf.fillRoundRect(x, y, w, h, 5, airline.primaryColor);

        // Stylized tail-fin / dynamic livery diagonal accent stripe on right side
        int stripeW = max(8, w / 3);
        buf.fillTriangle(
            x + w - stripeW, y + h - 1,
            x + w - 1, y + 3,
            x + w - 1, y + h - 1,
            airline.accentColor
        );
        buf.fillTriangle(
            x + w - stripeW - 5, y + h - 1,
            x + w - 4, y + 1,
            x + w - stripeW, y + h - 1,
            airline.accentColor
        );

        // Subtle border
        buf.drawRoundRect(x, y, w, h, 5, theme.borderPrimary);

        // Bold IATA or ICAO code
        String badgeCode = !airline.iata.isEmpty() ? airline.iata : airline.icao;
        if (badgeCode.isEmpty()) badgeCode = "AIR";

        buf.setTextColor(airline.textColor);
        if (h >= 34 && badgeCode.length() <= 3) {
            UseFont(buf, 4);
            buf.setTextSize(1);
            int ty = y + (h - buf.fontHeight()) / 2 + 1;
            buf.drawString(badgeCode, x + 8, ty);
            UseFont(buf, 1);
        } else {
            UseFont(buf, 2);
            buf.setTextSize(1);
            int ty = y + (h - buf.fontHeight()) / 2;
            buf.drawString(badgeCode, x + 7, ty);
            UseFont(buf, 1);
        }
    }

    // Draws a top-down / technical profile aircraft silhouette inside [x, y, w, h]
    inline void DrawAircraftSilhouette(
        LGFX_Sprite& buf,
        int x, int y, int w, int h,
        SilhouetteCategory cat,
        const ThemePalette& theme,
        const String& typeCode,
        const String& typeName,
        const String& reg,
        bool showAnnotations = true)
    {
        // Background frame + subtle loading shimmer band across placeholder
        buf.fillRoundRect(x, y, w, h, 6, theme.bgSecondary);
        {
            int bandW = max(24, w / 5);
            int period = w + bandW * 2;
            int sweepX = x - bandW + static_cast<int>((millis() / 16) % max(1, period));
            int sx0 = max(x + 2, sweepX);
            int sx1 = min(x + w - 2, sweepX + bandW);
            if (sx1 > sx0) {
                buf.fillRect(sx0, y + 2, sx1 - sx0, h - 4, theme.bgRowAlt);
            }
        }

        // Subtle technical gridlines
        int gridStep = 20;
        for (int gx = x + gridStep; gx < x + w - 4; gx += gridStep) {
            buf.drawFastVLine(gx, y + 2, h - 4, theme.gridLine);
        }
        for (int gy = y + gridStep; gy < y + h - 4; gy += gridStep) {
            buf.drawFastHLine(x + 2, gy, w - 4, theme.gridLine);
        }
        buf.drawRoundRect(x, y, w, h, 6, theme.borderSecondary);

        // Corner technical crosshair ticks
        buf.drawFastHLine(x + 5, y + 5, 8, theme.textMuted);
        buf.drawFastVLine(x + 5, y + 5, 8, theme.textMuted);
        buf.drawFastHLine(x + w - 13, y + 5, 8, theme.textMuted);
        buf.drawFastVLine(x + w - 6, y + 5, 8, theme.textMuted);
        buf.drawFastHLine(x + 5, y + h - 6, 8, theme.textMuted);
        buf.drawFastVLine(x + 5, y + h - 13, 8, theme.textMuted);
        buf.drawFastHLine(x + w - 13, y + h - 6, 8, theme.textMuted);
        buf.drawFastVLine(x + w - 6, y + h - 13, 8, theme.textMuted);

        const int cx = x + w / 2;
        const int cy = y + h / 2 - (showAnnotations ? 4 : 0);
        const float scale = min(w / 220.0f, h / 115.0f);
        auto S = [scale](float v) -> int { return static_cast<int>(std::round(v * scale)); };

        uint32_t bodyFill = theme.isLight ? lgfx::color888(58, 76, 108) : theme.textSecondary;
        uint32_t wingFill = theme.isLight ? lgfx::color888(84, 104, 138) : theme.borderPrimary;
        uint32_t accentCol = theme.textAccent;

        // Center reference axis line
        buf.drawFastHLine(cx - S(85), cy, S(170), theme.borderSecondary);
        buf.drawFastVLine(cx, cy - S(42), S(84), theme.borderSecondary);

        switch (cat) {
            case SIL_WIDEBODY_QUAD: {
                // 4-Engine Superjumbo (A380 / B747) planform silhouette facing right
                // Swept main wings
                buf.fillTriangle(cx + S(12), cy - S(7), cx - S(22), cy - S(46), cx - S(8), cy - S(7), wingFill);
                buf.fillTriangle(cx + S(12), cy - S(7), cx - S(22), cy - S(46), cx - S(12), cy - S(46), wingFill);
                buf.fillTriangle(cx + S(12), cy + S(7), cx - S(22), cy + S(46), cx - S(8), cy + S(7), wingFill);
                buf.fillTriangle(cx + S(12), cy + S(7), cx - S(22), cy + S(46), cx - S(12), cy + S(46), wingFill);

                // 4 engine nacelles
                buf.fillRoundRect(cx - S(1),  cy - S(22), S(12), S(5), 2, accentCol);
                buf.fillRoundRect(cx - S(10), cy - S(36), S(11), S(5), 2, accentCol);
                buf.fillRoundRect(cx - S(1),  cy + S(17), S(12), S(5), 2, accentCol);
                buf.fillRoundRect(cx - S(10), cy + S(31), S(11), S(5), 2, accentCol);

                // Tail stabilizers
                buf.fillTriangle(cx - S(48), cy - S(3), cx - S(68), cy - S(20), cx - S(60), cy - S(3), wingFill);
                buf.fillTriangle(cx - S(48), cy + S(3), cx - S(68), cy + S(20), cx - S(60), cy + S(3), wingFill);

                // Wide fuselage + nose cone
                buf.fillRoundRect(cx - S(60), cy - S(8), S(114), S(17), S(8), bodyFill);
                buf.fillCircle(cx + S(52), cy, S(7), bodyFill);
                // Cockpit windshield arc
                buf.fillRoundRect(cx + S(46), cy - S(4), S(6), S(9), 2, theme.bgCard);
                break;
            }

            case SIL_WIDEBODY_TWIN: {
                // Twin-aisle Widebody (A350 / B787 / B777)
                buf.fillTriangle(cx + S(14), cy - S(6), cx - S(20), cy - S(45), cx - S(10), cy - S(6), wingFill);
                buf.fillTriangle(cx + S(14), cy + S(6), cx - S(20), cy + S(45), cx - S(10), cy + S(6), wingFill);
                // Raked wingtips
                buf.drawLine(cx - S(20), cy - S(45), cx - S(26), cy - S(47), accentCol);
                buf.drawLine(cx - S(20), cy + S(45), cx - S(26), cy + S(47), accentCol);

                // 2 large high-bypass turbofan nacelles
                buf.fillRoundRect(cx - S(1), cy - S(24), S(14), S(6), 2, accentCol);
                buf.fillRoundRect(cx - S(1), cy + S(18), S(14), S(6), 2, accentCol);

                // Tailplanes
                buf.fillTriangle(cx - S(46), cy - S(3), cx - S(66), cy - S(19), cx - S(58), cy - S(3), wingFill);
                buf.fillTriangle(cx - S(46), cy + S(3), cx - S(66), cy + S(19), cx - S(58), cy + S(3), wingFill);

                // Fuselage
                buf.fillRoundRect(cx - S(58), cy - S(6), S(112), S(13), S(6), bodyFill);
                buf.fillTriangle(cx + S(50), cy - S(6), cx + S(64), cy, cx + S(50), cy + S(6), bodyFill);
                buf.fillRoundRect(cx + S(46), cy - S(3), S(5), S(7), 1, theme.bgCard);
                break;
            }

            case SIL_REGIONAL_PROP: {
                // Straight-wing Twin Turboprop (ATR 72 / Dash 8 Q400)
                buf.fillRoundRect(cx - S(6), cy - S(42), S(14), S(84), 3, wingFill);
                // Twin turboprop nacelles + spinning prop discs
                buf.fillRoundRect(cx + S(2), cy - S(22), S(14), S(5), 2, bodyFill);
                buf.fillRoundRect(cx + S(2), cy + S(17), S(14), S(5), 2, bodyFill);
                buf.drawFastVLine(cx + S(18), cy - S(27), S(15), accentCol);
                buf.drawFastVLine(cx + S(18), cy + S(12), S(15), accentCol);

                // T-tail stabilizer
                buf.fillRoundRect(cx - S(56), cy - S(16), S(10), S(32), 2, wingFill);
                // Fuselage
                buf.fillRoundRect(cx - S(54), cy - S(5), S(100), S(11), S(5), bodyFill);
                buf.fillRoundRect(cx + S(38), cy - S(3), S(4), S(7), 1, theme.bgCard);
                break;
            }

            case SIL_BIZJET: {
                // Sleek rear-engine Business Jet (Gulfstream / Global / Citation)
                buf.fillTriangle(cx + S(6), cy - S(4), cx - S(24), cy - S(36), cx - S(14), cy - S(4), wingFill);
                buf.fillTriangle(cx + S(6), cy + S(4), cx - S(24), cy + S(36), cx - S(14), cy + S(4), wingFill);
                // Rear fuselage engine pods
                buf.fillRoundRect(cx - S(38), cy - S(10), S(14), S(4), 1, accentCol);
                buf.fillRoundRect(cx - S(38), cy + S(6),  S(14), S(4), 1, accentCol);
                // Swept T-tail
                buf.fillTriangle(cx - S(44), cy - S(2), cx - S(58), cy - S(15), cx - S(52), cy - S(2), wingFill);
                buf.fillTriangle(cx - S(44), cy + S(2), cx - S(58), cy + S(15), cx - S(52), cy + S(2), wingFill);
                // Slender fuselage + needle nose
                buf.fillRoundRect(cx - S(50), cy - S(4), S(92), S(9), S(4), bodyFill);
                buf.fillTriangle(cx + S(38), cy - S(4), cx + S(54), cy, cx + S(38), cy + S(4), bodyFill);
                buf.fillRoundRect(cx + S(34), cy - S(2), S(5), S(5), 1, theme.bgCard);
                break;
            }

            case SIL_GA_LIGHT: {
                // Single-engine Light Propeller Aircraft (Cessna 172 / Cirrus / PC-12)
                buf.fillRoundRect(cx - S(4), cy - S(38), S(14), S(76), 2, wingFill);
                buf.fillRoundRect(cx - S(42), cy - S(14), S(9), S(28), 2, wingFill);
                buf.fillRoundRect(cx - S(40), cy - S(4), S(76), S(9), S(4), bodyFill);
                // Nose spinner & propeller arc
                buf.fillTriangle(cx + S(34), cy - S(3), cx + S(42), cy, cx + S(34), cy + S(3), accentCol);
                buf.drawFastVLine(cx + S(44), cy - S(14), S(28), accentCol);
                buf.fillRoundRect(cx + S(14), cy - S(3), S(8), S(7), 1, theme.bgCard);
                break;
            }

            case SIL_HELICOPTER: {
                // Rotorcraft / Helicopter top-down silhouette
                // Tail boom & tail rotor
                buf.fillRect(cx - S(54), cy - S(2), S(38), S(5), bodyFill);
                buf.drawFastVLine(cx - S(54), cy - S(10), S(10), accentCol);
                buf.fillRect(cx - S(38), cy - S(8), S(4), S(16), wingFill);
                // Landing skids
                buf.drawFastHLine(cx - S(12), cy - S(13), S(28), wingFill);
                buf.drawFastHLine(cx - S(12), cy + S(13), S(28), wingFill);
                // Cabin pod
                buf.fillRoundRect(cx - S(18), cy - S(9), S(44), S(19), S(8), bodyFill);
                buf.fillRoundRect(cx + S(14), cy - S(6), S(8), S(13), 3, theme.bgCard);
                // Main rotor disc & 4 blades
                buf.drawCircle(cx + S(2), cy, S(42), accentCol);
                buf.drawLine(cx + S(2) - S(30), cy - S(30), cx + S(2) + S(30), cy + S(30), accentCol);
                buf.drawLine(cx + S(2) - S(30), cy + S(30), cx + S(2) + S(30), cy - S(30), accentCol);
                buf.fillCircle(cx + S(2), cy, S(4), accentCol);
                break;
            }

            case SIL_MILITARY: {
                // Delta-wing / tactical military aircraft
                buf.fillTriangle(cx + S(18), cy, cx - S(34), cy - S(40), cx - S(28), cy, wingFill);
                buf.fillTriangle(cx + S(18), cy, cx - S(34), cy + S(40), cx - S(28), cy, wingFill);
                // Canards / twin tails
                buf.fillTriangle(cx + S(30), cy, cx + S(16), cy - S(14), cx + S(16), cy, accentCol);
                buf.fillTriangle(cx + S(30), cy, cx + S(16), cy + S(14), cx + S(16), cy, accentCol);
                // Fuselage & nose cone
                buf.fillRoundRect(cx - S(46), cy - S(5), S(86), S(11), S(4), bodyFill);
                buf.fillTriangle(cx + S(38), cy - S(5), cx + S(58), cy, cx + S(38), cy + S(5), bodyFill);
                buf.fillRoundRect(cx + S(24), cy - S(2), S(10), S(5), 2, theme.bgCard);
                break;
            }

            case SIL_NARROWBODY:
            default: {
                // Standard Narrowbody Airliner (A320 / B737)
                buf.fillTriangle(cx + S(10), cy - S(5), cx - S(18), cy - S(40), cx - S(8), cy - S(5), wingFill);
                buf.fillTriangle(cx + S(10), cy + S(5), cx - S(18), cy + S(40), cx - S(8), cy + S(5), wingFill);
                // Sharklets / winglets
                buf.fillRect(cx - S(21), cy - S(41), S(5), S(2), accentCol);
                buf.fillRect(cx - S(21), cy + S(39), S(5), S(2), accentCol);

                // Twin underwing turbofans
                buf.fillRoundRect(cx - S(1), cy - S(20), S(12), S(5), 2, accentCol);
                buf.fillRoundRect(cx - S(1), cy + S(15), S(12), S(5), 2, accentCol);

                // Horizontal stabilizer
                buf.fillTriangle(cx - S(42), cy - S(3), cx - S(58), cy - S(17), cx - S(50), cy - S(3), wingFill);
                buf.fillTriangle(cx - S(42), cy + S(3), cx - S(58), cy + S(17), cx - S(50), cy + S(3), wingFill);

                // Fuselage
                buf.fillRoundRect(cx - S(52), cy - S(5), S(98), S(11), S(5), bodyFill);
                buf.fillTriangle(cx + S(42), cy - S(5), cx + S(54), cy, cx + S(42), cy + S(5), bodyFill);
                buf.fillRoundRect(cx + S(38), cy - S(3), S(5), S(7), 1, theme.bgCard);
                break;
            }
        }

        if (showAnnotations) {
            UseFont(buf, 1);
            buf.setTextSize(1);
            buf.setTextColor(theme.textMuted);
            String topLeft = !typeCode.isEmpty() ? ("TYPE // " + typeCode) : "SPEC // AIRCRAFT";
            buf.drawString(topLeft, x + 8, y + 6);
            if (!reg.isEmpty()) {
                buf.setTextColor(theme.textAccent);
                buf.drawRightString("REG " + reg, x + w - 8, y + 6);
            }
            if (!typeName.isEmpty()) {
                buf.setTextColor(theme.textPrimary);
                buf.drawCentreString(typeName, cx, y + h - 14);
            }
        }
    }

    inline void DrawRouteProgressBar(LGFX_Sprite& buf, int x, int y, int w, float progressPct, const ThemePalette& theme)
    {
        const int barH = 4;
        const int cy = y + 6;

        // Track background
        buf.fillRoundRect(x, cy - barH / 2, w, barH, 2, theme.bgProgressTrack);

        float clampedPct = (progressPct >= 0.0f) ? constrain(progressPct, 4.0f, 96.0f) : 50.0f;
        int planeX = x + static_cast<int>((clampedPct / 100.0f) * w);

        // Completed segment
        if (progressPct >= 0.0f) {
            buf.fillRoundRect(x, cy - barH / 2, max(4, planeX - x), barH, 2, theme.routeLine);
        } else {
            // Dashed center line when progress % isn't known yet
            for (int dx = x + 8; dx < x + w - 8; dx += 12) {
                buf.fillRect(dx, cy - 1, 6, 2, theme.borderSecondary);
            }
        }

        // Origin & Destination airport terminal dots
        buf.fillCircle(x + 3, cy, 4, theme.routeLine);
        buf.fillCircle(x + w - 3, cy, 4, theme.borderPrimary);

        // Little vector airplane icon at planeX, cy
        buf.fillTriangle(planeX + 7, cy, planeX - 5, cy - 6, planeX - 2, cy, theme.textAccent);
        buf.fillTriangle(planeX + 7, cy, planeX - 5, cy + 6, planeX - 2, cy, theme.textAccent);
        buf.fillRect(planeX - 6, cy - 1, 11, 3, theme.textPrimary);
    }

    inline void DrawMiniCompassRadar(
        LGFX_Sprite& buf,
        int cx, int cy, int r,
        float bearingDeg, float distKm, float maxDistKm, float trackDeg,
        const ThemePalette& theme)
    {
        buf.fillCircle(cx, cy, r, theme.bgSecondary);
        buf.drawCircle(cx, cy, r, theme.radarRingOuter);
        buf.drawCircle(cx, cy, r / 2, theme.radarRingInner);
        buf.drawFastHLine(cx - r + 3, cy, (r - 3) * 2, theme.gridLine);
        buf.drawFastVLine(cx, cy - r + 3, (r - 3) * 2, theme.gridLine);

        // Home dot in center
        buf.fillCircle(cx, cy, 2, theme.textMuted);

        // Plot aircraft position on mini compass
        float normDist = (maxDistKm > 0.1f) ? constrain(distKm / maxDistKm, 0.1f, 0.88f) : 0.5f;
        float radBrg = radians(bearingDeg);
        int ax = cx + static_cast<int>(std::sin(radBrg) * normDist * r);
        int ay = cy - static_cast<int>(std::cos(radBrg) * normDist * r);

        // Heading vector triangle
        float dx = std::sin(radians(trackDeg));
        float dy = -std::cos(radians(trackDeg));
        float px = -dy;
        float py = dx;
        buf.fillTriangle(
            ax + dx * 6, ay + dy * 6,
            ax - dx * 4 + px * 3, ay - dy * 4 + py * 3,
            ax - dx * 4 - px * 3, ay - dy * 4 - py * 3,
            theme.aircraftSelected
        );
        buf.drawCircle(ax, ay, 8, theme.textAccent);
    }
}

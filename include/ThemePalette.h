#pragma once

#include <Arduino.h>
#include "LGFX.h"

enum DisplayViewMode {
    VIEW_RADAR_SPLIT = 0,
    VIEW_DEPARTURE_BOARD = 1,
    VIEW_PORTRAIT_POSTER = 2,
    VIEW_COUNT = 3
};

enum ThemeId {
    THEME_GALLERY = 0,
    THEME_COCKPIT = 1,
    THEME_SOLARI = 2,
    THEME_BLUEPRINT = 3,
    THEME_RADAR_CRT = 4,
    THEME_COUNT = 5
};

struct ThemePalette {
    ThemeId id;
    const char* key;
    const char* displayName;
    bool isLight;

    uint32_t bgPrimary;
    uint32_t bgSecondary;
    uint32_t bgCard;
    uint32_t bgHeader;
    uint32_t bgRowAlt;
    uint32_t bgProgressTrack;

    uint32_t borderPrimary;
    uint32_t borderSecondary;
    uint32_t gridLine;

    uint32_t textPrimary;
    uint32_t textSecondary;
    uint32_t textMuted;
    uint32_t textAccent;
    uint32_t textWarning;
    uint32_t textPinned;

    uint32_t radarRingOuter;
    uint32_t radarRingInner;
    uint32_t radarSweepLead;
    uint32_t radarSweepR;
    uint32_t radarSweepG;
    uint32_t radarSweepB;

    uint32_t aircraftDot;
    uint32_t aircraftSelected;
    uint32_t aircraftTrail;
    uint32_t routeLine;

    uint32_t buttonBg;
    uint32_t buttonBorder;
    uint32_t buttonText;
};

inline const ThemePalette& GetThemePalette(ThemeId id)
{
    static const ThemePalette PALETTES[THEME_COUNT] = {
        // 0: AvioFrame Gallery Light (Warm Paper / Living-Room Art Frame)
        {
            THEME_GALLERY, "gallery", "Gallery Light", true,
            lgfx::color888(244, 240, 232), // bgPrimary (warm paper)
            lgfx::color888(234, 228, 217), // bgSecondary
            lgfx::color888(252, 250, 246), // bgCard
            lgfx::color888(26, 36, 54),    // bgHeader (deep aviation navy)
            lgfx::color888(239, 234, 224), // bgRowAlt
            lgfx::color888(216, 210, 198), // bgProgressTrack
            lgfx::color888(42, 54, 75),    // borderPrimary
            lgfx::color888(188, 180, 166), // borderSecondary
            lgfx::color888(214, 208, 196), // gridLine
            lgfx::color888(24, 30, 42),    // textPrimary (charcoal ink)
            lgfx::color888(68, 82, 104),   // textSecondary
            lgfx::color888(120, 126, 136), // textMuted
            lgfx::color888(198, 48, 44),   // textAccent (aviation vermilion red)
            lgfx::color888(196, 118, 16),  // textWarning
            lgfx::color888(198, 48, 44),   // textPinned
            lgfx::color888(52, 68, 94),    // radarRingOuter
            lgfx::color888(178, 172, 160), // radarRingInner
            lgfx::color888(198, 48, 44),   // radarSweepLead
            198, 60, 56,                   // radarSweep RGB
            lgfx::color888(28, 52, 92),    // aircraftDot
            lgfx::color888(198, 48, 44),   // aircraftSelected
            lgfx::color888(140, 152, 172), // aircraftTrail
            lgfx::color888(198, 48, 44),   // routeLine
            lgfx::color888(232, 226, 215), // buttonBg
            lgfx::color888(68, 82, 104),   // buttonBorder
            lgfx::color888(24, 30, 42)     // buttonText
        },
        // 1: Midnight Cockpit (Modern Boeing/Airbus EFIS Avionics Dark)
        {
            THEME_COCKPIT, "cockpit", "Midnight Cockpit", false,
            lgfx::color888(11, 17, 28),    // bgPrimary
            lgfx::color888(16, 25, 40),    // bgSecondary
            lgfx::color888(18, 28, 46),    // bgCard
            lgfx::color888(24, 38, 64),    // bgHeader
            lgfx::color888(15, 23, 38),    // bgRowAlt
            lgfx::color888(34, 50, 76),    // bgProgressTrack
            lgfx::color888(0, 195, 255),   // borderPrimary (EFIS cyan)
            lgfx::color888(38, 64, 98),    // borderSecondary
            lgfx::color888(26, 44, 68),    // gridLine
            lgfx::color888(235, 245, 255), // textPrimary
            lgfx::color888(110, 210, 255), // textSecondary
            lgfx::color888(105, 130, 160), // textMuted
            lgfx::color888(0, 230, 255),   // textAccent (EFIS cyan)
            lgfx::color888(255, 185, 0),   // textWarning (avionics amber)
            lgfx::color888(255, 70, 210),  // textPinned (FMC magenta)
            lgfx::color888(0, 185, 240),   // radarRingOuter
            lgfx::color888(28, 64, 96),    // radarRingInner
            lgfx::color888(0, 225, 255),   // radarSweepLead
            0, 175, 230,                   // radarSweep RGB
            lgfx::color888(0, 235, 255),   // aircraftDot
            lgfx::color888(255, 80, 220),  // aircraftSelected (magenta)
            lgfx::color888(0, 115, 155),   // aircraftTrail
            lgfx::color888(255, 80, 220),  // routeLine
            lgfx::color888(20, 34, 54),    // buttonBg
            lgfx::color888(0, 175, 230),   // buttonBorder
            lgfx::color888(210, 242, 255)  // buttonText
        },
        // 2: Split-Flap Solari Amber (Retro Airport Departure Board)
        {
            THEME_SOLARI, "solari", "Split-Flap Amber", false,
            lgfx::color888(18, 17, 15),    // bgPrimary
            lgfx::color888(26, 24, 21),    // bgSecondary
            lgfx::color888(28, 26, 22),    // bgCard
            lgfx::color888(42, 36, 26),    // bgHeader
            lgfx::color888(34, 31, 27),    // bgRowAlt
            lgfx::color888(54, 48, 38),    // bgProgressTrack
            lgfx::color888(245, 178, 32),  // borderPrimary (warm amber)
            lgfx::color888(76, 66, 48),    // borderSecondary
            lgfx::color888(48, 42, 32),    // gridLine
            lgfx::color888(255, 206, 72),  // textPrimary (Solari gold-amber)
            lgfx::color888(242, 234, 212), // textSecondary (flap white)
            lgfx::color888(146, 130, 98),  // textMuted
            lgfx::color888(255, 176, 24),  // textAccent
            lgfx::color888(255, 110, 40),  // textWarning
            lgfx::color888(110, 245, 130), // textPinned
            lgfx::color888(230, 165, 28),  // radarRingOuter
            lgfx::color888(82, 62, 24),    // radarRingInner
            lgfx::color888(255, 195, 45),  // radarSweepLead
            210, 145, 20,                  // radarSweep RGB
            lgfx::color888(255, 200, 50),  // aircraftDot
            lgfx::color888(255, 255, 255), // aircraftSelected
            lgfx::color888(140, 100, 24),  // aircraftTrail
            lgfx::color888(255, 185, 35),  // routeLine
            lgfx::color888(34, 30, 24),    // buttonBg
            lgfx::color888(215, 155, 28),  // buttonBorder
            lgfx::color888(255, 206, 72)   // buttonText
        },
        // 3: Technical Blueprint (Engineering Prussian Blue & Crisp White)
        {
            THEME_BLUEPRINT, "blueprint", "Blueprint", false,
            lgfx::color888(14, 46, 92),    // bgPrimary (Prussian blueprint blue)
            lgfx::color888(18, 56, 110),   // bgSecondary
            lgfx::color888(18, 58, 114),   // bgCard
            lgfx::color888(10, 34, 70),    // bgHeader
            lgfx::color888(22, 66, 126),   // bgRowAlt
            lgfx::color888(32, 82, 150),   // bgProgressTrack
            lgfx::color888(220, 238, 255), // borderPrimary
            lgfx::color888(66, 118, 186),  // borderSecondary
            lgfx::color888(34, 80, 142),   // gridLine
            lgfx::color888(245, 250, 255), // textPrimary
            lgfx::color888(170, 214, 255), // textSecondary
            lgfx::color888(108, 158, 214), // textMuted
            lgfx::color888(255, 220, 90),  // textAccent (drafting yellow)
            lgfx::color888(255, 195, 65),  // textWarning
            lgfx::color888(255, 225, 90),  // textPinned
            lgfx::color888(210, 232, 255), // radarRingOuter
            lgfx::color888(58, 110, 178),  // radarRingInner
            lgfx::color888(240, 248, 255), // radarSweepLead
            140, 195, 255,                 // radarSweep RGB
            lgfx::color888(255, 255, 255), // aircraftDot
            lgfx::color888(255, 220, 80),  // aircraftSelected
            lgfx::color888(105, 165, 230), // aircraftTrail
            lgfx::color888(255, 220, 80),  // routeLine
            lgfx::color888(22, 66, 126),   // buttonBg
            lgfx::color888(190, 222, 255), // buttonBorder
            lgfx::color888(245, 250, 255)  // buttonText
        },
        // 4: Classic Radar Green (Original Phosphor CRT)
        {
            THEME_RADAR_CRT, "radar", "Classic Radar", false,
            lgfx::color888(0, 6, 0),       // bgPrimary
            lgfx::color888(0, 14, 0),      // bgSecondary
            lgfx::color888(0, 18, 4),      // bgCard
            lgfx::color888(0, 34, 8),      // bgHeader
            lgfx::color888(0, 24, 6),      // bgRowAlt
            lgfx::color888(0, 52, 12),     // bgProgressTrack
            lgfx::color888(0, 220, 40),    // borderPrimary
            lgfx::color888(0, 95, 20),     // borderSecondary
            lgfx::color888(0, 45, 10),     // gridLine
            lgfx::color888(0, 255, 60),    // textPrimary
            lgfx::color888(0, 220, 170),   // textSecondary
            lgfx::color888(0, 145, 40),    // textMuted
            lgfx::color888(80, 255, 120),  // textAccent
            lgfx::color888(220, 220, 0),   // textWarning
            lgfx::color888(0, 255, 255),   // textPinned
            lgfx::color888(0, 200, 30),    // radarRingOuter
            lgfx::color888(0, 68, 12),     // radarRingInner
            lgfx::color888(0, 235, 40),    // radarSweepLead
            0, 180, 20,                    // radarSweep RGB
            lgfx::color888(0, 255, 40),    // aircraftDot
            lgfx::color888(0, 255, 255),   // aircraftSelected
            lgfx::color888(0, 115, 25),    // aircraftTrail
            lgfx::color888(0, 235, 180),   // routeLine
            lgfx::color888(0, 24, 4),      // buttonBg
            lgfx::color888(0, 190, 30),    // buttonBorder
            lgfx::color888(0, 255, 50)     // buttonText
        }
    };

    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= THEME_COUNT) idx = 0;
    return PALETTES[idx];
}

inline ThemeId ParseThemeId(const String& key)
{
    String k = key;
    k.toLowerCase();
    k.trim();
    if (k == "gallery")   return THEME_GALLERY;
    if (k == "cockpit")   return THEME_COCKPIT;
    if (k == "solari")    return THEME_SOLARI;
    if (k == "blueprint") return THEME_BLUEPRINT;
    if (k == "radar")     return THEME_RADAR_CRT;
    return THEME_COCKPIT; // Default to Midnight Cockpit
}

inline const char* ThemeIdToKey(ThemeId id)
{
    return GetThemePalette(id).key;
}

inline const char* ThemeIdToDisplayName(ThemeId id)
{
    return GetThemePalette(id).displayName;
}

inline DisplayViewMode ParseViewMode(const String& key)
{
    String k = key;
    k.toLowerCase();
    k.trim();
    if (k == "board" || k == "departure") return VIEW_DEPARTURE_BOARD;
    if (k == "poster" || k == "portrait") return VIEW_PORTRAIT_POSTER;
    return VIEW_RADAR_SPLIT;
}

inline const char* ViewModeToKey(DisplayViewMode mode)
{
    switch (mode) {
        case VIEW_DEPARTURE_BOARD: return "board";
        case VIEW_PORTRAIT_POSTER: return "poster";
        default:                   return "radar";
    }
}

inline const char* ViewModeToDisplayName(DisplayViewMode mode)
{
    switch (mode) {
        case VIEW_DEPARTURE_BOARD: return "DEPARTURE BOARD";
        case VIEW_PORTRAIT_POSTER: return "SPOTTER POSTER";
        default:                   return "RADAR + CARD";
    }
}

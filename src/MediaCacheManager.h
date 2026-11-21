#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include "LGFX.h"
#include "ThemePalette.h"
#include "AviationCatalog.h"
#include "VectorArt.h"

class MediaCacheManager
{
public:
    static constexpr int PHOTO_CACHE_W = 220;
    static constexpr int PHOTO_CACHE_H = 116;
    static constexpr int LOGO_CACHE_W  = 84;
    static constexpr int LOGO_CACHE_H  = 36;

    struct CachedPhoto {
        bool attempted = false;
        bool valid = false;
        uint16_t* pixels565 = nullptr; // Allocated in PSRAM (PHOTO_CACHE_W * PHOTO_CACHE_H * 2 bytes)
        String photographer;
        unsigned long lastUsedMs = 0;
    };

    struct CachedLogo {
        bool attempted = false;
        bool valid = false;
        uint16_t* pixels565 = nullptr; // Allocated in PSRAM (LOGO_CACHE_W * LOGO_CACHE_H * 2 bytes)
        unsigned long lastUsedMs = 0;
    };

private:
    static constexpr size_t MAX_CACHED_PHOTOS = 14;
    static constexpr size_t MAX_CACHED_LOGOS  = 24;
    static constexpr unsigned long FETCH_COOLDOWN_MS = 1200;

    std::map<String, CachedPhoto> photoCache;
    std::map<String, CachedLogo> logoCache;

    String pendingPhotoIcao;
    String pendingPhotoReg;
    String pendingLogoIata;
    unsigned long lastFetchMs = 0;
    bool mediaEnabled = true;

    bool DownloadBytesToPsram(const String& url, uint8_t*& outBuf, size_t& outLen, size_t maxBytes = 131072);
    static bool GetJpegDimensions(const uint8_t* data, size_t len, int& outW, int& outH);
    static bool GetPngDimensions(const uint8_t* data, size_t len, int& outW, int& outH);

    void EvictOldestPhotoIfNeeded();
    void EvictOldestLogoIfNeeded();

    bool FetchAircraftPhotoNow(const String& icao24, const String& reg);
    bool FetchCarrierLogoNow(const String& iata);

public:
    MediaCacheManager() = default;
    ~MediaCacheManager();

    void SetMediaEnabled(bool enabled) { mediaEnabled = enabled; }
    bool IsMediaEnabled() const { return mediaEnabled; }

    void RequestAircraftPhoto(const String& icao24, const String& reg);
    void RequestCarrierLogo(const String& iata);

    // Call once per loop after pushSprite so media downloads never block touch/rendering
    void ProcessPendingQueue();

    bool HasAircraftPhoto(const String& icao24) const;

    // Draws either the real aircraft photo (scaled to target box) or the vector silhouette fallback
    void DrawAircraftVisual(
        LGFX_Sprite& dst,
        int x, int y, int w, int h,
        const String& icao24,
        const String& reg,
        const AircraftTypeInfo& typeInfo,
        const ThemePalette& theme,
        bool showAnnotations = true);

    // Draws either the carrier logo PNG/bitmap or the stylized livery badge fallback
    void DrawCarrierBadge(
        LGFX_Sprite& dst,
        int x, int y, int w, int h,
        const AirlineInfo& airline,
        const ThemePalette& theme);
};

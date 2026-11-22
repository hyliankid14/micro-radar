#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "LGFX.h"
#include "ThemePalette.h"
#include "AviationCatalog.h"
#include "VectorArt.h"
#include "NetWorker.h"

// Caches real aircraft photos and airline logos (decoded RGB565 in PSRAM).
// All downloading/decoding runs on the NetWorker task; the UI thread only
// requests media and blits whatever is already cached (never blocks on network).
class MediaCacheManager
{
public:
    static constexpr int PHOTO_CACHE_W = 420;
    static constexpr int PHOTO_CACHE_H = 280;
    static constexpr int LOGO_CACHE_W  = 160;
    static constexpr int LOGO_CACHE_H  = 64;

    struct CachedPhoto {
        bool valid = false;
        uint16_t* pixels565 = nullptr; // PHOTO_CACHE_W * PHOTO_CACHE_H * 2 bytes in PSRAM
        String photographer;
        unsigned long lastUsedMs = 0;
        uint8_t failCount = 0;
        unsigned long retryAtMs = 0;
    };

    struct CachedLogo {
        bool valid = false;
        uint16_t* pixels565 = nullptr; // LOGO_CACHE_W * LOGO_CACHE_H * 2 bytes in PSRAM
        unsigned long lastUsedMs = 0;
        uint8_t failCount = 0;
        unsigned long retryAtMs = 0;
    };

private:
    static constexpr size_t MAX_CACHED_PHOTOS = 10;
    static constexpr size_t MAX_CACHED_LOGOS  = 32;
    static constexpr uint8_t MAX_TRIES = 3;

    std::map<String, CachedPhoto> photoCache;
    std::map<String, CachedLogo> logoCache;
    SemaphoreHandle_t mutex = nullptr;
    NetWorker* worker = nullptr;
    bool mediaEnabled = true;
    volatile uint32_t version = 0; // bumps whenever a new image lands (drives redraws)

    void Lock() const { if (mutex) xSemaphoreTake(mutex, portMAX_DELAY); }
    void Unlock() const { if (mutex) xSemaphoreGive(mutex); }

    static bool DownloadBytesToPsram(const String& url, uint8_t*& outBuf, size_t& outLen, size_t maxBytes);
    static bool GetJpegDimensions(const uint8_t* data, size_t len, int& outW, int& outH);
    static bool GetPngDimensions(const uint8_t* data, size_t len, int& outW, int& outH);

    void EvictOldestPhotoIfNeeded();
    void EvictOldestLogoIfNeeded();

    void FetchAircraftPhotoNow(const String& icao24, const String& reg);
    void FetchCarrierLogoNow(const String& iata);

    void RecordPhotoFailure(const String& key, bool permanent);
    void RecordLogoFailure(const String& key, bool permanent);

public:
    MediaCacheManager();
    ~MediaCacheManager();

    void Begin(NetWorker* netWorker) { worker = netWorker; }
    void SetMediaEnabled(bool enabled) { mediaEnabled = enabled; }
    bool IsMediaEnabled() const { return mediaEnabled; }
    uint32_t Version() const { return version; }

    void RequestAircraftPhoto(const String& icao24, const String& reg, int prio = NetWorker::PRIO_VISIBLE);
    void RequestCarrierLogo(const String& iata, int prio = NetWorker::PRIO_VISIBLE);

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

    // Draws either the carrier logo bitmap or the stylized livery badge fallback
    void DrawCarrierBadge(
        LGFX_Sprite& dst,
        int x, int y, int w, int h,
        const AirlineInfo& airline,
        const ThemePalette& theme);
};

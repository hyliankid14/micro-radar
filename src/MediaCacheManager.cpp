#include "MediaCacheManager.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

MediaCacheManager::~MediaCacheManager()
{
    for (auto& [k, v] : photoCache) {
        if (v.pixels565) {
            heap_caps_free(v.pixels565);
            v.pixels565 = nullptr;
        }
    }
    for (auto& [k, v] : logoCache) {
        if (v.pixels565) {
            heap_caps_free(v.pixels565);
            v.pixels565 = nullptr;
        }
    }
}

bool MediaCacheManager::DownloadBytesToPsram(const String& url, uint8_t*& outBuf, size_t& outLen, size_t maxBytes)
{
    outBuf = nullptr;
    outLen = 0;
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setTimeout(3500);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(url)) return false;

    http.addHeader("User-Agent", "MicroRadar-AvioFrame/2.0");
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    int contentLen = http.getSize();
    if (contentLen > (int)maxBytes) {
        http.end();
        return false;
    }

    size_t allocSize = (contentLen > 0) ? (size_t)contentLen : maxBytes;
    uint8_t* buf = static_cast<uint8_t*>(heap_caps_malloc(allocSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf) {
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t totalRead = 0;
    unsigned long startMs = millis();

    while (http.connected() && (contentLen > 0 ? (totalRead < (size_t)contentLen) : true)) {
        size_t avail = stream->available();
        if (avail > 0) {
            size_t toRead = min(avail, allocSize - totalRead);
            if (toRead == 0) break;
            int r = stream->readBytes(buf + totalRead, toRead);
            if (r > 0) {
                totalRead += r;
                startMs = millis();
            }
        } else {
            if (contentLen < 0 && totalRead > 0 && !stream->connected()) break;
            if (millis() - startMs > 2500) break;
            delay(2);
        }
        if (totalRead >= allocSize) break;
    }

    http.end();

    if (totalRead < 64) {
        heap_caps_free(buf);
        return false;
    }

    outBuf = buf;
    outLen = totalRead;
    return true;
}

bool MediaCacheManager::GetJpegDimensions(const uint8_t* data, size_t len, int& outW, int& outH)
{
    outW = 0;
    outH = 0;
    if (!data || len < 16 || data[0] != 0xFF || data[1] != 0xD8) return false;

    size_t i = 2;
    while (i + 9 < len) {
        if (data[i] != 0xFF) {
            i++;
            continue;
        }
        uint8_t marker = data[i + 1];
        while (marker == 0xFF && i + 2 < len) {
            i++;
            marker = data[i + 1];
        }
        if (marker == 0xD8 || marker == 0xD9 || (marker >= 0xD0 && marker <= 0xD7)) {
            i += 2;
            continue;
        }
        if (i + 4 >= len) break;
        uint16_t segLen = (uint16_t(data[i + 2]) << 8) | data[i + 3];
        if (segLen < 2) break;

        // SOF0 / SOF1 / SOF2 markers
        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
            if (i + 8 < len) {
                outH = (int(data[i + 5]) << 8) | data[i + 6];
                outW = (int(data[i + 7]) << 8) | data[i + 8];
                return (outW > 0 && outH > 0);
            }
            break;
        }
        i += 2 + segLen;
    }
    return false;
}

bool MediaCacheManager::GetPngDimensions(const uint8_t* data, size_t len, int& outW, int& outH)
{
    outW = 0;
    outH = 0;
    if (!data || len < 24) return false;
    if (data[0] != 0x89 || data[1] != 0x50 || data[2] != 0x4E || data[3] != 0x47) return false;
    outW = (int(data[16]) << 24) | (int(data[17]) << 16) | (int(data[18]) << 8) | data[19];
    outH = (int(data[20]) << 24) | (int(data[21]) << 16) | (int(data[22]) << 8) | data[23];
    return (outW > 0 && outH > 0);
}

void MediaCacheManager::EvictOldestPhotoIfNeeded()
{
    if (photoCache.size() < MAX_CACHED_PHOTOS) return;
    auto oldestIt = photoCache.begin();
    for (auto it = photoCache.begin(); it != photoCache.end(); ++it) {
        if (it->second.lastUsedMs < oldestIt->second.lastUsedMs) {
            oldestIt = it;
        }
    }
    if (oldestIt != photoCache.end()) {
        if (oldestIt->second.pixels565) {
            heap_caps_free(oldestIt->second.pixels565);
        }
        photoCache.erase(oldestIt);
    }
}

void MediaCacheManager::EvictOldestLogoIfNeeded()
{
    if (logoCache.size() < MAX_CACHED_LOGOS) return;
    auto oldestIt = logoCache.begin();
    for (auto it = logoCache.begin(); it != logoCache.end(); ++it) {
        if (it->second.lastUsedMs < oldestIt->second.lastUsedMs) {
            oldestIt = it;
        }
    }
    if (oldestIt != logoCache.end()) {
        if (oldestIt->second.pixels565) {
            heap_caps_free(oldestIt->second.pixels565);
        }
        logoCache.erase(oldestIt);
    }
}

void MediaCacheManager::RequestAircraftPhoto(const String& icao24, const String& reg)
{
    if (!mediaEnabled || icao24.isEmpty()) return;
    String key = icao24;
    key.toLowerCase();

    auto it = photoCache.find(key);
    if (it != photoCache.end() && it->second.attempted) {
        it->second.lastUsedMs = millis();
        return;
    }

    pendingPhotoIcao = key;
    pendingPhotoReg = reg;
}

void MediaCacheManager::RequestCarrierLogo(const String& iata)
{
    if (!mediaEnabled || iata.length() != 2 || iata == "GA") return;
    String key = iata;
    key.toUpperCase();

    auto it = logoCache.find(key);
    if (it != logoCache.end() && it->second.attempted) {
        it->second.lastUsedMs = millis();
        return;
    }

    pendingLogoIata = key;
}

void MediaCacheManager::ProcessPendingQueue()
{
    if (!mediaEnabled || WiFi.status() != WL_CONNECTED) return;
    unsigned long now = millis();
    if (now - lastFetchMs < FETCH_COOLDOWN_MS) return;

    if (!pendingLogoIata.isEmpty()) {
        String iata = pendingLogoIata;
        pendingLogoIata = "";
        auto it = logoCache.find(iata);
        if (it == logoCache.end() || !it->second.attempted) {
            lastFetchMs = now;
            FetchCarrierLogoNow(iata);
            return;
        }
    }

    if (!pendingPhotoIcao.isEmpty()) {
        String icao = pendingPhotoIcao;
        String reg = pendingPhotoReg;
        pendingPhotoIcao = "";
        pendingPhotoReg = "";
        auto it = photoCache.find(icao);
        if (it == photoCache.end() || !it->second.attempted) {
            lastFetchMs = now;
            FetchAircraftPhotoNow(icao, reg);
            return;
        }
    }
}

bool MediaCacheManager::FetchAircraftPhotoNow(const String& icao24, const String& reg)
{
    EvictOldestPhotoIfNeeded();

    CachedPhoto entry;
    entry.attempted = true;
    entry.valid = false;
    entry.lastUsedMs = millis();

    HTTPClient http;
    http.setTimeout(3000);
    String apiUrl = "https://api.planespotters.net/pub/photos/hex/" + icao24;
    String thumbUrl = "";
    String photographer = "";

    auto tryQueryApi = [&](const String& url) -> bool {
        if (!http.begin(url)) return false;
        http.addHeader("User-Agent", "MicroRadar-AvioFrame/2.0");
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }
        String payload = http.getString();
        http.end();

        JsonDocument doc;
        if (deserializeJson(doc, payload)) return false;
        JsonArray photos = doc["photos"];
        if (photos.isNull() || photos.size() == 0) return false;
        JsonObject first = photos[0];
        thumbUrl = first["thumbnail"]["src"] | "";
        if (thumbUrl.isEmpty()) {
            thumbUrl = first["thumbnail_large"]["src"] | "";
        }
        photographer = first["photographer"] | "";
        return !thumbUrl.isEmpty();
    };

    if (!tryQueryApi(apiUrl) && !reg.isEmpty()) {
        tryQueryApi("https://api.planespotters.net/pub/photos/reg/" + reg);
    }

    if (thumbUrl.isEmpty()) {
        photoCache[icao24] = entry;
        return false;
    }

    uint8_t* jpgBytes = nullptr;
    size_t jpgLen = 0;
    if (!DownloadBytesToPsram(thumbUrl, jpgBytes, jpgLen, 98304)) {
        photoCache[icao24] = entry;
        return false;
    }

    int srcW = 200, srcH = 133;
    GetJpegDimensions(jpgBytes, jpgLen, srcW, srcH);
    if (srcW <= 0) srcW = 200;
    if (srcH <= 0) srcH = 133;

    // Decode once into a 16-bit RGB565 sprite in PSRAM
    LGFX_Sprite decodeSpr;
    decodeSpr.setColorDepth(16);
    decodeSpr.setPsram(true);
    if (!decodeSpr.createSprite(PHOTO_CACHE_W, PHOTO_CACHE_H)) {
        heap_caps_free(jpgBytes);
        photoCache[icao24] = entry;
        return false;
    }

    decodeSpr.fillScreen(lgfx::color888(16, 22, 34));
    float scaleX = (float)PHOTO_CACHE_W / (float)srcW;
    float scaleY = (float)PHOTO_CACHE_H / (float)srcH;
    float coverScale = max(scaleX, scaleY);

    bool decoded = decodeSpr.drawJpg(
        jpgBytes, jpgLen,
        PHOTO_CACHE_W / 2, PHOTO_CACHE_H / 2,
        PHOTO_CACHE_W, PHOTO_CACHE_H,
        0, 0,
        coverScale, coverScale,
        datum_t::middle_center
    );
    heap_caps_free(jpgBytes);

    if (!decoded) {
        decodeSpr.deleteSprite();
        photoCache[icao24] = entry;
        return false;
    }

    size_t pixelBytes = PHOTO_CACHE_W * PHOTO_CACHE_H * sizeof(uint16_t);
    uint16_t* buf565 = static_cast<uint16_t*>(heap_caps_malloc(pixelBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf565) {
        decodeSpr.deleteSprite();
        photoCache[icao24] = entry;
        return false;
    }

    memcpy(buf565, decodeSpr.getBuffer(), pixelBytes);
    decodeSpr.deleteSprite();

    entry.valid = true;
    entry.pixels565 = buf565;
    entry.photographer = photographer;
    photoCache[icao24] = entry;

    Serial.printf("[MEDIA] Cached real aircraft photo for %s (by %s)\n", icao24.c_str(), photographer.c_str());
    return true;
}

bool MediaCacheManager::FetchCarrierLogoNow(const String& iata)
{
    EvictOldestLogoIfNeeded();

    CachedLogo entry;
    entry.attempted = true;
    entry.valid = false;
    entry.lastUsedMs = millis();

    // Travelpayouts / Aviasales public airline logo CDN (PNG)
    String url = "https://pics.avs.io/140/60/" + iata + ".png";
    uint8_t* pngBytes = nullptr;
    size_t pngLen = 0;

    if (!DownloadBytesToPsram(url, pngBytes, pngLen, 49152)) {
        logoCache[iata] = entry;
        return false;
    }

    int srcW = 140, srcH = 60;
    GetPngDimensions(pngBytes, pngLen, srcW, srcH);
    if (srcW <= 0) srcW = 140;
    if (srcH <= 0) srcH = 60;

    LGFX_Sprite decodeSpr;
    decodeSpr.setColorDepth(16);
    decodeSpr.setPsram(true);
    if (!decodeSpr.createSprite(LOGO_CACHE_W, LOGO_CACHE_H)) {
        heap_caps_free(pngBytes);
        logoCache[iata] = entry;
        return false;
    }

    // Clean light plate background so dark and colored airline logos are always legible
    decodeSpr.fillScreen(lgfx::color888(246, 246, 248));
    float fitScale = min((float)(LOGO_CACHE_W - 6) / (float)srcW, (float)(LOGO_CACHE_H - 4) / (float)srcH);

    bool decoded = decodeSpr.drawPng(
        pngBytes, pngLen,
        LOGO_CACHE_W / 2, LOGO_CACHE_H / 2,
        LOGO_CACHE_W, LOGO_CACHE_H,
        0, 0,
        fitScale, fitScale,
        datum_t::middle_center
    );
    heap_caps_free(pngBytes);

    if (!decoded) {
        decodeSpr.deleteSprite();
        logoCache[iata] = entry;
        return false;
    }

    size_t pixelBytes = LOGO_CACHE_W * LOGO_CACHE_H * sizeof(uint16_t);
    uint16_t* buf565 = static_cast<uint16_t*>(heap_caps_malloc(pixelBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf565) {
        decodeSpr.deleteSprite();
        logoCache[iata] = entry;
        return false;
    }

    memcpy(buf565, decodeSpr.getBuffer(), pixelBytes);
    decodeSpr.deleteSprite();

    entry.valid = true;
    entry.pixels565 = buf565;
    logoCache[iata] = entry;

    Serial.printf("[MEDIA] Cached airline logo for %s\n", iata.c_str());
    return true;
}

bool MediaCacheManager::HasAircraftPhoto(const String& icao24) const
{
    String key = icao24;
    key.toLowerCase();
    auto it = photoCache.find(key);
    return (it != photoCache.end() && it->second.valid && it->second.pixels565 != nullptr);
}

void MediaCacheManager::DrawAircraftVisual(
    LGFX_Sprite& dst,
    int x, int y, int w, int h,
    const String& icao24,
    const String& reg,
    const AircraftTypeInfo& typeInfo,
    const ThemePalette& theme,
    bool showAnnotations)
{
    String key = icao24;
    key.toLowerCase();

    RequestAircraftPhoto(key, reg);

    auto it = photoCache.find(key);
    if (mediaEnabled && it != photoCache.end() && it->second.valid && it->second.pixels565) {
        it->second.lastUsedMs = millis();

        dst.fillRoundRect(x, y, w, h, 6, theme.bgSecondary);

        // Push cached RGB565 photo centered or scaled into [x+2, y+2, w-4, h-4]
        int innerW = w - 4;
        int innerH = h - 4;
        dst.setClipRect(x + 2, y + 2, innerW, innerH);

        LGFX_Sprite srcSpr;
        srcSpr.setColorDepth(16);
        srcSpr.setBuffer(it->second.pixels565, PHOTO_CACHE_W, PHOTO_CACHE_H, 16);

        float sx = (float)innerW / (float)PHOTO_CACHE_W;
        float sy = (float)innerH / (float)PHOTO_CACHE_H;
        float scale = max(sx, sy);
        srcSpr.pushRotateZoom(&dst, x + w / 2, y + h / 2, 0.0f, scale, scale);

        // Subtle bottom caption bar for photographer credit & aircraft model
        if (showAnnotations) {
            int barH = 16;
            dst.fillRect(x + 2, y + h - 2 - barH, innerW, barH, lgfx::color888(10, 14, 22));
            dst.setTextFont(1);
            dst.setTextSize(1);
            dst.setTextColor(lgfx::color888(230, 238, 250));
            String leftLabel = !typeInfo.fullName.isEmpty() ? typeInfo.fullName : typeInfo.code;
            if (!reg.isEmpty()) leftLabel += " (" + reg + ")";
            dst.drawString(leftLabel, x + 6, y + h - barH + 2);

            if (!it->second.photographer.isEmpty()) {
                dst.setTextColor(lgfx::color888(165, 178, 198));
                String credit = "Photo: " + it->second.photographer;
                if (credit.length() > 22) credit = credit.substring(0, 21) + ".";
                dst.drawRightString(credit, x + w - 6, y + h - barH + 2);
            }
        }

        dst.clearClipRect();
        dst.drawRoundRect(x, y, w, h, 6, theme.borderPrimary);
        return;
    }

    // Instant vector silhouette fallback
    VectorArt::DrawAircraftSilhouette(
        dst, x, y, w, h,
        typeInfo.category, theme,
        typeInfo.code, typeInfo.fullName, reg,
        showAnnotations
    );
}

void MediaCacheManager::DrawCarrierBadge(
    LGFX_Sprite& dst,
    int x, int y, int w, int h,
    const AirlineInfo& airline,
    const ThemePalette& theme)
{
    if (mediaEnabled && airline.matched && airline.iata.length() == 2) {
        String key = airline.iata;
        key.toUpperCase();
        RequestCarrierLogo(key);

        auto it = logoCache.find(key);
        if (it != logoCache.end() && it->second.valid && it->second.pixels565) {
            it->second.lastUsedMs = millis();

            dst.fillRoundRect(x, y, w, h, 5, lgfx::color888(246, 246, 248));
            dst.setClipRect(x + 2, y + 2, w - 4, h - 4);

            LGFX_Sprite srcSpr;
            srcSpr.setColorDepth(16);
            srcSpr.setBuffer(it->second.pixels565, LOGO_CACHE_W, LOGO_CACHE_H, 16);

            float sx = (float)(w - 4) / (float)LOGO_CACHE_W;
            float sy = (float)(h - 4) / (float)LOGO_CACHE_H;
            float scale = min(sx, sy);
            srcSpr.pushRotateZoom(&dst, x + w / 2, y + h / 2, 0.0f, scale, scale);

            dst.clearClipRect();
            dst.drawRoundRect(x, y, w, h, 5, theme.borderPrimary);
            return;
        }
    }

    VectorArt::DrawCarrierLogoBadge(dst, x, y, w, h, airline, theme);
}

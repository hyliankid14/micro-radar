#include "MediaCacheManager.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

namespace {

// Stream sink that collects an HTTP body straight into PSRAM. Using
// HTTPClient::writeToStream (with HTTP/1.0) avoids the chunked-transfer framing
// bytes that previously corrupted PNG/JPEG data read via the raw socket.
class PsramSink : public Stream
{
public:
    uint8_t* buf = nullptr;
    size_t cap = 0;
    size_t len = 0;
    bool overflow = false;

    explicit PsramSink(size_t capacity) : cap(capacity)
    {
        buf = static_cast<uint8_t*>(heap_caps_malloc(cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    ~PsramSink() { if (buf) heap_caps_free(buf); }

    uint8_t* Release() { uint8_t* b = buf; buf = nullptr; return b; }

    size_t write(uint8_t b) override { return write(&b, 1); }
    size_t write(const uint8_t* d, size_t n) override
    {
        if (!buf || len + n > cap) { overflow = true; return 0; }
        memcpy(buf + len, d, n);
        len += n;
        return n;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
};

} // namespace

MediaCacheManager::MediaCacheManager()
{
    mutex = xSemaphoreCreateMutex();
}

MediaCacheManager::~MediaCacheManager()
{
    for (auto& [k, v] : photoCache) {
        if (v.pixels565) heap_caps_free(v.pixels565);
    }
    for (auto& [k, v] : logoCache) {
        if (v.pixels565) heap_caps_free(v.pixels565);
    }
}

bool MediaCacheManager::DownloadBytesToPsram(const String& url, uint8_t*& outBuf, size_t& outLen, size_t maxBytes)
{
    outBuf = nullptr;
    outLen = 0;
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setTimeout(4500);
    http.setReuse(false);
    http.useHTTP10(true); // no chunked encoding
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(url)) return false;
    http.addHeader("User-Agent", "MicroRadar-AvioFrame/2.0");

    const uint32_t t0 = millis();
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[MEDIA] GET %s -> HTTP %d\n", url.c_str(), code);
        http.end();
        return false;
    }

    int contentLen = http.getSize();
    if (contentLen > static_cast<int>(maxBytes)) {
        Serial.printf("[MEDIA] %s too large (%d bytes)\n", url.c_str(), contentLen);
        http.end();
        return false;
    }

    PsramSink sink(contentLen > 0 ? static_cast<size_t>(contentLen) : maxBytes);
    if (!sink.buf) {
        http.end();
        return false;
    }
    http.writeToStream(&sink);
    http.end();

    if (sink.overflow || sink.len < 64 || (contentLen > 0 && sink.len != static_cast<size_t>(contentLen))) {
        Serial.printf("[MEDIA] %s incomplete (%u of %d bytes)\n", url.c_str(), static_cast<unsigned>(sink.len), contentLen);
        return false;
    }

    outLen = sink.len;
    outBuf = sink.Release();
    Serial.printf("[MEDIA] %s %u bytes in %lu ms\n", url.c_str(), static_cast<unsigned>(outLen), static_cast<unsigned long>(millis() - t0));
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
    // Caller holds the lock
    size_t valid = 0;
    for (auto& kv : photoCache) if (kv.second.pixels565) ++valid;
    if (valid < MAX_CACHED_PHOTOS) return;

    auto oldest = photoCache.end();
    for (auto it = photoCache.begin(); it != photoCache.end(); ++it) {
        if (!it->second.pixels565) continue;
        if (oldest == photoCache.end() || it->second.lastUsedMs < oldest->second.lastUsedMs) oldest = it;
    }
    if (oldest != photoCache.end()) {
        heap_caps_free(oldest->second.pixels565);
        photoCache.erase(oldest);
    }
}

void MediaCacheManager::EvictOldestLogoIfNeeded()
{
    // Caller holds the lock
    size_t valid = 0;
    for (auto& kv : logoCache) if (kv.second.pixels565) ++valid;
    if (valid < MAX_CACHED_LOGOS) return;

    auto oldest = logoCache.end();
    for (auto it = logoCache.begin(); it != logoCache.end(); ++it) {
        if (!it->second.pixels565) continue;
        if (oldest == logoCache.end() || it->second.lastUsedMs < oldest->second.lastUsedMs) oldest = it;
    }
    if (oldest != logoCache.end()) {
        heap_caps_free(oldest->second.pixels565);
        logoCache.erase(oldest);
    }
}

void MediaCacheManager::RecordPhotoFailure(const String& key, bool permanent)
{
    Lock();
    CachedPhoto& e = photoCache[key];
    e.valid = false;
    e.failCount = permanent ? MAX_TRIES : static_cast<uint8_t>(e.failCount + 1);
    e.retryAtMs = millis() + 30000UL * e.failCount;
    e.lastUsedMs = millis();
    Unlock();
}

void MediaCacheManager::RecordLogoFailure(const String& key, bool permanent)
{
    Lock();
    CachedLogo& e = logoCache[key];
    e.valid = false;
    e.failCount = permanent ? MAX_TRIES : static_cast<uint8_t>(e.failCount + 1);
    e.retryAtMs = millis() + 30000UL * e.failCount;
    e.lastUsedMs = millis();
    Unlock();
}

void MediaCacheManager::RequestAircraftPhoto(const String& icao24, const String& reg, int prio)
{
    if (!mediaEnabled || !worker || icao24.isEmpty()) return;
    String key = icao24;
    key.toLowerCase();

    Lock();
    auto it = photoCache.find(key);
    if (it != photoCache.end()) {
        it->second.lastUsedMs = millis();
        if (it->second.valid || it->second.failCount >= MAX_TRIES || millis() < it->second.retryAtMs) {
            Unlock();
            return;
        }
    }
    Unlock();

    String regCopy = reg;
    worker->Submit("photo:" + key, [this, key, regCopy]() { FetchAircraftPhotoNow(key, regCopy); }, prio);
}

void MediaCacheManager::RequestCarrierLogo(const String& iata, int prio)
{
    if (!mediaEnabled || !worker || iata.length() != 2 || iata == "GA") return;
    String key = iata;
    key.toUpperCase();

    Lock();
    auto it = logoCache.find(key);
    if (it != logoCache.end()) {
        it->second.lastUsedMs = millis();
        if (it->second.valid || it->second.failCount >= MAX_TRIES || millis() < it->second.retryAtMs) {
            Unlock();
            return;
        }
    }
    Unlock();

    worker->Submit("logo:" + key, [this, key]() { FetchCarrierLogoNow(key); }, prio);
}

void MediaCacheManager::FetchAircraftPhotoNow(const String& icao24, const String& reg)
{
    if (WiFi.status() != WL_CONNECTED) return; // try again later without penalty

    String thumbUrl;
    String photographer;
    bool apiReachable = false;

    auto tryQueryApi = [&](const String& url) -> bool {
        HTTPClient http;
        http.setTimeout(4000);
        if (!http.begin(url)) return false;
        http.addHeader("User-Agent", "MicroRadar-AvioFrame/2.0");
        int code = http.GET();
        if (code != HTTP_CODE_OK && code != HTTP_CODE_NOT_FOUND) {
            Serial.printf("[MEDIA] planespotters %s -> HTTP %d\n", url.c_str(), code);
            http.end();
            return false;
        }
        apiReachable = true;
        if (code != HTTP_CODE_OK) { http.end(); return false; }
        String payload = http.getString();
        http.end();

        JsonDocument doc;
        if (deserializeJson(doc, payload)) return false;
        JsonArray photos = doc["photos"];
        if (photos.isNull() || photos.size() == 0) return false;
        JsonObject first = photos[0];
        thumbUrl = first["thumbnail_large"]["src"] | "";
        if (thumbUrl.isEmpty()) thumbUrl = first["thumbnail"]["src"] | "";
        photographer = first["photographer"] | "";
        return !thumbUrl.isEmpty();
    };

    if (!tryQueryApi("https://api.planespotters.net/pub/photos/hex/" + icao24) && !reg.isEmpty()) {
        tryQueryApi("https://api.planespotters.net/pub/photos/reg/" + reg);
    }

    if (thumbUrl.isEmpty()) {
        // Reachable but no photo => don't retry this session; unreachable => retry with backoff
        RecordPhotoFailure(icao24, apiReachable);
        Serial.printf("[MEDIA] no photo for %s (%s)\n", icao24.c_str(), apiReachable ? "none exists" : "api unreachable");
        return;
    }

    uint8_t* jpgBytes = nullptr;
    size_t jpgLen = 0;
    if (!DownloadBytesToPsram(thumbUrl, jpgBytes, jpgLen, 160000)) {
        RecordPhotoFailure(icao24, false);
        return;
    }

    int srcW = 420, srcH = 280;
    GetJpegDimensions(jpgBytes, jpgLen, srcW, srcH);
    if (srcW <= 0) srcW = 420;
    if (srcH <= 0) srcH = 280;

    LGFX_Sprite decodeSpr;
    decodeSpr.setColorDepth(16);
    decodeSpr.setPsram(true);
    if (!decodeSpr.createSprite(PHOTO_CACHE_W, PHOTO_CACHE_H)) {
        heap_caps_free(jpgBytes);
        RecordPhotoFailure(icao24, false);
        return;
    }

    decodeSpr.fillScreen(lgfx::color888(16, 22, 34));
    float coverScale = max((float)PHOTO_CACHE_W / srcW, (float)PHOTO_CACHE_H / srcH);
    bool decoded = decodeSpr.drawJpg(
        jpgBytes, jpgLen,
        PHOTO_CACHE_W / 2, PHOTO_CACHE_H / 2,
        PHOTO_CACHE_W, PHOTO_CACHE_H,
        0, 0,
        coverScale, coverScale,
        datum_t::middle_center);
    heap_caps_free(jpgBytes);

    if (!decoded) {
        Serial.printf("[MEDIA] JPEG decode failed for %s\n", icao24.c_str());
        decodeSpr.deleteSprite();
        RecordPhotoFailure(icao24, false);
        return;
    }

    const size_t pixelBytes = PHOTO_CACHE_W * PHOTO_CACHE_H * sizeof(uint16_t);
    uint16_t* buf565 = static_cast<uint16_t*>(heap_caps_malloc(pixelBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf565) {
        decodeSpr.deleteSprite();
        RecordPhotoFailure(icao24, false);
        return;
    }
    memcpy(buf565, decodeSpr.getBuffer(), pixelBytes);
    decodeSpr.deleteSprite();

    Lock();
    EvictOldestPhotoIfNeeded();
    CachedPhoto& e = photoCache[icao24];
    if (e.pixels565) heap_caps_free(e.pixels565);
    e.valid = true;
    e.pixels565 = buf565;
    e.photographer = photographer;
    e.failCount = 0;
    e.lastUsedMs = millis();
    version = version + 1;
    Unlock();

    Serial.printf("[MEDIA] cached photo for %s (by %s)\n", icao24.c_str(), photographer.c_str());
}

void MediaCacheManager::FetchCarrierLogoNow(const String& iata)
{
    if (WiFi.status() != WL_CONNECTED) return;

    // Source chain: Aviasales CDN first, Kiwi as fallback
    const String urls[] = {
        "https://pics.avs.io/320/128/" + iata + ".png",
        "https://images.kiwi.com/airlines/128x128/" + iata + ".png",
    };

    for (const String& url : urls) {
        uint8_t* pngBytes = nullptr;
        size_t pngLen = 0;
        if (!DownloadBytesToPsram(url, pngBytes, pngLen, 90000)) continue;

        int srcW = 0, srcH = 0;
        if (!GetPngDimensions(pngBytes, pngLen, srcW, srcH)) {
            Serial.printf("[MEDIA] %s is not a PNG\n", url.c_str());
            heap_caps_free(pngBytes);
            continue;
        }

        LGFX_Sprite decodeSpr;
        decodeSpr.setColorDepth(16);
        decodeSpr.setPsram(true);
        if (!decodeSpr.createSprite(LOGO_CACHE_W, LOGO_CACHE_H)) {
            heap_caps_free(pngBytes);
            continue;
        }

        // Clean light plate so dark and coloured logos are legible on every theme
        decodeSpr.fillScreen(lgfx::color888(248, 248, 250));
        float fitScale = min((float)(LOGO_CACHE_W - 12) / srcW, (float)(LOGO_CACHE_H - 8) / srcH);
        bool decoded = decodeSpr.drawPng(
            pngBytes, pngLen,
            LOGO_CACHE_W / 2, LOGO_CACHE_H / 2,
            LOGO_CACHE_W, LOGO_CACHE_H,
            0, 0,
            fitScale, fitScale,
            datum_t::middle_center);
        heap_caps_free(pngBytes);

        if (!decoded) {
            Serial.printf("[MEDIA] PNG decode failed for %s\n", url.c_str());
            decodeSpr.deleteSprite();
            continue;
        }

        const size_t pixelBytes = LOGO_CACHE_W * LOGO_CACHE_H * sizeof(uint16_t);
        uint16_t* buf565 = static_cast<uint16_t*>(heap_caps_malloc(pixelBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!buf565) {
            decodeSpr.deleteSprite();
            continue;
        }
        memcpy(buf565, decodeSpr.getBuffer(), pixelBytes);
        decodeSpr.deleteSprite();

        Lock();
        EvictOldestLogoIfNeeded();
        CachedLogo& e = logoCache[iata];
        if (e.pixels565) heap_caps_free(e.pixels565);
        e.valid = true;
        e.pixels565 = buf565;
        e.failCount = 0;
        e.lastUsedMs = millis();
        version = version + 1;
        Unlock();

        Serial.printf("[MEDIA] cached logo %s\n", iata.c_str());
        return;
    }

    RecordLogoFailure(iata, false);
}

bool MediaCacheManager::HasAircraftPhoto(const String& icao24) const
{
    String key = icao24;
    key.toLowerCase();
    Lock();
    auto it = photoCache.find(key);
    bool r = (it != photoCache.end() && it->second.valid && it->second.pixels565 != nullptr);
    Unlock();
    return r;
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

    bool drewPhoto = false;
    Lock();
    auto it = photoCache.find(key);
    if (mediaEnabled && it != photoCache.end() && it->second.valid && it->second.pixels565) {
        it->second.lastUsedMs = millis();
        drewPhoto = true;

        dst.fillRoundRect(x, y, w, h, 6, theme.bgSecondary);

        const int innerW = w - 4;
        const int innerH = h - 4;
        dst.setClipRect(x + 2, y + 2, innerW, innerH);

        LGFX_Sprite srcSpr;
        srcSpr.setColorDepth(16);
        srcSpr.setBuffer(it->second.pixels565, PHOTO_CACHE_W, PHOTO_CACHE_H, 16);

        const float scale = max((float)innerW / PHOTO_CACHE_W, (float)innerH / PHOTO_CACHE_H);
        srcSpr.pushRotateZoom(&dst, x + w / 2, y + h / 2, 0.0f, scale, scale);

        if (showAnnotations) {
            const int barH = 16;
            dst.fillRect(x + 2, y + h - 2 - barH, innerW, barH, lgfx::color888(10, 14, 22));
            UseFont(dst, 1);
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
    }
    Unlock();
    if (drewPhoto) return;

    // Instant vector silhouette fallback while loading / when no photo exists
    VectorArt::DrawAircraftSilhouette(
        dst, x, y, w, h,
        typeInfo.category, theme,
        typeInfo.code, typeInfo.fullName, reg,
        showAnnotations);
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

        bool drew = false;
        Lock();
        auto it = logoCache.find(key);
        if (it != logoCache.end() && it->second.valid && it->second.pixels565) {
            it->second.lastUsedMs = millis();
            drew = true;

            dst.fillRoundRect(x, y, w, h, 5, lgfx::color888(248, 248, 250));
            dst.setClipRect(x + 2, y + 2, w - 4, h - 4);

            LGFX_Sprite srcSpr;
            srcSpr.setColorDepth(16);
            srcSpr.setBuffer(it->second.pixels565, LOGO_CACHE_W, LOGO_CACHE_H, 16);

            const float scale = min((float)(w - 4) / LOGO_CACHE_W, (float)(h - 4) / LOGO_CACHE_H);
            srcSpr.pushRotateZoom(&dst, x + w / 2, y + h / 2, 0.0f, scale, scale);

            dst.clearClipRect();
            dst.drawRoundRect(x, y, w, h, 5, theme.borderPrimary);
        }
        Unlock();
        if (drew) return;
    }

    VectorArt::DrawCarrierLogoBadge(dst, x, y, w, h, airline, theme);
}

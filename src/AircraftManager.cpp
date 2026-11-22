#include "AircraftManager.h"
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include <algorithm>
#include "DisplayLayout.h"
#include "DrawHelpers.h"

#include <ArduinoJson.h>
#include "models/Aircraft.h"

static int DrawWrappedText(LGFX_Sprite& buf, int x, int y, const String& text, int maxWidth, int lineHeight, int maxLines = 2)
{
    if (text.isEmpty()) return y;

    String remaining = text;
    int linesDrawn = 0;

    while (remaining.length() > 0 && linesDrawn < maxLines) {
        if (linesDrawn > 0) y += lineHeight;
        String line = "";
        unsigned int charIndex = 0;

        while (charIndex < remaining.length()) {
            String testLine = line + remaining.charAt(charIndex);
            if (buf.textWidth(testLine) > maxWidth) {
                int lastSpace = line.lastIndexOf(' ');
                if (lastSpace > 0) {
                    line = line.substring(0, lastSpace);
                    remaining = remaining.substring(lastSpace + 1);
                } else {
                    remaining = remaining.substring(charIndex);
                }
                break;
            }
            line = testLine;
            charIndex++;
        }

        if (charIndex >= remaining.length()) {
            remaining = "";
        } else if (linesDrawn == maxLines - 1 && line.length() > 3) {
            line = line.substring(0, line.length() - 2) + "..";
            remaining = "";
        }

        buf.drawString(line, x, y);
        linesDrawn++;
    }

    y += lineHeight;
    return y;
}

static String TruncateToWidth(LGFX_Sprite& buf, const String& text, int maxWidth)
{
    if (buf.textWidth(text) <= maxWidth) return text;
    String s = text;
    while (s.length() > 2 && buf.textWidth(s + "..") > maxWidth) {
        s.remove(s.length() - 1);
    }
    return s + "..";
}

void AircraftManager::Initialise()
{
    ReloadRuntimeSettings();
    if (!resultMutex) resultMutex = xSemaphoreCreateMutex();
    net.Begin();
    mediaCache.Begin(&net);
}

void AircraftManager::ReloadRuntimeSettings()
{
    lat = configServer.GetStoredString("latitude").toDouble();
    lon = configServer.GetStoredString("longitude").toDouble();
    double storedRad = configServer.GetStoredString("radius").toDouble();
    if (storedRad > 0.0) {
        rad = storedRad;
    } else {
        rad = 1.0;
    }

    const String unitsStr = configServer.GetStoredString("units");
    units = (unitsStr == "imperial") ? IMPERIAL : METRIC;

    const String renderText = configServer.GetStoredString("infotext");
    const String renderTris = configServer.GetStoredString("triangle");
    const String renderScan = configServer.GetStoredString("scanline");
    displayInfoText = renderText.isEmpty() || renderText == "true";
    displayTriangles = renderTris.isEmpty() || renderTris == "true";
    displayScanline = renderScan.isEmpty() || renderScan == "true";

    const String mediaStr = configServer.GetStoredString("media");
    mediaCache.SetMediaEnabled(mediaStr.isEmpty() || mediaStr == "true");

    const String storedTheme = configServer.GetStoredString("theme");
    themeId = storedTheme.isEmpty() ? THEME_RADAR_CRT : ParseThemeId(storedTheme);

    const String storedView = configServer.GetStoredString("view-mode");
    viewMode = storedView.isEmpty() ? VIEW_RADAR_SPLIT : ParseViewMode(storedView);

    const String storedAutoCycle = configServer.GetStoredString("auto-cycle-views");
    autoCycleViews = (storedAutoCycle == "true");

    giftTitle = configServer.GetStoredString("gift-title");
    giftTitle.trim();
    if (giftTitle.isEmpty()) giftTitle = "SKY FRAME";

    giftSubtitle = configServer.GetStoredString("gift-subtitle");
    giftSubtitle.trim();
    if (giftSubtitle.isEmpty()) giftSubtitle = "Live Aviation Display";

    const String tzStr = configServer.GetStoredString("tz-offset");
    tzOffsetMinutes = tzStr.isEmpty() ? 0 : tzStr.toInt();

    const String brStr = configServer.GetStoredString("brightness");
    const String nbStr = configServer.GetStoredString("night-brightness");
    dayBrightnessPct = brStr.isEmpty() ? 80 : constrain(brStr.toInt(), 5, 100);
    nightBrightnessPct = nbStr.isEmpty() ? 10 : constrain(nbStr.toInt(), 0, 100);

    nightDimEnabled = (configServer.GetStoredString("night-dim") == "true");
    const String nsStr = configServer.GetStoredString("night-start");
    const String neStr = configServer.GetStoredString("night-end");
    nightStartHour = nsStr.isEmpty() ? 23 : constrain(nsStr.toInt(), 0, 23);
    nightEndHour = neStr.isEmpty() ? 7 : constrain(neStr.toInt(), 0, 23);

    pinnedQuery = configServer.GetStoredString("pinned-flight");
    pinnedQuery.trim();
    pinnedQuery.toUpperCase();
    if (pinnedQuery.isEmpty()) {
        pinnedIcao = "";
    }

    constexpr unsigned long MS_PER_DAY = 24UL * 60UL * 60UL * 1000UL;
    trackingProvider = configServer.GetStoredString("tracking-provider");
    if (trackingProvider.isEmpty()) trackingProvider = "adsblol";

    if (trackingProvider == "opensky") {
        constexpr int ANONYMOUS_TOKENS_PER_DAY = 400;
        constexpr int AUTHED_TOKENS_PER_DAY = 4000;
        constexpr int TOKEN_BUFFER = 3;
        int dailyRequestBudget = ANONYMOUS_TOKENS_PER_DAY - TOKEN_BUFFER;

        const String openskyId = configServer.GetStoredString("opensky-id");
        const String openskySecret = configServer.GetStoredString("opensky-secret");
        if (!openskyId.isEmpty() && !openskySecret.isEmpty()) {
            dailyRequestBudget = AUTHED_TOKENS_PER_DAY - TOKEN_BUFFER;
        }
        fetchInterval = MS_PER_DAY / dailyRequestBudget;
    } else {
        trackingProvider = "adsblol";
        // ~15 second refresh interval for smooth live tracking on adsb.lol
        fetchInterval = 15000;
    }

    Serial.printf("[INIT] View=%s, Theme=%s, Provider=%s, Interval=%lums\n",
        ViewModeToKey(viewMode), ThemeIdToKey(themeId), trackingProvider.c_str(), fetchInterval);
}

void AircraftManager::SetViewMode(DisplayViewMode mode, bool persist)
{
    int idx = static_cast<int>(mode);
    if (idx < 0 || idx >= VIEW_COUNT) idx = 0;
    viewMode = static_cast<DisplayViewMode>(idx);
    lastViewCycleMs = millis();
    if (persist) {
        configServer.SetStoredString("view-mode", ViewModeToKey(viewMode));
    }
    Serial.printf("[VIEW] Switched to %s\n", ViewModeToDisplayName(viewMode));
}

void AircraftManager::CycleViewMode()
{
    int next = (static_cast<int>(viewMode) + 1) % VIEW_COUNT;
    SetViewMode(static_cast<DisplayViewMode>(next), true);
}

void AircraftManager::SetThemeId(ThemeId id, bool persist)
{
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= THEME_COUNT) idx = 0;
    themeId = static_cast<ThemeId>(idx);
    if (persist) {
        configServer.SetStoredString("theme", ThemeIdToKey(themeId));
    }
    Serial.printf("[THEME] Switched to %s\n", ThemeIdToDisplayName(themeId));
}

void AircraftManager::CycleTheme()
{
    int next = (static_cast<int>(themeId) + 1) % THEME_COUNT;
    SetThemeId(static_cast<ThemeId>(next), true);
}

void AircraftManager::SetPinnedFlightQuery(const String& query, bool persist)
{
    pinnedQuery = query;
    pinnedQuery.trim();
    pinnedQuery.toUpperCase();
    // Remove internal spaces (e.g. "BA 117" -> "BA117")
    pinnedQuery.replace(" ", "");

    if (persist) {
        configServer.SetStoredString("pinned-flight", pinnedQuery);
    }

    if (pinnedQuery.isEmpty()) {
        pinnedIcao = "";
        Serial.println("[PIN] Cleared pinned flight");
    } else {
        lastPinnedFetchMs = 0; // trigger immediate fetch
        Serial.printf("[PIN] Pinned flight set to: %s\n", pinnedQuery.c_str());
    }
}

void AircraftManager::SetBrightness(int pct, bool persist)
{
    dayBrightnessPct = constrain(pct, 5, 100);
    if (persist) configServer.SetStoredString("brightness", String(dayBrightnessPct));
}

bool AircraftManager::IsNightQuietHours() const
{
    if (!nightDimEnabled) return false;
    time_t nowUtc = time(nullptr);
    if (nowUtc < 1700000000) return false;
    time_t localTime = nowUtc + (tzOffsetMinutes * 60);
    struct tm tmbuf;
    gmtime_r(&localTime, &tmbuf);
    int hr = tmbuf.tm_hour;

    if (nightStartHour == nightEndHour) return false;
    if (nightStartHour > nightEndHour) {
        // e.g. 23:00 to 07:00
        return (hr >= nightStartHour || hr < nightEndHour);
    } else {
        // e.g. 01:00 to 06:00
        return (hr >= nightStartHour && hr < nightEndHour);
    }
}

void AircraftManager::adjustRadius(double delta)
{
    setRadius(rad + delta);
}

void AircraftManager::setRadius(double newRad)
{
    rad = newRad;
    if (rad < 0.05) rad = 0.05;
    if (rad > 180.0) rad = 180.0;
    configServer.SetStoredString("radius", String(rad, 4));
    Serial.printf("[RADAR] Radius adjusted to: %.4f deg\n", rad);
}

std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - lon;
    const float dLat = predLat - lat;

    const float normLon = (dLon + rad) / (2.0f * rad);
    const float normLat = (dLat + rad) / (2.0f * rad);

    const int x = RADAR_X + static_cast<int>(normLon * RADAR_SIZE);
    const int y = RADAR_Y + static_cast<int>(RADAR_SIZE - (normLat * RADAR_SIZE));

    return { x, y };
}

std::vector<AircraftManager::SortedFlightItem> AircraftManager::GetSortedActiveFlights() const
{
    std::vector<SortedFlightItem> list;
    list.reserve(trackedAircraft.size());

    for (const auto& [icao, tracked] : trackedAircraft) {
        bool isPin = (!pinnedIcao.isEmpty() && icao == pinnedIcao);
        if (tracked.state.onGround && !isPin) continue;

        auto [predLat, predLon] = tracked.GetDisplayPosition();
        float distKm = HasLocationConfigured()
            ? AviationCatalog::ComputeDistanceKm(lat, lon, predLat, predLon)
            : 0.0f;
        float brg = HasLocationConfigured()
            ? AviationCatalog::ComputeBearingDeg(lat, lon, predLat, predLon)
            : 0.0f;

        list.push_back({ icao, distKm, brg, isPin });
    }

    std::sort(list.begin(), list.end(), [](const SortedFlightItem& a, const SortedFlightItem& b) {
        if (a.isPinned != b.isPinned) return a.isPinned > b.isPinned;
        return a.distKm < b.distKm;
    });

    return list;
}

void AircraftManager::SelectNextFlight(int direction)
{
    auto list = GetSortedActiveFlights();
    if (list.empty()) return;

    int curIdx = -1;
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].icao == selectedIcao) {
            curIdx = static_cast<int>(i);
            break;
        }
    }

    int nextIdx = 0;
    if (curIdx >= 0) {
        int n = static_cast<int>(list.size());
        nextIdx = (curIdx + direction + n) % n;
    }

    SelectFlight(list[nextIdx].icao);
}

bool AircraftManager::HandleContentTap(int x, int y)
{
    if (statsModalOpen) {
        statsModalOpen = false;
        return true;
    }

    if (viewMode == VIEW_RADAR_SPLIT) {
        // Right card CLOSE tap zone (top-right of card)
        if (x >= 720 && x <= 792 && y >= 8 && y <= 52 && IsDetailOpen()) {
            DismissDetail();
            return true;
        }
        // Right card < PREV / NEXT > tap zone or cycle tap
        if (x >= 452 && y < 436) {
            SelectNextFlight(1);
            return true;
        }
        // Radar scope tap
        if (x >= RADAR_X && x < RADAR_X + RADAR_SIZE && y >= RADAR_Y && y < RADAR_Y + RADAR_SIZE) {
            String hit = HitTestAircraft(x, y);
            if (!hit.isEmpty()) {
                SelectFlight(hit);
                return true;
            }
        }
        return false;
    }

    if (viewMode == VIEW_DEPARTURE_BOARD) {
        // Rows start at y=84, height=48, up to 7 rows
        const int rowStartY = 84;
        const int rowH = 48;
        if (y >= rowStartY && y < rowStartY + 7 * rowH) {
            int rowIdx = (y - rowStartY) / rowH;
            auto list = GetSortedActiveFlights();
            if (rowIdx >= 0 && rowIdx < (int)list.size()) {
                SelectFlight(list[rowIdx].icao);
                SetViewMode(VIEW_PORTRAIT_POSTER, false);
                return true;
            }
        }
        return false;
    }

    if (viewMode == VIEW_PORTRAIT_POSTER) {
        // Prev / Next buttons in bottom-right of poster card
        if (y >= 384 && y < 436 && x >= 464 && x < 620) {
            SelectNextFlight(-1);
            return true;
        }
        if (y >= 384 && y < 436 && x >= 624 && x < 788) {
            SelectNextFlight(1);
            return true;
        }
        // Tap anywhere else on poster cycles to next flight
        if (y < 436) {
            SelectNextFlight(1);
            return true;
        }
    }

    return false;
}

void AircraftManager::Draw(LGFX_Sprite& backbuffer)
{
    const ThemePalette& theme = GetTheme();
    backbuffer.fillScreen(theme.bgPrimary);

    if (viewMode == VIEW_DEPARTURE_BOARD) {
        DrawDepartureBoardView(backbuffer, theme);
    } else if (viewMode == VIEW_PORTRAIT_POSTER) {
        DrawPortraitPosterView(backbuffer, theme);
    } else {
        DrawSplitRadarView(backbuffer, theme);
    }

    if (statsModalOpen) {
        DrawStatsModal(backbuffer, theme);
    }
}

void AircraftManager::DrawRadarCircles(LGFX_Sprite& backbuffer, const ThemePalette& theme) const
{
    backbuffer.fillCircle(RADAR_CENTRE_X, RADAR_CENTRE_Y, RADAR_RADIUS, theme.bgSecondary);

    // Crosshairs
    backbuffer.drawFastHLine(RADAR_CENTRE_X - RADAR_RADIUS + 4, RADAR_CENTRE_Y, (RADAR_RADIUS - 4) * 2, theme.gridLine);
    backbuffer.drawFastVLine(RADAR_CENTRE_X, RADAR_CENTRE_Y - RADAR_RADIUS + 4, (RADAR_RADIUS - 4) * 2, theme.gridLine);

    // Concentric range rings
    backbuffer.drawCircle(RADAR_CENTRE_X, RADAR_CENTRE_Y, RADAR_RADIUS, theme.radarRingOuter);
    backbuffer.drawCircle(RADAR_CENTRE_X, RADAR_CENTRE_Y, (RADAR_RADIUS * 2) / 3, theme.radarRingInner);
    backbuffer.drawCircle(RADAR_CENTRE_X, RADAR_CENTRE_Y, RADAR_RADIUS / 3, theme.radarRingInner);

    // Center home crosshair marker
    backbuffer.fillCircle(RADAR_CENTRE_X, RADAR_CENTRE_Y, 3, theme.textAccent);

    // Cardinal compass labels
    UseFont(backbuffer, 1);
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(theme.textMuted);
    backbuffer.drawCentreString("N", RADAR_CENTRE_X, RADAR_Y + 6);
    backbuffer.drawCentreString("S", RADAR_CENTRE_X, RADAR_Y + RADAR_SIZE - 14);
    backbuffer.drawString("W", RADAR_X + 6, RADAR_CENTRE_Y - 4);
    backbuffer.drawRightString("E", RADAR_X + RADAR_SIZE - 6, RADAR_CENTRE_Y - 4);

    // Range ring distance label
    float outerKm = static_cast<float>(rad * 111.32);
    String rangeLabel = String(ConvertDistance(outerKm), outerKm < 15.0f ? 1 : 0) + " " + DistUnit();
    backbuffer.drawString(rangeLabel, RADAR_CENTRE_X + 6, RADAR_Y + RADAR_SIZE / 3 - 10);
}

void AircraftManager::DrawAircraftTrail(LGFX_Sprite& backbuffer, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected) const
{
    if (tracked.trailCount < 2) return;
    uint32_t col = isSelected ? theme.routeLine : theme.aircraftTrail;

    for (int i = 1; i < tracked.trailCount; ++i) {
        auto [x0, y0] = ProjectCoordinateToScreen(tracked.trail[i - 1].lat, tracked.trail[i - 1].lon);
        auto [x1, y1] = ProjectCoordinateToScreen(tracked.trail[i].lat, tracked.trail[i].lon);
        backbuffer.drawLine(x0, y0, x1, y1, col);
    }
    auto [lastX, lastY] = ProjectCoordinateToScreen(tracked.trail[tracked.trailCount - 1].lat, tracked.trail[tracked.trailCount - 1].lon);
    auto [curLat, curLon] = tracked.GetDisplayPosition();
    auto [curX, curY] = ProjectCoordinateToScreen(curLat, curLon);
    backbuffer.drawLine(lastX, lastY, curX, curY, col);
}

void AircraftManager::DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected) const
{
    const float speed = ConvertSpeed(tracked.state.velocity);
    const float alt = ConvertAltitude(tracked.state.baroAltitude);

    UseFont(backbuffer, 1);
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(isSelected ? theme.textPrimary : theme.textSecondary);

    String label = !tracked.state.callsign.isEmpty() ? tracked.state.callsign : tracked.state.icao24;
    if (!tracked.state.typeCode.isEmpty()) {
        label += " · " + tracked.state.typeCode;
    }
    backbuffer.drawString(label, x + 8, y - 6);

    backbuffer.setTextColor(theme.textMuted);
    String sub = String(alt, 0) + AltUnit() + " " + String(speed, 0) + SpeedUnit();
    backbuffer.drawString(sub, x + 8, y + 4);
}

void AircraftManager::DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected, bool isPinned) const
{
    const float dx = std::sin(radians(tracked.state.trueTrack));
    const float dy = -std::cos(radians(tracked.state.trueTrack));
    const float px = -dy;
    const float py = dx;

    const float len = isSelected ? 8.0f : 6.5f;
    const float wid = isSelected ? 4.5f : 3.5f;

    const float tipX = x + dx * len;
    const float tipY = y + dy * len;
    const float leftX = x - dx * len * 0.5f + px * wid;
    const float leftY = y - dy * len * 0.5f + py * wid;
    const float rightX = x - dx * len * 0.5f - px * wid;
    const float rightY = y - dy * len * 0.5f - py * wid;

    uint32_t col = isPinned ? theme.textPinned : (isSelected ? theme.aircraftSelected : theme.aircraftDot);
    backbuffer.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, col);

    if (isSelected || isPinned) {
        backbuffer.drawCircle(x, y, 12, col);
        if (isPinned) {
            backbuffer.drawCircle(x, y, 15, theme.textWarning);
        }
    }
}

void AircraftManager::DrawSplitRadarView(LGFX_Sprite& backbuffer, const ThemePalette& theme)
{
#ifdef CROWPANEL_7
    backbuffer.setClipRect(RADAR_X, RADAR_Y, RADAR_SIZE, RADAR_SIZE);
#endif
    DrawRadarCircles(backbuffer, theme);

    if (displayScanline) {
        DrawScanLines(
            backbuffer,
            RADAR_CENTRE_X,
            RADAR_CENTRE_Y,
            RADAR_CENTRE_X + (std::cos(millis() / 3000.0f) * RADAR_RADIUS),
            RADAR_CENTRE_Y + (std::sin(millis() / 3000.0f) * RADAR_RADIUS),
            20, 140, 5, RADAR_RADIUS,
            theme.radarSweepR, theme.radarSweepG, theme.radarSweepB
        );
    }

    for (auto& [icao, tracked] : trackedAircraft) {
        bool isPinned = (!pinnedIcao.isEmpty() && icao == pinnedIcao);
        if (tracked.state.onGround && !isPinned) continue;

        tracked.Tick();
        bool isSelected = (icao == selectedIcao);

        DrawAircraftTrail(backbuffer, tracked, theme, isSelected);

        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);

        if (displayInfoText) {
            DrawAircraftInfo(backbuffer, x, y, tracked, theme, isSelected);
        }

        if (displayTriangles) {
            DrawAircraftTriangle(backbuffer, x, y, tracked, theme, isSelected, isPinned);
        } else {
            backbuffer.fillCircle(x, y, isSelected ? 4 : 3, isSelected ? theme.aircraftSelected : theme.aircraftDot);
        }
    }

#ifdef CROWPANEL_7
    backbuffer.clearClipRect();

    // Outer frame border around radar box
    backbuffer.drawRoundRect(RADAR_X, RADAR_Y, RADAR_SIZE, RADAR_SIZE, 8, theme.borderSecondary);

    // Top-left radar overlay badge: active flight count
    backbuffer.fillRoundRect(RADAR_X + 8, RADAR_Y + 8, 108, 22, 4, theme.bgHeader);
    backbuffer.drawRoundRect(RADAR_X + 8, RADAR_Y + 8, 108, 22, 4, theme.borderSecondary);
    UseFont(backbuffer, 1);
    backbuffer.setTextColor(theme.textPrimary);
    backbuffer.drawString(String(GetSortedActiveFlights().size()) + " AIRBORNE", RADAR_X + 16, RADAR_Y + 15);

    if (!pinnedQuery.isEmpty()) {
        backbuffer.fillRoundRect(RADAR_X + RADAR_SIZE - 124, RADAR_Y + 8, 116, 22, 4, theme.bgHeader);
        backbuffer.drawRoundRect(RADAR_X + RADAR_SIZE - 124, RADAR_Y + 8, 116, 22, 4, theme.textPinned);
        backbuffer.setTextColor(theme.textPinned);
        backbuffer.drawCentreString("PIN: " + pinnedQuery, RADAR_X + RADAR_SIZE - 66, RADAR_Y + 15);
    }

    if (IsDetailOpen()) {
        DrawFlightDetail(backbuffer);
    } else {
        DrawIdleWelcomePanel(backbuffer, theme);
    }
#endif
}

void AircraftManager::DrawIdleWelcomePanel(LGFX_Sprite& buf, const ThemePalette& theme) const
{
    const int px = 452, py = 8, pw = 340, ph = 428;

    buf.fillRoundRect(px, py, pw, ph, 8, theme.bgCard);
    buf.drawRoundRect(px, py, pw, ph, 8, theme.borderPrimary);

    // Header banner
    buf.fillRoundRect(px + 2, py + 2, pw - 4, 54, 6, theme.bgHeader);
    UseFont(buf, 4);
    buf.setTextSize(1);
    buf.setTextColor(theme.isLight ? lgfx::color888(250, 250, 250) : theme.textPrimary);
    buf.drawString(TruncateToWidth(buf, giftTitle, pw - 24), px + 14, py + 8);

    UseFont(buf, 2);
    buf.setTextColor(theme.isLight ? lgfx::color888(210, 220, 235) : theme.textSecondary);
    buf.drawString(TruncateToWidth(buf, giftSubtitle, pw - 24), px + 14, py + 34);

    int y = py + 68;
    const int x = px + 14;

    // Status message
    UseFont(buf, 2);
    if (!HasLocationConfigured()) {
        buf.setTextColor(theme.textWarning);
        buf.drawString("Setup Required: Scan QR Below", x, y);
        y += 20;
        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        buf.drawString("Set home coordinates with 1 tap on your phone", x, y);
        y += 16;
    } else {
        buf.setTextColor(theme.textAccent);
        size_t activeCount = GetSortedActiveFlights().size();
        if (activeCount == 0) {
            buf.drawString("Scanning local airspace...", x, y);
        } else {
            buf.drawString(String(activeCount) + " aircraft in range — tap to inspect", x, y);
        }
        y += 20;
        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        char coordBuf[64];
        float rangeDist = ConvertDistance(static_cast<float>(rad * 111.32));
        snprintf(coordBuf, sizeof(coordBuf), "Home: %.3f, %.3f  |  Range: %.0f %s",
            lat, lon, rangeDist, DistUnit().c_str());
        buf.drawString(coordBuf, x, y);
        y += 16;
    }

    buf.drawFastHLine(x, y, pw - 28, theme.borderSecondary);
    y += 10;

    // Mini Spotter Stats summary card
    buf.fillRoundRect(x, y, pw - 28, 76, 6, theme.bgSecondary);
    buf.drawRoundRect(x, y, pw - 28, 76, 6, theme.borderSecondary);
    UseFont(buf, 1);
    buf.setTextColor(theme.textAccent);
    buf.drawString("SESSION SPOTTER SUMMARY", x + 10, y + 8);

    UseFont(buf, 2);
    buf.setTextColor(theme.textPrimary);
    buf.drawString("Spotted: " + String(stats.totalUniqueSeen), x + 10, y + 24);
    if (stats.closestDistKm < 99999.0f) {
        String closeStr = "Closest: " + String(ConvertDistance(stats.closestDistKm), 1) + DistUnit() + " (" + stats.closestCallsign + ")";
        buf.drawString(TruncateToWidth(buf, closeStr, pw - 48), x + 10, y + 46);
    } else {
        buf.setTextColor(theme.textMuted);
        buf.drawString("Waiting for first flyover...", x + 10, y + 46);
    }
    y += 88;

    // Phone Setup & Live Remote Control QR Code box
    buf.fillRoundRect(x, y, pw - 28, 170, 6, theme.bgSecondary);
    buf.drawRoundRect(x, y, pw - 28, 170, 6, theme.borderSecondary);

    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "microradar.local";
    String qrUrl = "http://" + ipStr + "/";

    // White quiet-zone plate for scannable QR code
    const int qrBoxSize = 128;
    const int qrX = x + 12;
    const int qrY = y + 21;
    buf.fillRoundRect(qrX - 6, qrY - 6, qrBoxSize + 12, qrBoxSize + 12, 4, lgfx::color888(255, 255, 255));
    buf.qrcode(qrUrl, qrX, qrY, qrBoxSize, 3);

    int infoX = qrX + qrBoxSize + 16;
    int infoW = (px + pw - 18) - infoX;
    UseFont(buf, 2);
    buf.setTextColor(theme.textPrimary);
    buf.drawString("Phone Remote", infoX, y + 18);
    buf.drawString("& Easy Setup", infoX, y + 36);

    UseFont(buf, 1);
    buf.setTextColor(theme.textSecondary);
    DrawWrappedText(buf, infoX, y + 60, "Scan QR to switch themes, pin a flight, or set GPS location.", infoW, 13, 4);

    buf.setTextColor(theme.textAccent);
    buf.drawString("microradar.local", infoX, y + 124);
    buf.setTextColor(theme.textMuted);
    buf.drawString(ipStr, infoX, y + 138);
}

void AircraftManager::DrawFlightDetail(LGFX_Sprite& buf)
{
    if (selectedIcao.isEmpty()) return;

    auto it = trackedAircraft.find(selectedIcao);
    if (it == trackedAircraft.end()) return;

    const TrackedAircraft& tracked = it->second;
    const ThemePalette& theme = GetTheme();

    const int px = 452, py = 8, pw = 340, ph = 428;
    const int x = px + 12;
    const int innerW = pw - 24;

    buf.fillRoundRect(px, py, pw, ph, 8, theme.bgCard);
    buf.drawRoundRect(px, py, pw, ph, 8, theme.borderPrimary);

    AirlineInfo airline = AviationCatalog::LookupAirlineInfo(tracked.state.callsign, tracked.state.operatorName);
    AircraftTypeInfo typeInfo = AviationCatalog::LookupAircraftType(
        tracked.state.typeCode,
        tracked.state.typeDescription,
        tracked.state.emitterCategory
    );
    bool isPinned = (!pinnedIcao.isEmpty() && selectedIcao == pinnedIcao);

    // 1. Top Header Strip: Carrier Logo Badge + Callsign/Airline + Close Button
    buf.fillRoundRect(px + 2, py + 2, pw - 4, 52, 6, theme.bgHeader);
    mediaCache.DrawCarrierBadge(buf, x, py + 10, 74, 34, airline, theme);

    int titleX = x + 82;
    UseFont(buf, 4);
    buf.setTextSize(1);
    buf.setTextColor(theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
    String mainCall = !tracked.state.callsign.isEmpty() ? tracked.state.callsign : selectedIcao;
    buf.drawString(mainCall, titleX, py + 7);

    // Close / Pin pill in top right
    if (isPinned) {
        buf.fillRoundRect(px + pw - 74, py + 8, 62, 18, 4, theme.textPinned);
        UseFont(buf, 1);
        buf.setTextColor(lgfx::color888(15, 15, 20));
        buf.drawCentreString("PINNED", px + pw - 43, py + 13);
    } else if (typeInfo.isRareCatch && !typeInfo.rareBadge.isEmpty()) {
        buf.fillRoundRect(px + pw - 86, py + 8, 74, 18, 4, theme.textWarning);
        UseFont(buf, 1);
        buf.setTextColor(lgfx::color888(15, 15, 20));
        buf.drawCentreString(typeInfo.rareBadge, px + pw - 49, py + 13);
    } else {
        buf.drawRoundRect(px + pw - 58, py + 8, 46, 18, 4, theme.borderSecondary);
        UseFont(buf, 1);
        buf.setTextColor(theme.isLight ? lgfx::color888(220, 225, 235) : theme.textMuted);
        buf.drawCentreString("CLOSE", px + pw - 35, py + 13);
    }

    UseFont(buf, 2);
    buf.setTextColor(theme.isLight ? lgfx::color888(215, 225, 240) : theme.textSecondary);
    buf.drawString(TruncateToWidth(buf, airline.name, pw - 98), titleX, py + 32);

    int y = py + 60;

    // 2. Hero Aircraft Photo / Vector Silhouette Box (316 x 114)
    const int heroH = 114;
    mediaCache.DrawAircraftVisual(
        buf, x, y, innerW, heroH,
        selectedIcao, tracked.state.registration,
        typeInfo, theme, true
    );

    // Distance & Bearing overlay badge on top-right of hero image
    if (HasLocationConfigured()) {
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        float distKm = AviationCatalog::ComputeDistanceKm(lat, lon, predLat, predLon);
        float brg = AviationCatalog::ComputeBearingDeg(lat, lon, predLat, predLon);
        String distText = String(ConvertDistance(distKm), distKm < 10.0f ? 1 : 0) + " " + DistUnit() + " " + AviationCatalog::BearingToCardinal(brg);

        UseFont(buf, 1);
        int pillW = max(58, static_cast<int>(buf.textWidth(distText)) + 16);
        buf.fillRoundRect(x + innerW - pillW - 6, y + 20, pillW, 18, 4, theme.bgHeader);
        buf.drawRoundRect(x + innerW - pillW - 6, y + 20, pillW, 18, 4, theme.borderPrimary);
        buf.setTextColor(theme.isLight ? lgfx::color888(255, 255, 255) : theme.textAccent);
        buf.drawCentreString(distText, x + innerW - pillW / 2 - 6, y + 24);
    }

    y += heroH + 8;

    // 3. AvioFrame Route Card (Origin -> Destination + Progress Bar)
    const int routeH = 98;
    buf.fillRoundRect(x, y, innerW, routeH, 6, theme.bgSecondary);
    buf.drawRoundRect(x, y, innerW, routeH, 6, theme.borderSecondary);

    String depCode = !selectedDetail.departureAirport.isEmpty() ? selectedDetail.departureAirport : "---";
    String arrCode = !selectedDetail.arrivalAirport.isEmpty() ? selectedDetail.arrivalAirport : "---";

    UseFont(buf, 4);
    buf.setTextColor(theme.textPrimary);
    buf.drawString(depCode, x + 12, y + 8);
    buf.drawRightString(arrCode, x + innerW - 12, y + 8);

    // Compute route progress percentage if coordinates exist
    auto [curLat, curLon] = tracked.GetDisplayPosition();
    float totalKm = 0.0f, remKm = 0.0f;
    float progressPct = AviationCatalog::ComputeRouteProgressPct(
        selectedDetail.depLat, selectedDetail.depLon,
        selectedDetail.arrLat, selectedDetail.arrLon,
        curLat, curLon, totalKm, remKm
    );

    VectorArt::DrawRouteProgressBar(buf, x + 76, y + 14, innerW - 152, progressPct, theme);

    // Progress % or Flight code above/below bar
    UseFont(buf, 1);
    buf.setTextColor(theme.textAccent);
    if (progressPct >= 0.0f && remKm > 0.0f) {
        String remText = String(ConvertDistance(remKm), 0) + " " + DistUnit() + " left";
        if (tracked.state.velocity > 25.0f) {
            int remMins = static_cast<int>((remKm * 1000.0f) / (tracked.state.velocity * 60.0f));
            if (remMins >= 60) {
                remText += " | " + String(remMins / 60) + "h" + String(remMins % 60) + "m";
            } else if (remMins > 0) {
                remText += " | " + String(remMins) + "m";
            }
        }
        buf.drawCentreString(remText, x + innerW / 2, y + 31);
    } else {
        buf.setTextColor(theme.textMuted);
        buf.drawCentreString(
            selectedDetail.loaded ? "DIRECT ROUTE" : (selectedDetail.attempted ? "LOCAL TRACK" : "RESOLVING ROUTE..."),
            x + innerW / 2, y + 31
        );
    }

    // Full airport / city names
    UseFont(buf, 2);
    buf.setTextColor(theme.textSecondary);
    String depLabel = !selectedDetail.departureCity.isEmpty()
        ? selectedDetail.departureCity
        : (!selectedDetail.departureAirportFull.isEmpty() ? selectedDetail.departureAirportFull : "Origin");
    String arrLabel = !selectedDetail.arrivalCity.isEmpty()
        ? selectedDetail.arrivalCity
        : (!selectedDetail.arrivalAirportFull.isEmpty() ? selectedDetail.arrivalAirportFull : "Destination");

    buf.drawString(TruncateToWidth(buf, depLabel, innerW / 2 - 14), x + 10, y + 48);
    String arrTrunc = TruncateToWidth(buf, arrLabel, innerW / 2 - 14);
    buf.drawRightString(arrTrunc, x + innerW - 10, y + 48);

    // Full airport subtitle line
    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    if (!selectedDetail.departureAirportFull.isEmpty()) {
        buf.drawString(TruncateToWidth(buf, selectedDetail.departureAirportFull, innerW - 20), x + 10, y + 68);
    }
    if (!selectedDetail.arrivalAirportFull.isEmpty()) {
        buf.drawString(TruncateToWidth(buf, "To: " + selectedDetail.arrivalAirportFull, innerW - 20), x + 10, y + 81);
    }

    y += routeH + 8;

    // 4. Telemetry 2x2 Grid (Altitude, Ground Speed, Heading, Registration/ICAO)
    const int cellW = (innerW - 8) / 2;
    const int cellH = 44;

    auto drawTelemetryCell = [&](int cx, int cy, const char* label, const String& val, const String& subVal) {
        buf.fillRoundRect(cx, cy, cellW, cellH, 5, theme.bgSecondary);
        buf.drawRoundRect(cx, cy, cellW, cellH, 5, theme.borderSecondary);
        UseFont(buf, 1);
        int subW = subVal.isEmpty() ? 0 : (static_cast<int>(buf.textWidth(subVal)) + 10);
        buf.setTextColor(theme.textMuted);
        buf.drawString(TruncateToWidth(buf, label, cellW - 16 - subW), cx + 8, cy + 5);
        if (!subVal.isEmpty()) {
            buf.setTextColor(theme.textAccent);
            buf.drawRightString(subVal, cx + cellW - 8, cy + 5);
        }
        UseFont(buf, 2);
        buf.setTextColor(theme.textPrimary);
        buf.drawString(TruncateToWidth(buf, val, cellW - 16), cx + 8, cy + 21);
    };

    const float alt = ConvertAltitude(tracked.state.baroAltitude);
    const float spd = ConvertSpeed(tracked.state.velocity);
    String vRateStr = "";
    if (std::abs(tracked.state.verticalRate) >= 0.5f) {
        float vr = ConvertAltitude(tracked.state.verticalRate) * 60.0f; // per min
        vRateStr = String(vr > 0 ? "+" : "") + String(vr, 0) + "/m";
    }

    drawTelemetryCell(x, y, "ALTITUDE", String(alt, 0) + " " + AltUnit(), vRateStr);
    drawTelemetryCell(x + cellW + 8, y, "GROUND SPEED", String(spd, 0) + " " + SpeedUnit(), airline.formattedFlight);

    y += cellH + 6;
    String hdgCard = AviationCatalog::BearingToCardinal(tracked.state.trueTrack);
    String regOrHex = !tracked.state.registration.isEmpty() ? tracked.state.registration : selectedIcao;
    regOrHex.toUpperCase();
    String sqkStr = !tracked.state.squawk.isEmpty() ? ("SQK " + tracked.state.squawk) : ("HEX " + selectedIcao);

    drawTelemetryCell(x, y, "HEADING", String(tracked.state.trueTrack, 0) + " deg", hdgCard);
    drawTelemetryCell(x + cellW + 8, y, "TAIL / SQUAWK", regOrHex, sqkStr);

    UseFont(buf, 1);
}

void AircraftManager::DrawDepartureBoardView(LGFX_Sprite& buf, const ThemePalette& theme)
{
    // Full-screen 800x436 Retro Split-Flap / LED Airport Departure Board
    const int bx = 8, by = 8, bw = 784, bh = 428;

    buf.fillRoundRect(bx, by, bw, bh, 8, theme.bgCard);
    buf.drawRoundRect(bx, by, bw, bh, 8, theme.borderPrimary);

    // Top Terminal Header Banner
    buf.fillRoundRect(bx + 2, by + 2, bw - 4, 44, 6, theme.bgHeader);

    // Flap-style departure icon box
    buf.fillRoundRect(bx + 12, by + 9, 32, 30, 4, theme.textAccent);
    UseFont(buf, 2);
    buf.setTextColor(lgfx::color888(15, 15, 20));
    buf.drawCentreString("AIR", bx + 28, by + 16);

    UseFont(buf, 4);
    buf.setTextColor(theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
    buf.drawString(TruncateToWidth(buf, giftTitle + " | LIVE OVERHEAD BOARD", 520), bx + 54, by + 11);

    // Live UTC / Local clock & flight count on right of header
    time_t nowUtc = time(nullptr);
    char clockText[32] = "LIVE RADAR";
    if (nowUtc > 1700000000) {
        time_t localT = nowUtc + (tzOffsetMinutes * 60);
        struct tm tmbuf;
        gmtime_r(&localT, &tmbuf);
        strftime(clockText, sizeof(clockText), "%H:%M:%S", &tmbuf);
    }
    UseFont(buf, 4);
    buf.setTextColor(theme.textAccent);
    buf.drawRightString(clockText, bx + bw - 14, by + 11);

    // Column Header Bar
    const int colY = by + 50;
    buf.fillRect(bx + 4, colY, bw - 8, 22, theme.bgSecondary);
    buf.drawFastHLine(bx + 4, colY + 22, bw - 8, theme.borderSecondary);

    const int cCarrier = bx + 14;
    const int cFlight  = bx + 166;
    const int cRoute   = bx + 266;
    const int cType    = bx + 426;
    const int cAlt     = bx + 538;
    const int cSpeed   = bx + 628;
    const int cDist    = bx + 704;

    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    buf.drawString("CARRIER",       cCarrier, colY + 7);
    buf.drawString("FLIGHT",        cFlight,  colY + 7);
    buf.drawString("ORIGIN > DEST", cRoute,   colY + 7);
    buf.drawString("AIRCRAFT",      cType,    colY + 7);
    buf.drawString("ALTITUDE",      cAlt,     colY + 7);
    buf.drawString("SPEED",         cSpeed,   colY + 7);
    buf.drawString("DISTANCE",      cDist,    colY + 7);

    auto list = GetSortedActiveFlights();
    const int rowStartY = by + 76;
    const int rowH = 48;
    const int maxRows = 7;

    if (list.empty()) {
        UseFont(buf, 4);
        buf.setTextColor(theme.textMuted);
        buf.drawCentreString("NO AIRCRAFT CURRENTLY IN RANGE", bx + bw / 2, by + bh / 2 - 20);
        UseFont(buf, 2);
        buf.setTextColor(theme.textSecondary);
        buf.drawCentreString("Increase zoom radius or pin a flight from http://microradar.local", bx + bw / 2, by + bh / 2 + 16);
        UseFont(buf, 1);
        return;
    }

    // Passively queue route lookup for the top un-attempted row every cooldown interval
    unsigned long now = millis();
    if (now - lastRouteLookupMs >= ROUTE_LOOKUP_COOLDOWN_MS) {
        for (size_t i = 0; i < min((size_t)maxRows, list.size()); ++i) {
            const String& icao = list[i].icao;
            auto cached = flightDetailCache.find(icao);
            if (cached == flightDetailCache.end() && !net.IsInflight("detail:" + icao)) {
                lastRouteLookupMs = now;
                RequestFlightDetail(icao, false, NetWorker::PRIO_PREFETCH);
                break;
            }
        }
    }

    for (int i = 0; i < maxRows && i < (int)list.size(); ++i) {
        const auto& item = list[i];
        auto it = trackedAircraft.find(item.icao);
        if (it == trackedAircraft.end()) continue;
        const TrackedAircraft& tracked = it->second;

        int ry = rowStartY + i * rowH;
        bool isSel = (item.icao == selectedIcao);

        // Split-flap row background tile
        uint32_t rowBg = isSel ? theme.bgHeader : ((i % 2 == 0) ? theme.bgCard : theme.bgRowAlt);
        buf.fillRoundRect(bx + 8, ry + 2, bw - 16, rowH - 4, 4, rowBg);
        buf.drawRoundRect(bx + 8, ry + 2, bw - 16, rowH - 4, 4, item.isPinned ? theme.textPinned : (isSel ? theme.borderPrimary : theme.borderSecondary));

        // Mechanical split-flap horizontal hinge line across the middle of the row
        buf.drawFastHLine(bx + 10, ry + rowH / 2, bw - 20, theme.gridLine);

        AirlineInfo airline = AviationCatalog::LookupAirlineInfo(tracked.state.callsign, tracked.state.operatorName);
        AircraftTypeInfo typeInfo = AviationCatalog::LookupAircraftType(
            tracked.state.typeCode,
            tracked.state.typeDescription,
            tracked.state.emitterCategory
        );

        // 1. Carrier Badge + Name
        mediaCache.DrawCarrierBadge(buf, cCarrier, ry + 8, 64, 32, airline, theme);
        UseFont(buf, 2);
        buf.setTextColor(isSel && theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
        buf.drawString(TruncateToWidth(buf, airline.name, 80), cCarrier + 70, ry + 9);

        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        String regOrHex = !tracked.state.registration.isEmpty() ? tracked.state.registration : item.icao;
        regOrHex.toUpperCase();
        if (tracked.state.registration.isEmpty()) regOrHex = "ICAO " + regOrHex;
        buf.drawString(regOrHex, cCarrier + 70, ry + 27);

        // 2. Flight / Callsign Flap
        String callStr = !tracked.state.callsign.isEmpty() ? tracked.state.callsign : regOrHex;
        buf.fillRoundRect(cFlight, ry + 8, 92, 32, 3, theme.bgSecondary);
        buf.drawRoundRect(cFlight, ry + 8, 92, 32, 3, theme.borderSecondary);
        UseFont(buf, 2);
        buf.setTextColor(item.isPinned ? theme.textPinned : theme.textAccent);
        buf.drawString(callStr, cFlight + 8, ry + 16);

        // 3. Route Flap (DEP > ARR)
        buf.fillRoundRect(cRoute, ry + 8, 152, 32, 3, theme.bgSecondary);
        buf.drawRoundRect(cRoute, ry + 8, 152, 32, 3, theme.borderSecondary);
        auto routeIt = flightDetailCache.find(item.icao);
        if (routeIt != flightDetailCache.end() && routeIt->second.loaded) {
            String dep = !routeIt->second.departureAirport.isEmpty() ? routeIt->second.departureAirport : "---";
            String arr = !routeIt->second.arrivalAirport.isEmpty() ? routeIt->second.arrivalAirport : "---";
            UseFont(buf, 2);
            buf.setTextColor(theme.textPrimary);
            buf.drawString(dep + "  >  " + arr, cRoute + 10, ry + 7);

            UseFont(buf, 1);
            buf.setTextColor(theme.textMuted);
            String cityPair = (!routeIt->second.departureCity.isEmpty() ? routeIt->second.departureCity : dep)
                            + " to "
                            + (!routeIt->second.arrivalCity.isEmpty() ? routeIt->second.arrivalCity : arr);
            buf.drawString(TruncateToWidth(buf, cityPair, 134), cRoute + 10, ry + 25);
        } else if (routeIt == flightDetailCache.end()) {
            UseFont(buf, 1);
            buf.setTextColor(theme.textMuted);
            buf.drawString("Looking up route...", cRoute + 10, ry + 18);
        } else {
            UseFont(buf, 1);
            buf.setTextColor(theme.textMuted);
            buf.drawString("Local / Unscheduled", cRoute + 10, ry + 18);
        }

        // 4. Aircraft Type
        UseFont(buf, 2);
        buf.setTextColor(isSel && theme.isLight ? lgfx::color888(255, 255, 255) : theme.textSecondary);
        String modelShort = !tracked.state.typeCode.isEmpty() ? tracked.state.typeCode : "----";
        buf.drawString(modelShort, cType, ry + 8);
        UseFont(buf, 1);
        buf.setTextColor(typeInfo.isRareCatch ? theme.textWarning : theme.textMuted);
        buf.drawString(TruncateToWidth(buf, typeInfo.fullName, 104), cType, ry + 27);

        // 5. Altitude
        float alt = ConvertAltitude(tracked.state.baroAltitude);
        UseFont(buf, 2);
        buf.setTextColor(isSel && theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
        buf.drawString(String(alt, 0) + " " + AltUnit(), cAlt, ry + 9);
        UseFont(buf, 1);
        if (std::abs(tracked.state.verticalRate) > 0.6f) {
            buf.setTextColor(tracked.state.verticalRate > 0 ? theme.textAccent : theme.textWarning);
            buf.drawString(tracked.state.verticalRate > 0 ? "^ CLIMB" : "v DESC", cAlt, ry + 27);
        } else {
            buf.setTextColor(theme.textMuted);
            buf.drawString("LEVEL", cAlt, ry + 27);
        }

        // 6. Ground Speed
        float spd = ConvertSpeed(tracked.state.velocity);
        UseFont(buf, 2);
        buf.setTextColor(isSel && theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
        buf.drawString(String(spd, 0) + " " + SpeedUnit(), cSpeed, ry + 9);
        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        buf.drawString(String(tracked.state.trueTrack, 0) + " deg", cSpeed, ry + 27);

        // 7. Distance & Compass Bearing
        UseFont(buf, 2);
        buf.setTextColor(theme.textAccent);
        float dVal = ConvertDistance(item.distKm);
        buf.drawString(String(dVal, dVal < 10.0f ? 1 : 0) + " " + DistUnit(), cDist, ry + 9);
        UseFont(buf, 1);
        buf.setTextColor(theme.textSecondary);
        buf.drawString(String(AviationCatalog::BearingToCardinal(item.bearingDeg)) + (item.isPinned ? " PIN" : ""), cDist, ry + 27);
    }

    // Bottom ticker inside board
    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    buf.drawString("TAP ANY FLIGHT ROW TO OPEN FULL AVIOFRAME POSTER", bx + 14, by + bh - 14);
    String rightStat = "SESSION: " + String(stats.totalUniqueSeen) + " SPOTTED";
    if (stats.closestDistKm < 99999.0f) {
        rightStat += "  |  CLOSEST: " + String(ConvertDistance(stats.closestDistKm), 1) + " " + DistUnit() + " (" + stats.closestCallsign + ")";
    }
    buf.drawRightString(rightStat, bx + bw - 14, by + bh - 14);
}

void AircraftManager::DrawPortraitPosterView(LGFX_Sprite& buf, const ThemePalette& theme)
{
    // Full-screen 800x436 AvioFrame Gallery Portrait / Spotter Card Poster
    const int bx = 8, by = 8, bw = 784, bh = 428;

    buf.fillRoundRect(bx, by, bw, bh, 8, theme.bgCard);
    buf.drawRoundRect(bx, by, bw, bh, 8, theme.borderPrimary);

    // Ensure a flight is selected if any exist
    if (selectedIcao.isEmpty() || trackedAircraft.find(selectedIcao) == trackedAircraft.end()) {
        auto list = GetSortedActiveFlights();
        if (!list.empty()) {
            selectedIcao = list[0].icao;
            auto cached = flightDetailCache.find(selectedIcao);
            if (cached != flightDetailCache.end() && cached->second.attempted) {
                selectedDetail = cached->second;
            } else if (millis() - lastRouteLookupMs >= ROUTE_LOOKUP_COOLDOWN_MS) {
                PopulateFlightDetail(selectedIcao, false);
            }
        }
    }

    auto it = trackedAircraft.find(selectedIcao);
    if (it == trackedAircraft.end()) {
        // Fallback when zero flights are in range
        DrawIdleWelcomePanel(buf, theme);
        // Draw left-side big silhouette art
        VectorArt::DrawAircraftSilhouette(buf, 16, 16, 424, 412, SIL_WIDEBODY_TWIN, theme, "A35K", giftTitle + " | " + giftSubtitle, "SCANNING SKIES", true);
        return;
    }

    const TrackedAircraft& tracked = it->second;
    AirlineInfo airline = AviationCatalog::LookupAirlineInfo(tracked.state.callsign, tracked.state.operatorName);
    AircraftTypeInfo typeInfo = AviationCatalog::LookupAircraftType(
        tracked.state.typeCode,
        tracked.state.typeDescription,
        tracked.state.emitterCategory
    );
    bool isPinned = (!pinnedIcao.isEmpty() && selectedIcao == pinnedIcao);

    // LEFT COLUMN (440px wide): Header + Large Hero Photo/Illustration + Origin/Destination Poster Banner
    const int lx = bx + 14;
    const int lw = 434;

    // Top Carrier & Callsign Banner
    buf.fillRoundRect(lx, by + 12, lw, 64, 6, theme.bgHeader);
    mediaCache.DrawCarrierBadge(buf, lx + 10, by + 20, 96, 48, airline, theme);

    int hTextX = lx + 118;
    UseFont(buf, 4);
    buf.setTextColor(theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
    String mainCall = !tracked.state.callsign.isEmpty() ? tracked.state.callsign : selectedIcao;
    buf.drawString(mainCall, hTextX, by + 18);

    if (!airline.formattedFlight.isEmpty() && airline.formattedFlight != mainCall) {
        UseFont(buf, 2);
        buf.setTextColor(theme.textAccent);
        buf.drawRightString(airline.formattedFlight, lx + lw - 12, by + 22);
    }

    UseFont(buf, 2);
    buf.setTextColor(theme.isLight ? lgfx::color888(215, 225, 240) : theme.textSecondary);
    buf.drawString(TruncateToWidth(buf, airline.name, lw - 130), hTextX, by + 48);

    // Hero Aircraft Visual (434 x 212)
    const int heroY = by + 84;
    const int heroH = 212;
    mediaCache.DrawAircraftVisual(
        buf, lx, heroY, lw, heroH,
        selectedIcao, tracked.state.registration,
        typeInfo, theme, true
    );

    // Pinned or Rare Catch badge ribbon on top-left of Hero box
    if (isPinned) {
        buf.fillRoundRect(lx + 10, heroY + 10, 118, 22, 4, theme.textPinned);
        UseFont(buf, 2);
        buf.setTextColor(lgfx::color888(15, 15, 20));
        buf.drawCentreString("PINNED FLIGHT", lx + 69, heroY + 13);
    } else if (typeInfo.isRareCatch && !typeInfo.rareBadge.isEmpty()) {
        buf.fillRoundRect(lx + 10, heroY + 10, 118, 22, 4, theme.textWarning);
        UseFont(buf, 2);
        buf.setTextColor(lgfx::color888(15, 15, 20));
        buf.drawCentreString(typeInfo.rareBadge, lx + 69, heroY + 13);
    }

    // Bottom Route Poster Card (434 x 116)
    const int routeY = heroY + heroH + 8;
    const int routeH = 116;
    buf.fillRoundRect(lx, routeY, lw, routeH, 6, theme.bgSecondary);
    buf.drawRoundRect(lx, routeY, lw, routeH, 6, theme.borderPrimary);

    String depCode = !selectedDetail.departureAirport.isEmpty() ? selectedDetail.departureAirport : "---";
    String arrCode = !selectedDetail.arrivalAirport.isEmpty() ? selectedDetail.arrivalAirport : "---";

    UseFont(buf, 6);
    buf.setTextSize(1);
    buf.setTextColor(theme.textPrimary);
    buf.drawString(depCode, lx + 14, routeY + 10);
    buf.drawRightString(arrCode, lx + lw - 14, routeY + 10);

    auto [curLat, curLon] = tracked.GetDisplayPosition();
    float totalKm = 0.0f, remKm = 0.0f;
    float progressPct = AviationCatalog::ComputeRouteProgressPct(
        selectedDetail.depLat, selectedDetail.depLon,
        selectedDetail.arrLat, selectedDetail.arrLon,
        curLat, curLon, totalKm, remKm
    );

    VectorArt::DrawRouteProgressBar(buf, lx + 118, routeY + 24, lw - 236, progressPct, theme);

    UseFont(buf, 1);
    buf.setTextColor(theme.textAccent);
    if (progressPct >= 0.0f) {
        String pctStr = String(progressPct, 0) + "% COMPLETE";
        if (remKm > 0.0f) {
            pctStr += "  |  " + String(ConvertDistance(remKm), 0) + " " + DistUnit() + " REMAINING";
        }
        buf.drawCentreString(pctStr, lx + lw / 2, routeY + 44);
    } else {
        buf.drawCentreString(selectedDetail.loaded ? "EN ROUTE" : "LIVE ADS-B TELEMETRY", lx + lw / 2, routeY + 44);
    }

    UseFont(buf, 2);
    buf.setTextColor(theme.textSecondary);
    String depName = !selectedDetail.departureAirportFull.isEmpty() ? selectedDetail.departureAirportFull : "Origin Airport";
    String arrName = !selectedDetail.arrivalAirportFull.isEmpty() ? selectedDetail.arrivalAirportFull : "Destination Airport";
    buf.drawString(TruncateToWidth(buf, "From: " + depName, lw - 28), lx + 14, routeY + 66);
    buf.drawString(TruncateToWidth(buf, "To:   " + arrName, lw - 28), lx + 14, routeY + 88);

    // RIGHT COLUMN (310px wide): Aircraft Specs + 6 Telemetry Cards + Mini Compass Locator + Prev/Next Buttons
    const int rx = lx + lw + 12;
    const int rw = (bx + bw - 14) - rx;

    // Aircraft Model & Registration Banner
    buf.fillRoundRect(rx, by + 12, rw, 54, 6, theme.bgSecondary);
    buf.drawRoundRect(rx, by + 12, rw, 54, 6, theme.borderSecondary);
    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    buf.drawString("AIRCRAFT MODEL", rx + 10, by + 18);
    String regBadge = !tracked.state.registration.isEmpty() ? ("REG " + tracked.state.registration) : ("HEX " + selectedIcao);
    regBadge.toUpperCase();
    buf.setTextColor(theme.textAccent);
    buf.drawRightString(regBadge, rx + rw - 10, by + 18);

    UseFont(buf, 2);
    buf.setTextColor(theme.textPrimary);
    buf.drawString(TruncateToWidth(buf, typeInfo.fullName, rw - 20), rx + 10, by + 34);

    // 3x2 Telemetry Grid
    const int tCellW = (rw - 8) / 2;
    const int tCellH = 54;
    int ty = by + 74;

    auto drawPosterStat = [&](int cx, int cy, const char* label, const String& bigVal, const String& unitOrSub) {
        buf.fillRoundRect(cx, cy, tCellW, tCellH, 5, theme.bgSecondary);
        buf.drawRoundRect(cx, cy, tCellW, tCellH, 5, theme.borderSecondary);
        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        buf.drawString(label, cx + 8, cy + 6);
        UseFont(buf, 4);
        buf.setTextColor(theme.textPrimary);
        buf.drawString(TruncateToWidth(buf, bigVal, tCellW - 14), cx + 8, cy + 20);
        UseFont(buf, 1);
        buf.setTextColor(theme.textAccent);
        buf.drawRightString(unitOrSub, cx + tCellW - 8, cy + 6);
    };

    float altVal = ConvertAltitude(tracked.state.baroAltitude);
    float spdVal = ConvertSpeed(tracked.state.velocity);
    float vrMin = ConvertAltitude(tracked.state.verticalRate) * 60.0f;
    float distKm = HasLocationConfigured() ? AviationCatalog::ComputeDistanceKm(lat, lon, curLat, curLon) : 0.0f;
    float brgDeg = HasLocationConfigured() ? AviationCatalog::ComputeBearingDeg(lat, lon, curLat, curLon) : 0.0f;

    drawPosterStat(rx,              ty, "ALTITUDE",      String(altVal, 0), AltUnit());
    drawPosterStat(rx + tCellW + 8, ty, "GROUND SPEED",  String(spdVal, 0), SpeedUnit());
    ty += tCellH + 8;

    drawPosterStat(rx,              ty, "VERTICAL RATE", String(vrMin > 0 ? "+" : "") + String(vrMin, 0), AltUnit() + "/m");
    drawPosterStat(rx + tCellW + 8, ty, "TRACK HEADING", String(tracked.state.trueTrack, 0) + " deg", AviationCatalog::BearingToCardinal(tracked.state.trueTrack));
    ty += tCellH + 8;

    // Mini Compass Radar Widget + Distance Card
    const int locH = 112;
    buf.fillRoundRect(rx, ty, rw, locH, 6, theme.bgSecondary);
    buf.drawRoundRect(rx, ty, rw, locH, 6, theme.borderSecondary);

    float maxRangeKm = static_cast<float>(rad * 111.32);
    VectorArt::DrawMiniCompassRadar(buf, rx + 58, ty + locH / 2, 44, brgDeg, distKm, maxRangeKm, tracked.state.trueTrack, theme);

    int locTextX = rx + 114;
    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    buf.drawString("DISTANCE FROM HOME", locTextX, ty + 12);

    UseFont(buf, 4);
    buf.setTextColor(theme.textAccent);
    float dConv = ConvertDistance(distKm);
    buf.drawString(String(dConv, dConv < 10.0f ? 1 : 0) + " " + DistUnit(), locTextX, ty + 28);

    UseFont(buf, 2);
    buf.setTextColor(theme.textPrimary);
    buf.drawString("Bearing: " + String(brgDeg, 0) + " deg (" + AviationCatalog::BearingToCardinal(brgDeg) + ")", locTextX, ty + 58);

    UseFont(buf, 1);
    buf.setTextColor(theme.textMuted);
    String sqkInfo = "ICAO: " + selectedIcao + (!tracked.state.squawk.isEmpty() ? ("  |  SQK: " + tracked.state.squawk) : "");
    buf.drawString(sqkInfo, locTextX, ty + 84);

    ty += locH + 8;

    // < PREV FLIGHT and NEXT FLIGHT > touch buttons
    const int btnW = (rw - 8) / 2;
    const int btnH = 38;
    buf.fillRoundRect(rx, ty, btnW, btnH, 5, theme.buttonBg);
    buf.drawRoundRect(rx, ty, btnW, btnH, 5, theme.buttonBorder);
    buf.fillRoundRect(rx + btnW + 8, ty, btnW, btnH, 5, theme.buttonBg);
    buf.drawRoundRect(rx + btnW + 8, ty, btnW, btnH, 5, theme.buttonBorder);

    UseFont(buf, 2);
    buf.setTextColor(theme.buttonText);
    buf.drawCentreString("< PREV FLIGHT", rx + btnW / 2, ty + 11);
    buf.drawCentreString("NEXT FLIGHT >", rx + btnW + 8 + btnW / 2, ty + 11);
    UseFont(buf, 1);
}

void AircraftManager::DrawStatsModal(LGFX_Sprite& buf, const ThemePalette& theme) const
{
    const int mx = 90, my = 36, mw = 620, mh = 376;

    // Shadow border & card background
    buf.fillRoundRect(mx + 4, my + 4, mw, mh, 10, lgfx::color888(0, 0, 0));
    buf.fillRoundRect(mx, my, mw, mh, 10, theme.bgCard);
    buf.drawRoundRect(mx, my, mw, mh, 10, theme.borderPrimary);

    buf.fillRoundRect(mx + 2, my + 2, mw - 4, 46, 8, theme.bgHeader);
    UseFont(buf, 4);
    buf.setTextColor(theme.isLight ? lgfx::color888(255, 255, 255) : theme.textPrimary);
    buf.drawString("SKY SPOTTER LOG & RECORDS", mx + 18, my + 12);

    UseFont(buf, 2);
    buf.setTextColor(theme.textAccent);
    buf.drawRightString("TAP TO CLOSE", mx + mw - 18, my + 16);

    // 4 Record Cards across the top
    const int cardY = my + 60;
    const int cardW = (mw - 36 - 24) / 4;
    const int cardH = 86;

    auto drawRecordCard = [&](int idx, const char* title, const String& bigVal, const String& sub) {
        int cx = mx + 18 + idx * (cardW + 8);
        buf.fillRoundRect(cx, cardY, cardW, cardH, 6, theme.bgSecondary);
        buf.drawRoundRect(cx, cardY, cardW, cardH, 6, theme.borderSecondary);
        UseFont(buf, 1);
        buf.setTextColor(theme.textMuted);
        buf.drawString(title, cx + 8, cardY + 8);
        UseFont(buf, 4);
        buf.setTextColor(theme.textAccent);
        buf.drawString(TruncateToWidth(buf, bigVal, cardW - 14), cx + 8, cardY + 26);
        UseFont(buf, 2);
        buf.setTextColor(theme.textPrimary);
        buf.drawString(TruncateToWidth(buf, sub, cardW - 14), cx + 8, cardY + 58);
    };

    drawRecordCard(0, "TOTAL SPOTTED", String(stats.totalUniqueSeen), String(GetSortedActiveFlights().size()) + " active now");

    String closeVal = (stats.closestDistKm < 99999.0f)
        ? (String(ConvertDistance(stats.closestDistKm), 1) + " " + DistUnit())
        : "---";
    drawRecordCard(1, "CLOSEST FLYOVER", closeVal, stats.closestCallsign.isEmpty() ? "None yet" : (stats.closestCallsign + " " + stats.closestType));

    String highVal = (stats.highestAltM > 0.0f)
        ? (String(ConvertAltitude(stats.highestAltM), 0) + " " + AltUnit())
        : "---";
    drawRecordCard(2, "HIGHEST ALT", highVal, stats.highestCallsign.isEmpty() ? "None yet" : (stats.highestCallsign + " " + stats.highestType));

    String fastVal = (stats.fastestSpeedMps > 0.0f)
        ? (String(ConvertSpeed(stats.fastestSpeedMps), 0) + " " + SpeedUnit())
        : "---";
    drawRecordCard(3, "FASTEST SPEED", fastVal, stats.fastestCallsign.isEmpty() ? "None yet" : (stats.fastestCallsign + " " + stats.fastestType));

    // Rare Catches & Notable Aircraft section
    int ry = cardY + cardH + 16;
    UseFont(buf, 2);
    buf.setTextColor(theme.textPrimary);
    buf.drawString("NOTABLE & RARE CATCHES (SUPERJUMBOS, HELICOPTERS, MILITARY)", mx + 18, ry);
    ry += 24;

    if (stats.rareCatches.empty()) {
        buf.fillRoundRect(mx + 18, ry, mw - 36, 130, 6, theme.bgSecondary);
        buf.drawRoundRect(mx + 18, ry, mw - 36, 130, 6, theme.borderSecondary);
        UseFont(buf, 2);
        buf.setTextColor(theme.textMuted);
        buf.drawCentreString("No rare catches logged yet this session.", mx + mw / 2, ry + 42);
        UseFont(buf, 1);
        buf.drawCentreString("Keep watching for Airbus A380s, Boeing 747s, Helicopters, and Military flights!", mx + mw / 2, ry + 70);
    } else {
        for (size_t i = 0; i < min((size_t)4, stats.rareCatches.size()); ++i) {
            const auto& rc = stats.rareCatches[i];
            int itemY = ry + i * 36;
            buf.fillRoundRect(mx + 18, itemY, mw - 36, 32, 4, theme.bgSecondary);
            buf.drawRoundRect(mx + 18, itemY, mw - 36, 32, 4, theme.borderSecondary);

            buf.fillRoundRect(mx + 24, itemY + 6, 92, 20, 3, theme.textWarning);
            UseFont(buf, 1);
            buf.setTextColor(lgfx::color888(15, 15, 20));
            buf.drawCentreString(rc.badge, mx + 70, itemY + 12);

            UseFont(buf, 2);
            buf.setTextColor(theme.textPrimary);
            buf.drawString(rc.callsign, mx + 126, itemY + 8);

            buf.setTextColor(theme.textSecondary);
            buf.drawString(rc.typeName, mx + 225, itemY + 8);

            buf.setTextColor(theme.textAccent);
            buf.drawRightString(String(ConvertDistance(rc.distKm), 1) + " " + DistUnit(), mx + mw - 28, itemY + 8);
        }
    }

    UseFont(buf, 1);
}

String AircraftManager::HitTestAircraft(int tapX, int tapY) const
{
#ifdef CROWPANEL_7
    if (tapX < RADAR_X || tapX >= RADAR_X + RADAR_SIZE ||
        tapY < RADAR_Y || tapY >= RADAR_Y + RADAR_SIZE) return "";
#endif
    String nearestIcao;
    float nearestDist = 1e9f;

    for (auto& [icao, tracked] : trackedAircraft) {
        bool isPinned = (!pinnedIcao.isEmpty() && icao == pinnedIcao);
        if (tracked.state.onGround && !isPinned) continue;
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [sx, sy] = ProjectCoordinateToScreen(predLat, predLon);

        const int textPadX = 75;
        const int textPadY = 32;
        const int bubblePad = 16;

        int left   = min(sx, sx + 5) - bubblePad;
        int right  = max(sx, sx + textPadX) + bubblePad;
        int top    = min(sy, sy + 5) - bubblePad;
        int bottom = max(sy, sy + textPadY) + bubblePad;

        if (tapX >= left && tapX <= right && tapY >= top && tapY <= bottom) {
            float ddx = sx - tapX;
            float ddy = sy - tapY;
            float dist = ddx * ddx + ddy * ddy;
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestIcao = icao;
            }
        }
    }
    return nearestIcao;
}

bool AircraftManager::FetchFlightDetailAirlabs(const String& icao, const String& callsign, FlightDetail& detail)
{
    // Runs on the NetWorker task. Must not touch UI-thread state.
    if (!airlabsManager.updateSingle(icao)) {
        Serial.println("[DETAIL] Airlabs hex lookup failed, trying callsign...");
    }

    String lookupIcao = icao;
    lookupIcao.toLowerCase();

    const auto& airlabsAircraft = airlabsManager.getAircraft();
    auto it = airlabsAircraft.find(lookupIcao);
    if (it != airlabsAircraft.end()) {
        const AirLabsAircraft& ac = it->second;
        detail.departureAirport = ac.dep_iata;
        detail.arrivalAirport = ac.arr_iata;
        ResolveAirportNames(detail);
        detail.departureTime = ac.dep_time;
        detail.arrivalTime = ac.arr_time;
        return true;
    }

    if (callsign.isEmpty()) return false;

    AirLabsAircraft flightData;
    if (!airlabsManager.lookupByCallsign(callsign, flightData)) {
        return false;
    }

    detail.departureAirport = flightData.dep_iata;
    detail.arrivalAirport = flightData.arr_iata;
    ResolveAirportNames(detail);
    detail.departureTime = flightData.dep_time;
    detail.arrivalTime = flightData.arr_time;
    return true;
}

void AircraftManager::ResolveAirportNames(FlightDetail& detail) const
{
    if (!airportDb) return;
    if (detail.departureAirportFull.isEmpty() && !detail.departureAirport.isEmpty()) {
        String name = airportDb->getAirportName(detail.departureAirport);
        if (!name.isEmpty()) detail.departureAirportFull = name;
    }
    if (detail.arrivalAirportFull.isEmpty() && !detail.arrivalAirport.isEmpty()) {
        String name = airportDb->getAirportName(detail.arrivalAirport);
        if (!name.isEmpty()) detail.arrivalAirportFull = name;
    }
}

bool AircraftManager::FetchFlightDetailAdsbdb(const String& callsign, const String& icao, bool needAirframe, FlightDetail& detail)
{
    // Runs on the NetWorker task. Airframe metadata is returned inside `detail`
    // and applied to the tracked aircraft later on the UI thread.

    String cs = callsign;
    cs.trim();
    if (cs.isEmpty() && !needAirframe) return false;

    char url[220];
    if (!cs.isEmpty()) {
        snprintf(url, sizeof(url), "https://api.adsbdb.com/v0/aircraft/%s?callsign=%s", icao.c_str(), cs.c_str());
    } else {
        snprintf(url, sizeof(url), "https://api.adsbdb.com/v0/aircraft/%s", icao.c_str());
    }

    HttpResult result = http.Get(url);
    if (!result.success || result.statusCode != 200) {
        if (!cs.isEmpty()) {
            snprintf(url, sizeof(url), "https://api.adsbdb.com/v0/callsign/%s", cs.c_str());
            result = http.Get(url);
            if (!result.success || result.statusCode != 200) return false;
        } else {
            return false;
        }
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result.response);
    if (error) return false;

    // Extract airframe metadata if present
    JsonObject acObj = doc["response"]["aircraft"];
    if (!acObj.isNull()) {
        detail.afReg = acObj["registration"] | "";
        detail.afType = acObj["icao_type"] | "";
        detail.afTypeDesc = acObj["type"] | "";
        detail.afOwner = acObj["registered_owner"] | "";
    }

    JsonObject route = doc["response"]["flightroute"];
    if (route.isNull()) return false;

    JsonObject origin = route["origin"];
    JsonObject dest = route["destination"];
    if (origin.isNull() && dest.isNull()) return false;

    String depIata = origin["iata_code"] | "";
    String depIcao = origin["icao_code"] | "";
    String depName = origin["name"] | "";
    String depCity = origin["municipality"] | "";
    double depLat  = origin["latitude"] | 0.0;
    double depLon  = origin["longitude"] | 0.0;

    String arrIata = dest["iata_code"] | "";
    String arrIcao = dest["icao_code"] | "";
    String arrName = dest["name"] | "";
    String arrCity = dest["municipality"] | "";
    double arrLat  = dest["latitude"] | 0.0;
    double arrLon  = dest["longitude"] | 0.0;

    detail.departureAirport = !depIata.isEmpty() ? depIata : depIcao;
    detail.departureAirportFull = depName;
    detail.departureCity = depCity;
    detail.depLat = depLat;
    detail.depLon = depLon;

    detail.arrivalAirport = !arrIata.isEmpty() ? arrIata : arrIcao;
    detail.arrivalAirportFull = arrName;
    detail.arrivalCity = arrCity;
    detail.arrLat = arrLat;
    detail.arrLon = arrLon;

    if (detail.departureAirport.isEmpty() && detail.arrivalAirport.isEmpty()) {
        return false;
    }

    ResolveAirportNames(detail);
    detail.loaded = true;
    detail.fetchedMs = millis();
    Serial.printf("[ROUTE] adsbdb %s: %s (%s) -> %s (%s)\n",
        cs.c_str(),
        detail.departureAirport.c_str(), detail.departureCity.c_str(),
        detail.arrivalAirport.c_str(), detail.arrivalCity.c_str());
    return true;
}

bool AircraftManager::FetchFlightDetailHexdb(const String& callsign, FlightDetail& detail)
{
    String cs = callsign;
    cs.trim();
    if (cs.isEmpty()) return false;

    char url[192];
    snprintf(url, sizeof(url), "https://hexdb.io/api/v2/routes/%s", cs.c_str());
    HttpResult result = http.Get(url);
    if (!result.success || result.statusCode != 200) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result.response);
    if (error) return false;

    JsonObject origin = doc["origin"];
    JsonObject dest = doc["destination"];
    if (origin.isNull() && dest.isNull()) return false;

    String depIata = origin["iata"] | "";
    String depIcao = origin["icao"] | "";
    String depName = origin["name"] | "";
    String arrIata = dest["iata"] | "";
    String arrIcao = dest["icao"] | "";
    String arrName = dest["name"] | "";

    detail.departureAirport = !depIata.isEmpty() ? depIata : depIcao;
    detail.departureAirportFull = depName;
    detail.arrivalAirport = !arrIata.isEmpty() ? arrIata : arrIcao;
    detail.arrivalAirportFull = arrName;

    if (detail.departureAirport.isEmpty() && detail.arrivalAirport.isEmpty()) {
        return false;
    }

    ResolveAirportNames(detail);
    detail.loaded = true;
    detail.fetchedMs = millis();
    return true;
}

void AircraftManager::RequestFlightDetail(const String& icao, bool allowAirlabs, int prio)
{
    auto trackedIt = trackedAircraft.find(icao);
    if (trackedIt == trackedAircraft.end()) return;

    String callsign = trackedIt->second.state.callsign;
    callsign.trim();
    const bool needAirframe = trackedIt->second.state.typeCode.isEmpty() || trackedIt->second.state.registration.isEmpty();
    const String airlabsKey = allowAirlabs ? configServer.GetStoredString("airlabs-key") : String("");

    net.Submit("detail:" + icao, [this, icao, callsign, needAirframe, airlabsKey]() {
        FlightDetail d;
        if (!callsign.isEmpty() || needAirframe) {
            if (!FetchFlightDetailAdsbdb(callsign, icao, needAirframe, d) && !callsign.isEmpty()) {
                FetchFlightDetailHexdb(callsign, d);
            }
        }
        if (!d.loaded && !airlabsKey.isEmpty()) {
            if (FetchFlightDetailAirlabs(icao, callsign, d)) {
                d.loaded = !d.departureAirport.isEmpty() || !d.arrivalAirport.isEmpty();
            }
        }
        d.attempted = true;
        d.fetchedMs = millis();

        xSemaphoreTake(resultMutex, portMAX_DELAY);
        detailResults[icao] = d;
        xSemaphoreGive(resultMutex);
    }, prio);
}

void AircraftManager::PopulateFlightDetail(const String& icao, bool allowAirlabs)
{
    auto cached = flightDetailCache.find(icao);
    if (cached != flightDetailCache.end() && cached->second.attempted) {
        selectedDetail = cached->second;
        return;
    }

    // Show the card immediately; route fills in when the background lookup lands
    selectedDetail = FlightDetail{};
    lastRouteLookupMs = millis();
    RequestFlightDetail(icao, allowAirlabs, NetWorker::PRIO_URGENT);
}

void AircraftManager::ApplyNetworkResults()
{
    if (!resultMutex) return;

    std::map<String, FlightDetail> details;
    std::vector<Aircraft> area;
    bool haveArea = false, areaSucceeded = false;
    Aircraft pinned, single;
    bool havePinned = false, haveSingle = false;
    String singleIcao;

    xSemaphoreTake(resultMutex, portMAX_DELAY);
    details.swap(detailResults);
    if (areaReady) { area.swap(areaResult); haveArea = true; areaSucceeded = areaOk; areaReady = false; }
    if (pinnedReady) { pinned = pinnedResult; havePinned = true; pinnedReady = false; }
    if (singleReady) { single = singleResult; singleIcao = singleResultIcao; haveSingle = true; singleReady = false; }
    xSemaphoreGive(resultMutex);

    unsigned long now = millis();

    if (haveArea && areaSucceeded) {
        for (auto& ac : area) {
            auto it = trackedAircraft.find(ac.icao24);
            if (it == trackedAircraft.end())
                trackedAircraft.emplace(ac.icao24, TrackedAircraft{ac, now});
            else
                it->second.Update(ac, now);
            UpdateSpotterStats(ac);
        }

        constexpr unsigned long AIRCRAFT_EXPIRY_MS = 180000;
        for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
            if (now - it->second.lastSeen > AIRCRAFT_EXPIRY_MS) {
                flightDetailCache.erase(it->first);
                it = trackedAircraft.erase(it);
            } else {
                ++it;
            }
        }
        dataVersion++;
        Serial.printf("[RADAR] %d aircraft tracked\n", (int)trackedAircraft.size());
    }

    if (havePinned) ApplyPinnedAircraft(pinned);

    if (haveSingle) {
        auto it = trackedAircraft.find(singleIcao);
        if (it != trackedAircraft.end()) it->second.Update(single, now);
    }

    for (auto& [icao, d] : details) {
        auto tIt = trackedAircraft.find(icao);
        if (tIt == trackedAircraft.end()) continue; // aircraft left while we were looking it up
        auto& st = tIt->second.state;
        if (st.registration.isEmpty() && !d.afReg.isEmpty()) st.registration = d.afReg;
        if (st.typeCode.isEmpty() && !d.afType.isEmpty()) st.typeCode = d.afType;
        if (st.typeDescription.isEmpty() && !d.afTypeDesc.isEmpty()) st.typeDescription = d.afTypeDesc;
        if (st.operatorName.isEmpty() && !d.afOwner.isEmpty()) st.operatorName = d.afOwner;

        flightDetailCache[icao] = d;
        if (icao == selectedIcao) selectedDetail = d;
        dataVersion++;
    }
}

void AircraftManager::SelectFlight(const String& icao24)
{
#ifdef CROWPANEL_7
    automaticSelection = false;
#endif
    selectedIcao = icao24;
    Serial.printf("[FLIGHT] Selected %s\n", icao24.c_str());
    PopulateFlightDetail(icao24, true);

    // Pre-request aircraft photo and carrier logo into PSRAM queue
    auto it = trackedAircraft.find(icao24);
    if (it != trackedAircraft.end()) {
        mediaCache.RequestAircraftPhoto(icao24, it->second.state.registration);
        AirlineInfo airline = AviationCatalog::LookupAirlineInfo(it->second.state.callsign, it->second.state.operatorName);
        if (airline.matched) {
            mediaCache.RequestCarrierLogo(airline.iata);
        }
    }
}

bool AircraftManager::FetchSingleAircraftOpenSky(const String& icao, Aircraft& out)
{
    const String token = authHandler.GetValidToken(
        configServer.GetStoredString("opensky-id"),
        configServer.GetStoredString("opensky-secret")
    );
    if (token.isEmpty()) return false;

    String icaoLower = icao;
    icaoLower.toLowerCase();

    char url[256];
    snprintf(url, sizeof(url),
        "https://opensky-network.org/api/states/all?icao24=%s",
        icaoLower.c_str());

    HttpResult result = http.Get(url, {}, {{"Authorization", "Bearer " + token}});
    if (!result.success) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result.response);
    if (error) return false;

    JsonArray states = doc["states"];
    if (states.isNull() || states.size() == 0) return false;

    out = JsonParser::Parse<Aircraft>(states[0]);
    return true;
}

bool AircraftManager::ParseAdsbLolAircraftObj(JsonVariant v, Aircraft& ac) const
{
    if (v["lat"].isNull() || v["lon"].isNull()) return false;

    ac = Aircraft{};
    ac.icao24 = v["hex"] | "";
    ac.icao24.trim();
    ac.icao24.toLowerCase();
    if (ac.icao24.isEmpty()) return false;

    ac.callsign = v["flight"] | "";
    ac.callsign.trim();
    ac.registration = v["r"] | "";
    ac.registration.trim();
    ac.typeCode = v["t"] | "";
    ac.typeCode.trim();
    ac.typeDescription = v["desc"] | "";
    ac.typeDescription.trim();
    ac.operatorName = v["ownOp"] | "";
    ac.operatorName.trim();
    ac.emitterCategory = v["category"] | "";
    ac.emitterCategory.trim();

    ac.latitude = v["lat"].as<float>();
    ac.longitude = v["lon"].as<float>();

    JsonVariant altVar = v["alt_baro"];
    if (altVar.is<const char*>() && String(altVar.as<const char*>()) == "ground") {
        ac.onGround = true;
        ac.baroAltitude = 0.0f;
    } else {
        ac.onGround = false;
        ac.baroAltitude = (altVar | 0.0f) * 0.3048f;
    }

    ac.velocity = (v["gs"] | 0.0f) * 0.514444f;
    ac.trueTrack = v["track"].isNull() ? (v["true_heading"] | 0.0f) : v["track"].as<float>();
    ac.verticalRate = (v["baro_rate"] | 0.0f) * 0.00508f;
    ac.squawk = v["squawk"] | "";
    return true;
}

bool AircraftManager::FetchAreaAdsbLol(std::vector<Aircraft>& out)
{
    if (!HasLocationConfigured()) return false;

    int distNm = std::max(1, std::min(250, static_cast<int>(std::ceil(rad * 60.0 * 1.42))));
    char url[160];
    snprintf(url, sizeof(url), "https://api.adsb.lol/v2/lat/%.6f/lon/%.6f/dist/%d", lat, lon, distNm);

    HttpResult result = http.Get(url);
    if (!result.success || result.statusCode != 200) {
        Serial.printf("[WARN] adsb.lol HTTP %d: %s\n", result.statusCode, result.errorMessage.c_str());
        return false;
    }

    JsonDocument doc;
    DeserializationError parseError = deserializeJson(doc, result.response);
    if (parseError) {
        Serial.printf("[WARN] adsb.lol JSON parse failed: %s\n", parseError.c_str());
        return false;
    }

    JsonArray acArray = doc["ac"];
    if (acArray.isNull()) return false;

    out.reserve(acArray.size());
    for (JsonVariant v : acArray) {
        Aircraft ac{};
        if (ParseAdsbLolAircraftObj(v, ac)) {
            out.push_back(ac);
        }
    }
    return true;
}

bool AircraftManager::FetchPinnedFlightAdsbLol(const String& pinnedQuery, Aircraft& pinnedAc)
{
    // Runs on the NetWorker task
    if (pinnedQuery.isEmpty()) return false;

    auto tryEndpoint = [&](const String& endpointUrl, Aircraft& outAc) -> bool {
        HttpResult res = http.Get(endpointUrl);
        if (!res.success || res.statusCode != 200) return false;
        JsonDocument doc;
        if (deserializeJson(doc, res.response)) return false;
        JsonArray arr = doc["ac"];
        if (arr.isNull() || arr.size() == 0) return false;
        return ParseAdsbLolAircraftObj(arr[0], outAc);
    };

    pinnedAc = Aircraft{};
    bool found = false;

    // 1. Try direct callsign lookup
    found = tryEndpoint("https://api.adsb.lol/v2/callsign/" + pinnedQuery, pinnedAc);

    // 2. If user typed 2-letter IATA flight number (e.g. "BA117"), convert to 3-letter ICAO ("BAW117")
    if (!found && pinnedQuery.length() >= 3 && isAlpha(pinnedQuery.charAt(0)) && isAlpha(pinnedQuery.charAt(1)) && isDigit(pinnedQuery.charAt(2))) {
        String iata2 = pinnedQuery.substring(0, 2);
        String digits = pinnedQuery.substring(2);
        const int count = sizeof(AviationCatalog::AIRLINES) / sizeof(AviationCatalog::AIRLINES[0]);
        for (int i = 0; i < count; ++i) {
            if (iata2 == AviationCatalog::AIRLINES[i].iata) {
                String icaoCall = String(AviationCatalog::AIRLINES[i].icao) + digits;
                if (tryEndpoint("https://api.adsb.lol/v2/callsign/" + icaoCall, pinnedAc)) {
                    found = true;
                    break;
                }
            }
        }
    }

    // 3. Try registration lookup (e.g. "G-XWBA" or "N12345")
    if (!found) {
        found = tryEndpoint("https://api.adsb.lol/v2/reg/" + pinnedQuery, pinnedAc);
    }

    // 4. Try 6-char ICAO hex lookup
    if (!found && pinnedQuery.length() == 6) {
        String hexLower = pinnedQuery;
        hexLower.toLowerCase();
        found = tryEndpoint("https://api.adsb.lol/v2/hex/" + hexLower, pinnedAc);
    }

    if (!found) {
        Serial.printf("[PIN] Pinned flight %s not currently transmitting on ADS-B\n", pinnedQuery.c_str());
        return false;
    }

    Serial.printf("[PIN] Tracking pinned flight %s (%s) at %.3f, %.3f\n",
        pinnedAc.callsign.c_str(), pinnedAc.icao24.c_str(), pinnedAc.latitude, pinnedAc.longitude);
    return true;
}

void AircraftManager::ApplyPinnedAircraft(const Aircraft& pinnedAc)
{
    unsigned long now = millis();
    auto it = trackedAircraft.find(pinnedAc.icao24);
    if (it == trackedAircraft.end()) {
        trackedAircraft.emplace(pinnedAc.icao24, TrackedAircraft{pinnedAc, now});
    } else {
        it->second.Update(pinnedAc, now);
    }
    UpdateSpotterStats(pinnedAc);

    bool newlyPinned = (pinnedIcao != pinnedAc.icao24);
    pinnedIcao = pinnedAc.icao24;
    if (newlyPinned || selectedIcao.isEmpty()) {
        SelectFlight(pinnedIcao);
    }
}

void AircraftManager::UpdateSpotterStats(const Aircraft& ac)
{
    if (ac.onGround) return;

    if (seenIcaosSession.find(ac.icao24) == seenIcaosSession.end()) {
        if (seenIcaosSession.size() > 800) {
            seenIcaosSession.clear();
        }
        seenIcaosSession.insert(ac.icao24);
        stats.totalUniqueSeen++;
    }

    String label = !ac.callsign.isEmpty() ? ac.callsign : (!ac.registration.isEmpty() ? ac.registration : ac.icao24);
    float distKm = HasLocationConfigured() ? AviationCatalog::ComputeDistanceKm(lat, lon, ac.latitude, ac.longitude) : 999999.0f;

    if (HasLocationConfigured() && distKm > 0.01f && distKm < stats.closestDistKm) {
        stats.closestDistKm = distKm;
        stats.closestCallsign = label;
        stats.closestType = ac.typeCode;
    }

    if (ac.baroAltitude > stats.highestAltM && ac.baroAltitude < 30000.0f) {
        stats.highestAltM = ac.baroAltitude;
        stats.highestCallsign = label;
        stats.highestType = ac.typeCode;
    }

    if (ac.velocity > stats.fastestSpeedMps && ac.velocity < 900.0f) {
        stats.fastestSpeedMps = ac.velocity;
        stats.fastestCallsign = label;
        stats.fastestType = ac.typeCode;
    }

    AircraftTypeInfo typeInfo = AviationCatalog::LookupAircraftType(ac.typeCode, ac.typeDescription, ac.emitterCategory);
    bool isEmergency = (ac.squawk == "7700" || ac.squawk == "7600" || ac.squawk == "7500");

    if (typeInfo.isRareCatch || isEmergency) {
        bool alreadyLogged = false;
        for (const auto& existing : stats.rareCatches) {
            if (existing.icao24 == ac.icao24) {
                alreadyLogged = true;
                break;
            }
        }
        if (!alreadyLogged) {
            RareCatchEntry entry;
            entry.icao24 = ac.icao24;
            entry.callsign = label;
            entry.typeCode = ac.typeCode;
            entry.typeName = typeInfo.fullName;
            entry.badge = isEmergency ? ("SQK " + ac.squawk) : typeInfo.rareBadge;
            entry.distKm = (distKm < 99999.0f) ? distKm : 0.0f;
            stats.rareCatches.insert(stats.rareCatches.begin(), entry);
            if (stats.rareCatches.size() > 8) {
                stats.rareCatches.pop_back();
            }
        }
    }
}

bool AircraftManager::FetchAreaOpenSky(std::vector<Aircraft>& out)
{
    if (!HasLocationConfigured()) return false;

    const String token = authHandler.GetValidToken(
        configServer.GetStoredString("opensky-id"),
        configServer.GetStoredString("opensky-secret")
    );

    std::vector<std::pair<String, String>> headers = {};
    if (!token.isEmpty()) headers.push_back({"Authorization", "Bearer " + token});

    HttpResult result = http.Get(
        "https://opensky-network.org/api/states/all",
        {
            {"lamin", String(lat - rad, 6)},
            {"lamax", String(lat + rad, 6)},
            {"lomin", String(lon - rad, 6)},
            {"lomax", String(lon + rad, 6)}
        },
        headers
    );

    if (!result.success || result.statusCode != 200) {
        return false;
    }

    JsonDocument doc;
    DeserializationError parseError = deserializeJson(doc, result.response);
    if (parseError) return false;

    JsonArray states = doc["states"];
    if (states.isNull()) return false;

    out = JsonParser::ParseArray<Aircraft>(states);
    return true;
}

void AircraftManager::Update()
{
    unsigned long now = millis();

    // Auto-cycle display views if enabled
    if (autoCycleViews && !statsModalOpen && (now - lastViewCycleMs >= 25000)) {
        lastViewCycleMs = now;
        int next = (static_cast<int>(viewMode) + 1) % VIEW_COUNT;
        viewMode = static_cast<DisplayViewMode>(next);
        dataVersion++;
    }

    // Pick up anything the background worker has finished (cheap; never blocks on network)
    ApplyNetworkResults();

    // Pinned flight polling (every 15s) -- runs on the worker
    if (!pinnedQuery.isEmpty() && (lastPinnedFetchMs == 0 || now - lastPinnedFetchMs >= 15000)) {
        lastPinnedFetchMs = now;
        const String query = pinnedQuery;
        net.Submit("pinned", [this, query]() {
            Aircraft ac{};
            if (FetchPinnedFlightAdsbLol(query, ac)) {
                xSemaphoreTake(resultMutex, portMAX_DELAY);
                pinnedResult = ac;
                pinnedReady = true;
                xSemaphoreGive(resultMutex);
            }
        }, NetWorker::PRIO_VISIBLE);
    }

    // Area poll -- runs on the worker; results merged by ApplyNetworkResults()
    if (now - lastFetch >= fetchInterval && !net.IsInflight("area")) {
        lastFetch = now;
        const bool useOpenSky = (trackingProvider == "opensky");
        net.Submit("area", [this, useOpenSky]() {
            std::vector<Aircraft> aircraft;
            bool ok = useOpenSky ? FetchAreaOpenSky(aircraft) : FetchAreaAdsbLol(aircraft);
            xSemaphoreTake(resultMutex, portMAX_DELAY);
            areaResult.swap(aircraft);
            areaOk = ok;
            areaReady = true;
            xSemaphoreGive(resultMutex);
        }, NetWorker::PRIO_VISIBLE);
    }

    if (IsDetailOpen()
#ifdef CROWPANEL_7
        && !automaticSelection
#endif
        && selectedIcao != lastSingleFetchHex) {
        lastSingleFetch = 0;
        lastSingleFetchHex = selectedIcao;
    }

    unsigned long nowSingle = millis();
    if (IsDetailOpen()
#ifdef CROWPANEL_7
        && !automaticSelection
#endif
        && (nowSingle - lastSingleFetch >= SINGLE_FETCH_INTERVAL)) {
        lastSingleFetch = nowSingle;

        const String icao = selectedIcao;
        const bool useOpenSky = (trackingProvider == "opensky");
        const String airlabsKey = configServer.GetStoredString("airlabs-key");
        net.Submit("single:" + icao, [this, icao, useOpenSky, airlabsKey]() {
            if (useOpenSky) {
                Aircraft ac{};
                if (FetchSingleAircraftOpenSky(icao, ac)) {
                    xSemaphoreTake(resultMutex, portMAX_DELAY);
                    singleResult = ac;
                    singleResultIcao = icao;
                    singleReady = true;
                    xSemaphoreGive(resultMutex);
                }
            }
            if (!airlabsKey.isEmpty()) {
                airlabsManager.updateSingle(icao);
            }
        }, NetWorker::PRIO_BACKGROUND);
    }
}

void AircraftManager::DismissDetail()
{
    selectedIcao = "";
    selectedDetail = FlightDetail{};
}

#ifdef CROWPANEL_7
bool AircraftManager::IsAircraftVisible(const TrackedAircraft& tracked) const
{
    if (!pinnedIcao.isEmpty() && tracked.state.icao24 == pinnedIcao) return true;
    if (tracked.state.onGround) return false;
    auto [predLat, predLon] = tracked.GetDisplayPosition();
    auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);
    return x >= RADAR_X && x < RADAR_X + RADAR_SIZE &&
           y >= RADAR_Y && y < RADAR_Y + RADAR_SIZE;
}

void AircraftManager::NoteInteraction(unsigned long now)
{
    lastInteractionMs = now;
    lastRotationMs = now;
    interactionPaused = true;
}

void AircraftManager::PrefetchNextInRotation(unsigned long now)
{
    (void)now;
    auto next = trackedAircraft.upper_bound(selectedIcao);
    for (size_t i = 0; i < trackedAircraft.size(); ++i) {
        if (next == trackedAircraft.end()) next = trackedAircraft.begin();
        if (IsAircraftVisible(next->second)) {
            if (next->first == selectedIcao) return;
            auto cached = flightDetailCache.find(next->first);
            if (cached == flightDetailCache.end() || !cached->second.attempted) {
                RequestFlightDetail(next->first, false, NetWorker::PRIO_PREFETCH);
            }
            mediaCache.RequestAircraftPhoto(next->first, next->second.state.registration, NetWorker::PRIO_PREFETCH);
            AirlineInfo airline = AviationCatalog::LookupAirlineInfo(next->second.state.callsign, next->second.state.operatorName);
            if (airline.matched) mediaCache.RequestCarrierLogo(airline.iata, NetWorker::PRIO_PREFETCH);
            return;
        }
        ++next;
    }
}

void AircraftManager::UpdateIdleSelection(unsigned long now)
{
    // If a flight is pinned and valid, keep it featured when idle
    if (!pinnedIcao.isEmpty() && trackedAircraft.find(pinnedIcao) != trackedAircraft.end()) {
        if (!interactionPaused || (now - lastInteractionMs >= 20000)) {
            if (selectedIcao != pinnedIcao) {
                selectedIcao = pinnedIcao;
                PopulateFlightDetail(selectedIcao, false);
            }
            return;
        }
    }

    constexpr unsigned long IDLE_DELAY_MS = 15000;
    constexpr unsigned long ROTATION_INTERVAL_MS = 10000;
    auto selected = trackedAircraft.find(selectedIcao);
    const bool missing = !selectedIcao.isEmpty() && selected == trackedAircraft.end();
    if (interactionPaused && !missing && now - lastInteractionMs < IDLE_DELAY_MS) return;
    interactionPaused = false;

    if (selected != trackedAircraft.end() && IsAircraftVisible(selected->second) &&
        now - lastRotationMs < ROTATION_INTERVAL_MS) {
        // Warm route + photo + logo for the next plane a few seconds early so the switch is instant
        if (now - lastRotationMs >= ROTATION_INTERVAL_MS - 5000) PrefetchNextInRotation(now);
        return;
    }

    auto next = trackedAircraft.upper_bound(selectedIcao);
    for (size_t i = 0; i < trackedAircraft.size(); ++i) {
        if (next == trackedAircraft.end()) next = trackedAircraft.begin();
        if (IsAircraftVisible(next->second)) {
            if (next->first != selectedIcao) {
                selectedIcao = next->first;
                auto cached = flightDetailCache.find(selectedIcao);
                if (cached != flightDetailCache.end() && cached->second.attempted) {
                    selectedDetail = cached->second;
                } else {
                    PopulateFlightDetail(selectedIcao, false); // non-blocking: queued on the worker
                }
                // Queue photo & logo in background
                mediaCache.RequestAircraftPhoto(selectedIcao, next->second.state.registration);
                AirlineInfo airline = AviationCatalog::LookupAirlineInfo(next->second.state.callsign, next->second.state.operatorName);
                if (airline.matched) {
                    mediaCache.RequestCarrierLogo(airline.iata);
                }
            } else if (!selectedDetail.attempted && now - lastRouteLookupMs >= ROUTE_LOOKUP_COOLDOWN_MS) {
                PopulateFlightDetail(selectedIcao, false);
            }
            automaticSelection = true;
            lastRotationMs = now;
            return;
        }
        ++next;
    }

    if (!selectedIcao.isEmpty()) {
        selectedIcao = "";
        selectedDetail = FlightDetail{};
    }
    automaticSelection = true;
}
#endif

String AircraftManager::BuildStatusJson() const
{
    JsonDocument doc;
    doc["view"] = ViewModeToKey(viewMode);
    doc["viewName"] = ViewModeToDisplayName(viewMode);
    doc["theme"] = ThemeIdToKey(themeId);
    doc["themeName"] = ThemeIdToDisplayName(themeId);
    doc["radius"] = rad;
    doc["pinnedQuery"] = pinnedQuery;
    doc["pinnedIcao"] = pinnedIcao;
    doc["selectedIcao"] = selectedIcao;
    doc["giftTitle"] = giftTitle;
    doc["giftSubtitle"] = giftSubtitle;
    doc["trackedCount"] = GetSortedActiveFlights().size();
    doc["totalSpotted"] = stats.totalUniqueSeen;
    doc["closestDist"] = (stats.closestDistKm < 99999.0f) ? ConvertDistance(stats.closestDistKm) : 0.0f;
    doc["closestCallsign"] = stats.closestCallsign;
    doc["distUnit"] = DistUnit();
    doc["altUnit"] = AltUnit();
    doc["speedUnit"] = SpeedUnit();

    JsonArray arr = doc["flights"].to<JsonArray>();
    auto sorted = GetSortedActiveFlights();
    for (size_t i = 0; i < min((size_t)12, sorted.size()); ++i) {
        auto it = trackedAircraft.find(sorted[i].icao);
        if (it == trackedAircraft.end()) continue;
        const TrackedAircraft& t = it->second;
        AirlineInfo al = AviationCatalog::LookupAirlineInfo(t.state.callsign, t.state.operatorName);
        AircraftTypeInfo ti = AviationCatalog::LookupAircraftType(t.state.typeCode, t.state.typeDescription, t.state.emitterCategory);

        JsonObject f = arr.add<JsonObject>();
        f["icao"] = sorted[i].icao;
        f["callsign"] = !t.state.callsign.isEmpty() ? t.state.callsign : sorted[i].icao;
        f["airline"] = al.name;
        f["iata"] = al.iata;
        f["reg"] = t.state.registration;
        f["type"] = !t.state.typeCode.isEmpty() ? t.state.typeCode : "AIR";
        f["typeName"] = ti.fullName;
        f["alt"] = static_cast<int>(ConvertAltitude(t.state.baroAltitude));
        f["speed"] = static_cast<int>(ConvertSpeed(t.state.velocity));
        f["dist"] = serialized(String(ConvertDistance(sorted[i].distKm), 1));
        f["dir"] = AviationCatalog::BearingToCardinal(sorted[i].bearingDeg);
        f["pinned"] = sorted[i].isPinned;
        f["selected"] = (sorted[i].icao == selectedIcao);

        auto routeIt = flightDetailCache.find(sorted[i].icao);
        if (routeIt != flightDetailCache.end() && routeIt->second.loaded) {
            f["dep"] = routeIt->second.departureAirport;
            f["arr"] = routeIt->second.arrivalAirport;
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

#include "AircraftManager.h"
#include <Preferences.h>
#include <time.h>

constexpr int SCREEN_SIZE = 240;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

#include <ArduinoJson.h>

void AircraftManager::Initialise()
{
    lat = configServer.GetStoredString("latitude").toDouble();
    lon = configServer.GetStoredString("longitude").toDouble();
    rad = configServer.GetStoredString("radius").toDouble();

    const String unitsStr = configServer.GetStoredString("units");
    units = (unitsStr == "imperial") ? IMPERIAL : METRIC;
    Serial.print("[UNITS] Set to: ");
    Serial.println(units == IMPERIAL ? "imperial" : "metric");

    const String renderText = configServer.GetStoredString("infotext");
    const String renderTris = configServer.GetStoredString("triangle");
    if (!renderText.isEmpty()) displayInfoText = renderText == "true" ? true : false;
    if (!renderTris.isEmpty()) displayTriangles = renderTris == "true" ? true : false;

    // AirLabs free tier: 1,000 requests/month
    // Fetch every 45 minutes = 960 requests/month (stays within limit)
    fetchInterval = 2700000;  // 45 minutes in milliseconds
}

void AircraftManager::Draw(LGFX_Sprite& backbuffer)
{
    DrawRadarCircles(backbuffer);

    for (auto& [icao, tracked] : trackedAircraft) {
        if (tracked.state.onGround) continue;

        tracked.Tick();
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);

        if (displayInfoText)
            DrawAircraftInfo(backbuffer, x, y, tracked);

        if (displayTriangles)
            DrawAircraftTriangle(backbuffer, x, y, tracked);
        else
            backbuffer.fillCircle(x, y, 3, lgfx::color888(0, 255, 0));
    }

    if (!selectedIcao.isEmpty()) {
        auto it = trackedAircraft.find(selectedIcao);
        if (it != trackedAircraft.end()) {
            auto [predLat, predLon] = it->second.GetDisplayPosition();
            auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);
            backbuffer.drawCircle(x, y, 10, lgfx::color888(0, 255, 255));
        }
    }
}

void AircraftManager::DrawRadarCircles(LGFX_Sprite& backbuffer) const
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;
    constexpr int OUTER = SCREEN_SIZE_DIV_2 - 1;

    backbuffer.drawCircle(CENTRE, CENTRE, OUTER, lgfx::color888(0, 200, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, (OUTER / 3) * 2, lgfx::color888(0, 64, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, OUTER / 3, lgfx::color888(0, 32, 0));
}

void AircraftManager::adjustRadius(double delta)
{
    rad += delta;
    if (rad < 0.05) rad = 0.05;
    if (rad > 180.0) rad = 180.0;
    
    Preferences prefs;
    prefs.begin("config");
    prefs.putString("radius", String(rad, 6));
    prefs.end();
    
    Serial.print("[RADAR] Radius adjusted to: ");
    Serial.println(rad, 6);
}

std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - lon;
    const float dLat = predLat - lat;

    const float normLon = (dLon + rad) / (2.0f * rad);
    const float normLat = (dLat + rad) / (2.0f * rad);

    const int x = static_cast<int>(normLon * SCREEN_SIZE);
    const int y = static_cast<int>(SCREEN_SIZE - (normLat * SCREEN_SIZE));

    return { x, y };
}

void AircraftManager::DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const
{
    const int lineHeight = tft.fontHeight() + 1;
    const float speed = ConvertSpeed(tracked.state.velocity);
    const float alt = ConvertAltitude(tracked.state.baroAltitude);

    backbuffer.setTextSize(1);
    backbuffer.setTextColor(lgfx::color888(0, 128, 0));
    backbuffer.drawString(tracked.state.callsign, x + 5, y + 5);
    backbuffer.drawString(String(speed, 0) + " " + SpeedUnit(), x + 5, y + 5 + lineHeight);
    backbuffer.drawString(String(alt, 0) + " " + AltUnit(), x + 5, y + 5 + lineHeight * 2);
}

void AircraftManager::DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const
{
    const float dx = std::sin(radians(tracked.state.trueTrack));
    const float dy = -std::cos(radians(tracked.state.trueTrack));
    const float px = -dy;
    const float py = dx;

    constexpr float TRIANGLE_LENGTH = 6.0f;
    constexpr float TRIANGLE_WIDTH = 3.0f;

    const float tipX = x + dx * TRIANGLE_LENGTH;
    const float tipY = y + dy * TRIANGLE_LENGTH;
    const float leftX = x - dx * TRIANGLE_LENGTH * 0.5f + px * TRIANGLE_WIDTH * 0.5f;
    const float leftY = y - dy * TRIANGLE_LENGTH * 0.5f + py * TRIANGLE_WIDTH * 0.5f;
    const float rightX = x - dx * TRIANGLE_LENGTH * 0.5f - px * TRIANGLE_WIDTH * 0.5f;
    const float rightY = y - dy * TRIANGLE_LENGTH * 0.5f - py * TRIANGLE_WIDTH * 0.5f;

    backbuffer.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, lgfx::color888(0, 255, 0));
}

String AircraftManager::HitTestAircraft(int tapX, int tapY) const
{
    String nearestIcao;
    float nearestDist = 1e9f;

    for (auto& [icao, tracked] : trackedAircraft) {
        if (tracked.state.onGround) continue;
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [sx, sy] = ProjectCoordinateToScreen(predLat, predLon);

        const int textPadX = 75;
        const int textPadY = 32;
        const int bubblePad = 14;

        int left   = min(sx, sx + 5) - bubblePad;
        int right  = max(sx, sx + textPadX) + bubblePad;
        int top    = min(sy, sy + 5) - bubblePad;
        int bottom = max(sy, sy + textPadY) + bubblePad;

        if (tapX >= left && tapX <= right && tapY >= top && tapY <= bottom) {
            float cx = (left + right) / 2.0f;
            float cy = (top + bottom) / 2.0f;
            float ddx = sx - cx;
            float ddy = sy - cy;
            float dist = ddx * ddx + ddy * ddy;
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestIcao = icao;
            }
        }
    }
    return nearestIcao;
}

void AircraftManager::SelectFlight(const String& icao24)
{
    selectedIcao = icao24;
    selectedDetail = FlightDetail{};
    
    Serial.printf("[FLIGHT] Selected %s\n", icao24.c_str());
    
    auto it = trackedAircraft.find(icao24);
    if (it != trackedAircraft.end()) {
        // Get route data from AirLabs cache
        const auto& airlabsAircraft = airlabsManager.getAircraft();
        auto airlabsIt = airlabsAircraft.find(icao24);
        
        if (airlabsIt != airlabsAircraft.end()) {
            const AirLabsAircraft& ac = airlabsIt->second;
            selectedDetail.departureAirport = ac.dep_iata;
            selectedDetail.arrivalAirport = ac.arr_iata;
            
            // Fetch airport names synchronously
            Serial.println("[FLIGHT] Fetching airport names...");
            if (!ac.dep_iata.isEmpty()) {
                auto depInfo = airlabsManager.getAirportInfo(ac.dep_iata);
                if (!depInfo.name.isEmpty()) {
                    selectedDetail.departureAirportFull = depInfo.name;
                }
            }
            if (!ac.arr_iata.isEmpty()) {
                auto arrInfo = airlabsManager.getAirportInfo(ac.arr_iata);
                if (!arrInfo.name.isEmpty()) {
                    selectedDetail.arrivalAirportFull = arrInfo.name;
                }
            }
        }
    }
    
    selectedDetail.loaded = true;
    Serial.println("[FLIGHT] Detail loaded");
}

void AircraftManager::Update()
{
    unsigned long now = millis();

    if (now - lastFetch >= fetchInterval) {
        lastFetch = now;

        bool success = airlabsManager.update(lat, lon, rad);

        if (!success) {
            Serial.println("[WARN] AirLabs API request failed");
            return;
        }

        const auto& airlabsAircraft = airlabsManager.getAircraft();
        now = millis();

        // Guard against empty responses wiping the radar — treat as fetch failure
        if (airlabsAircraft.empty()) {
            Serial.println("[WARN] AirLabs returned empty response, keeping existing track data");
            return;
        }

        for (auto& [hex, ac] : airlabsAircraft) {
            // Convert AirLabsAircraft to Aircraft struct
            Aircraft tracked;
            tracked.icao24 = ac.hex;
            tracked.callsign = ac.callsign;
            tracked.baroAltitude = ac.alt;
            tracked.velocity = ac.speed;
            tracked.trueTrack = ac.dir;
            tracked.verticalRate = ac.v_speed;
            tracked.latitude = ac.lat;
            tracked.longitude = ac.lng;
            tracked.onGround = ac.alt < 30; // approximate
            
            auto it = trackedAircraft.find(ac.hex);
            if (it == trackedAircraft.end())
                trackedAircraft.emplace(hex, TrackedAircraft{ tracked, now });
            else
                it->second.Update(tracked, now);
        }

        // Time-based expiry: only remove aircraft that haven't been seen for
        // AIRCRAFT_EXPIRY_MS. This prevents momentary API gaps from wiping the radar.
        constexpr unsigned long AIRCRAFT_EXPIRY_MS = 300000; // 5 minutes
        for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
            if (now - it->second.lastSeen > AIRCRAFT_EXPIRY_MS) {
                // Don't dismiss the detail panel just because the aircraft expired —
                // the user may still be viewing it. They can dismiss manually.
                Serial.printf("[RADAR] Expired stale track: %s\n", it->first.c_str());
                it = trackedAircraft.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (IsDetailOpen() && selectedIcao != lastSingleFetchHex) {
        lastSingleFetch = 0;
        lastSingleFetchHex = selectedIcao;
    }

    unsigned long nowSingle = millis();
    if (IsDetailOpen() && (nowSingle - lastSingleFetch >= SINGLE_FETCH_INTERVAL)) {
        lastSingleFetch = nowSingle;

        Serial.printf("[RADAR] Refreshing selected flight: %s\n", selectedIcao.c_str());
        bool ok = airlabsManager.updateSingle(selectedIcao);

        if (ok) {
            const auto& airlabsAircraft = airlabsManager.getAircraft();
            auto alIt = airlabsAircraft.find(selectedIcao);
            if (alIt != airlabsAircraft.end()) {
                const AirLabsAircraft& ac = alIt->second;
                auto trIt = trackedAircraft.find(selectedIcao);
                if (trIt != trackedAircraft.end()) {
                    Aircraft updated = trIt->second.state;
                    updated.baroAltitude = ac.alt;
                    updated.velocity = ac.speed;
                    updated.trueTrack = ac.dir;
                    updated.verticalRate = ac.v_speed;
                    updated.latitude = ac.lat;
                    updated.longitude = ac.lng;
                    updated.onGround = ac.alt < 30;
                    trIt->second.Update(updated, nowSingle);
                }
            }
        }
    }
}

void AircraftManager::DismissDetail()
{
    Serial.println("[FLIGHT] Detail dismissed");
    selectedIcao = "";
    selectedDetail = FlightDetail{};
}

String AircraftManager::LookupAirline(const String& callsign) const
{
    if (callsign.length() < 2) return "";
    String prefix = callsign.substring(0, 2);

    if (prefix == "BA") return "British Airways";
    if (prefix == "LH") return "Lufthansa";
    if (prefix == "AF") return "Air France";
    if (prefix == "KL") return "KLM";
    if (prefix == "DL") return "Delta Air Lines";
    if (prefix == "UA") return "United Airlines";
    if (prefix == "AA") return "American Airlines";
    if (prefix == "AY") return "Finnair";
    if (prefix == "IB") return "Iberia";
    if (prefix == "AZ") return "ITA Airways";
    if (prefix == "TP") return "TAP Air Portugal";
    if (prefix == "SK") return "SAS";
    if (prefix == "DY") return "Norwegian";
    if (prefix == "U2") return "easyJet";
    if (prefix == "FR") return "Ryanair";
    if (prefix == "VY") return "Vueling";
    if (prefix == "EY") return "Etihad Airways";
    if (prefix == "EK") return "Emirates";
    if (prefix == "QR") return "Qatar Airways";
    if (prefix == "TK") return "Turkish Airlines";
    if (prefix == "SA") return "South African Airways";
    if (prefix == "SQ") return "Singapore Airlines";
    if (prefix == "CX") return "Cathay Pacific";
    if (prefix == "QF") return "Qantas";
    if (prefix == "JL") return "Japan Airlines";
    if (prefix == "NH") return "All Nippon Airways";
    if (prefix == "KE") return "Korean Air";
    if (prefix == "OZ") return "Asiana Airlines";
    if (prefix == "AC") return "Air Canada";
    if (prefix == "WS") return "WestJet";
    if (prefix == "AM") return "Aeromexico";
    if (prefix == "CM") return "Copa Airlines";
    if (prefix == "AV") return "Avianca";
    if (prefix == "LA") return "LATAM";
    if (prefix == "AR") return "Aerolineas Argentinas";
    if (prefix == "ET") return "Ethiopian Airlines";
    if (prefix == "MS") return "EgyptAir";
    if (prefix == "AT") return "Royal Air Maroc";
    if (prefix == "EI") return "Aer Lingus";
    if (prefix == "LX") return "Swiss International";
    if (prefix == "OS") return "Austrian Airlines";
    if (prefix == "LO") return "LOT Polish Airlines";
    if (prefix == "OK") return "Czech Airlines";
    if (prefix == "SU") return "Aeroflot";
    if (prefix == "PS") return "UIA Ukraine";
    if (prefix == "BT") return "airBaltic";
    if (prefix == "FI") return "Icelandair";
    if (prefix == "AZ") return "Alitalia/ITA";
    if (prefix == "SN") return "Brussels Airlines";
    if (prefix == "TP") return "TAP Portugal";
    if (prefix == "BY") return "TUI Airways";
    if (prefix == "LS") return "Jet2";
    if (prefix == "W6") return "Wizz Air";
    if (prefix == "HV") return "Transavia";
    if (prefix == "NO") return "Neos";
    if (prefix == "PC") return "Pegasus Airlines";
    if (prefix == "XQ") return "SunExpress";
    if (prefix == "3U") return "Sichuan Airlines";
    if (prefix == "CA") return "Air China";
    if (prefix == "MU") return "China Eastern";
    if (prefix == "CZ") return "China Southern";
    if (prefix == "BR") return "EVA Air";
    if (prefix == "CI") return "China Airlines";
    if (prefix == "TG") return "Thai Airways";
    if (prefix == "MH") return "Malaysia Airlines";
    if (prefix == "GA") return "Garuda Indonesia";
    if (prefix == "PR") return "Philippine Airlines";
    if (prefix == "VN") return "Vietnam Airlines";
    if (prefix == "SV") return "Saudia";
    if (prefix == "MS") return "EgyptAir";
    if (prefix == "KM") return "Air Malta";
    if (prefix == "A3") return "Aegean Airlines";
    if (prefix == "TP") return "TAP Air Portugal";
    if (prefix == "IB") return "Iberia";
    return prefix;
}

void AircraftManager::DrawFlightDetail(LGFX_Sprite& buf) const
{
    if (selectedIcao.isEmpty()) return;

    auto it = trackedAircraft.find(selectedIcao);
    if (it == trackedAircraft.end()) return;

    const TrackedAircraft& tracked = it->second;

    const int px = 15, py = 15, pw = 210, ph = 210;

    buf.fillRect(px, py, pw, ph, lgfx::color888(0, 16, 0));
    buf.drawRect(px, py, pw, ph, lgfx::color888(0, 220, 0));
    buf.drawRect(px + 1, py + 1, pw - 2, ph - 2, lgfx::color888(0, 80, 0));

    buf.setTextSize(1);
    buf.setTextColor(lgfx::color888(0, 255, 0));

    int y = py + 8;
    const int lh = 11;
    const int leftX = px + 8;

    buf.setTextFont(2);
    buf.drawString(tracked.state.callsign, leftX, y);
    buf.drawString(LookupAirline(tracked.state.callsign), px + pw - 8 - buf.textWidth(LookupAirline(tracked.state.callsign)), y);
    buf.setTextFont(1);
    y += 18;

    buf.setTextColor(lgfx::color888(0, 180, 0));
    buf.drawString("ICAO: " + selectedIcao + "  " + tracked.state.originCountry, leftX, y);
    y += lh + 2;

    buf.drawLine(px + 6, y, px + pw - 6, y, lgfx::color888(0, 120, 0));
    y += 5;

    buf.setTextColor(lgfx::color888(0, 255, 0));
    const float speed = ConvertSpeed(tracked.state.velocity);
    const float alt = ConvertAltitude(tracked.state.baroAltitude);

    buf.drawString(String("Spd: ") + String(speed, 0) + " " + SpeedUnit(), leftX, y);
    buf.drawString(String("Alt: ") + String(alt, 0) + " " + AltUnit(), px + pw / 2, y);
    y += lh;
    buf.drawString(String("Hdg: ") + String(tracked.state.trueTrack, 0) + String((char)247), leftX, y);
    if (tracked.state.verticalRate != 0) {
        String vert = String(tracked.state.verticalRate > 0 ? "+" : "") + String(ConvertAltitude(tracked.state.verticalRate), 0) + " " + AltUnit() + "/m";
        buf.drawString(vert, px + pw / 2, y);
    }
    y += lh + 3;

    buf.drawLine(px + 6, y, px + pw - 6, y, lgfx::color888(0, 120, 0));
    y += 5;

    buf.setTextColor(lgfx::color888(0, 220, 180));
    const int maxWidth = pw - 8 - 40; // Available width for airport text
    
    if (selectedDetail.loaded) {
        if (!selectedDetail.departureAirport.isEmpty() || !selectedDetail.arrivalAirport.isEmpty()) {
            // Departure airport
            buf.drawString("From:", leftX, y);
            String depDisplay = selectedDetail.departureAirport;
            if (!selectedDetail.departureAirportFull.isEmpty()) {
                depDisplay = selectedDetail.departureAirport + " - " + selectedDetail.departureAirportFull;
            }
            
            // Wrap long text
            String remaining = depDisplay;
            bool firstLine = true;
            while (remaining.length() > 0) {
                if (!firstLine) y += lh;
                String line = "";
                int charIndex = 0;
                
                while (charIndex < remaining.length()) {
                    String testLine = line + remaining.charAt(charIndex);
                    if (buf.textWidth(testLine) > maxWidth) {
                        // Find last space to break at
                        int lastSpace = line.lastIndexOf(' ');
                        if (lastSpace > 0 && firstLine) {
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
                    // Last line
                    buf.drawString((firstLine ? "" : "    ") + line, leftX + 40, y);
                    remaining = "";
                } else {
                    buf.drawString((firstLine ? "" : "    ") + line, leftX + 40, y);
                }
                firstLine = false;
            }
            y += lh;
            
            // Arrival airport
            buf.drawString("To:", leftX, y);
            String arrDisplay = selectedDetail.arrivalAirport;
            if (!selectedDetail.arrivalAirportFull.isEmpty()) {
                arrDisplay = selectedDetail.arrivalAirport + " - " + selectedDetail.arrivalAirportFull;
            }
            
            // Wrap long text
            remaining = arrDisplay;
            firstLine = true;
            while (remaining.length() > 0) {
                if (!firstLine) y += lh;
                String line = "";
                int charIndex = 0;
                
                while (charIndex < remaining.length()) {
                    String testLine = line + remaining.charAt(charIndex);
                    if (buf.textWidth(testLine) > maxWidth) {
                        // Find last space to break at
                        int lastSpace = line.lastIndexOf(' ');
                        if (lastSpace > 0 && firstLine) {
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
                    // Last line
                    buf.drawString((firstLine ? "" : "    ") + line, leftX + 40, y);
                    remaining = "";
                } else {
                    buf.drawString((firstLine ? "" : "    ") + line, leftX + 40, y);
                }
                firstLine = false;
            }
            y += lh;
        }

        if (selectedDetail.departureTime > 0) {
            time_t t = (time_t)selectedDetail.departureTime;
            struct tm tmbuf;
            gmtime_r(&t, &tmbuf);
            char tbuf[16];
            strftime(tbuf, sizeof(tbuf), "%H:%M UTC", &tmbuf);
            buf.drawString("Departed:", leftX, y);
            buf.drawString(String(tbuf), leftX + 55, y);
            y += lh;
        }

        if (selectedDetail.arrivalTime > 0) {
            time_t t = (time_t)selectedDetail.arrivalTime;
            struct tm tmbuf;
            gmtime_r(&t, &tmbuf);
            char tbuf[16];
            strftime(tbuf, sizeof(tbuf), "%H:%M UTC", &tmbuf);
            buf.drawString("Arrived:", leftX, y);
            buf.drawString(String(tbuf), leftX + 55, y);
            y += lh;
        }

        if (selectedDetail.departureAirport.isEmpty() && selectedDetail.arrivalAirport.isEmpty()
            && selectedDetail.departureTime == 0 && selectedDetail.arrivalTime == 0) {
            buf.setTextColor(lgfx::color888(180, 180, 0));
            buf.drawString("Flight not in OpenSky", leftX, y);
            buf.drawString("route database", leftX, y + lh);
            y += lh * 2;
        }
    } else if (selectedDetail.attempted) {
        buf.setTextColor(lgfx::color888(180, 180, 0));
        buf.drawString("Fetch failed", leftX, y);
        buf.drawString("(Check serial log)", leftX, y + lh);
        y += lh * 2;
    } else {
        buf.setTextColor(lgfx::color888(180, 180, 0));
        buf.drawString("Needs OpenSky", leftX, y);
        buf.drawString("credentials", leftX, y + lh);
        y += lh * 2;
    }

    y = py + ph - 16;
    buf.drawLine(px + 6, y - 1, px + pw - 6, y - 1, lgfx::color888(0, 120, 0));
    buf.setTextColor(lgfx::color888(0, 180, 180));
    buf.drawCentreString("Tap screen to dismiss", px + pw / 2, y);
}

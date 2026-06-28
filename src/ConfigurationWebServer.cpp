#include "ConfigurationWebServer.h"
#include <ESPmDNS.h>
#include <algorithm>

// HTML stored in flash
// %PLACEHOLDER% tokens are substituted at serve time by the template processor
static const char CONFIG_HTML[] PROGMEM = R"CONFIGHTML(
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Configure Micro Radar</title>
        <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4.3.0"></script>
    </head>
    <body class="font-mono bg-gray-900 text-green-500 min-h-screen p-4 sm:p-0 text-md sm:text-sm">
        <fieldset class="border border-green-500 p-5 w-full max-w-2xl mx-auto sm:m-10">
            <legend class="px-2">Configure Micro Radar</legend>

            <form id="cfg" action="/save" method="POST" class="flex flex-col gap-4 sm:gap-2">

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Latitude:</span>
                        <input
                            name="latitude"
                            type="number"
                            min="-90"
                            step="0.000001"
                            max="90"
                            value='%LATITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>

                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Longitude:</span>
                        <input
                            name="longitude"
                            type="number"
                            min="-180"
                            step="0.000001"
                            max="180"
                            value='%LONGITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                </div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>Radius (in &deg;):</span>
                    <input
                        name="radius"
                        type="number"
                        min="0.000001"
                        step="0.000001"
                        max="2.499999"
                        value='%RADIUS%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <div class="flex flex-col sm:flex-row gap-4 sm:justify-between">
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Radar sweep:</span>
                        <input
                            name="scanline"
                            type="checkbox"
                            %SCANLINE%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Aircraft Info:</span>
                        <input
                            name="infotext"
                            type="checkbox"
                            %INFOTEXT%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Directional Aircraft:</span>
                        <input
                            name="triangle"
                            type="checkbox"
                            %TRIANGLE%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                </div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span class="sm:w-24 shrink-0">Units:</span>
                    <select
                        name="units"
                        class="flex-1 border border-green-500 bg-gray-900 px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                        <option value="metric" %UNITSMETRIC%>Metric (m/s, m)</option>
                        <option value="imperial" %UNITSIMPERIAL%>Imperial (knots, feet)</option>
                    </select>
                </label>

                <div class="px-1 font-bold text-lg sm:text-base text-green-300">Radar Tracking — OpenSky</div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>OpenSky Client ID:</span>
                    <input
                        name="opensky-id"
                        value='%OPENSKY_ID%'
                        placeholder="Enter OpenSky Client ID"
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>OpenSky Client Secret:</span>
                    <input
                        name="opensky-secret"
                        type="password"
                        value='%OPENSKY_SECRET%'
                        placeholder="Enter OpenSky Client Secret"
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <div class="flex gap-2 px-1 text-xs sm:text-xs opacity-70">
                    <span>&rarr;</span>
                    <span>Free signup: <a href="https://opensky-network.org" target="_blank" class="text-green-300 underline">opensky-network.org</a> (4,000 req/day authenticated)</span>
                </div>

                <hr class="border-green-500 opacity-40 my-2">

                <div class="px-1 font-bold text-lg sm:text-base text-green-300">Flight Details — Airlabs</div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>Airlabs API Key:</span>
                    <input
                        name="airlabs-key"
                        value='%AIRLABS_KEY%'
                        placeholder="Enter Airlabs API key"
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <div class="flex gap-2 px-1 text-xs sm:text-xs opacity-70">
                    <span>&rarr;</span>
                    <span>Free signup: <a href="https://airlabs.co/register" target="_blank" class="text-green-300 underline">airlabs.co/register</a> (1,000 req/month for flight details)</span>
                </div>

                <hr class="border-green-500 opacity-40 my-2">

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <input
                        type="submit"
                        value="Save"
                        class="bg-green-500 text-black mt-4 px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer">

                        <div id="result" class="mt-4 px-1 sm:px-10"></div>
                </div>
            </form>
        </fieldset>

        <script>
            document.getElementById('cfg').addEventListener('submit', function(e) {
                e.preventDefault();
                fetch(this.action, { method: 'POST', body: new FormData(this) })
                    .then(r => r.text())
                    .then(html => document.getElementById('result').innerHTML = html);
            });
        </script>
    </body>
</html>
)CONFIGHTML";

void ConfigurationWebServer::Initialise() {
    // start mDNS and check result
    if (!MDNS.begin("microradar")) {
        Serial.println("[WARN] Failed to start mDNS. Continuing without mDNS...");
    }

    // Handle visit to config web server
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Handling request to config web server...");

        prefs.begin("config", true);
        const String latitude = prefs.getString("latitude", "");
        const String longitude = prefs.getString("longitude", "");
        const String radius = prefs.getString("radius", "1.0");
        const String openskyId = prefs.getString("opensky-id", "");
        String openskySecret = prefs.getString("opensky-secret", "");
        const String airlabsKey = prefs.getString("airlabs-key", "");
        const String scanlineEnabled = prefs.getString("scanline", "true");
        const String infoTextEnabled = prefs.getString("infotext", "true");
        const String triangleEnabled = prefs.getString("triangle", "true");
        const String unitsValue = prefs.getString("units", "metric");
        prefs.end();

        std::fill(openskySecret.begin(), openskySecret.end(), '*');

        AsyncWebServerResponse* response = request->beginResponse(
            200, "text/html",
            (const uint8_t*)CONFIG_HTML, sizeof(CONFIG_HTML) - 1,
            [latitude, longitude, radius, openskyId, openskySecret, airlabsKey, scanlineEnabled, infoTextEnabled, triangleEnabled, unitsValue]
            (const String& var) -> String {
                if (var == "LATITUDE")        return latitude;
                if (var == "LONGITUDE")       return longitude;
                if (var == "RADIUS")          return radius;
                if (var == "OPENSKY_ID")      return openskyId;
                if (var == "OPENSKY_SECRET")  return openskySecret;
                if (var == "AIRLABS_KEY")     return airlabsKey;
                if (var == "SCANLINE")        return scanlineEnabled == "true" ? "checked" : "";
                if (var == "INFOTEXT")        return infoTextEnabled == "true" ? "checked" : "";
                if (var == "TRIANGLE")        return triangleEnabled == "true" ? "checked" : "";
                if (var == "UNITSMETRIC")     return unitsValue != "imperial" ? "selected" : "";
                if (var == "UNITSIMPERIAL")   return unitsValue == "imperial" ? "selected" : "";
                return "";
            }
        );
        request->send(response);
        }
    );

    // Handle save submission to web server
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Handling form submission to config web server...");

        auto TrySaveParam = [request, this](const char* paramName) {
            const auto* param = request->getParam(paramName, true);
            if (param == nullptr)
                return false;

            prefs.putString(paramName, param->value());
            return true;
            };

        prefs.begin("config", false);

        TrySaveParam("latitude");
        TrySaveParam("longitude");
        TrySaveParam("radius");
        TrySaveParam("opensky-id");

        const auto* secretParam = request->getParam("opensky-secret", true);
        if (secretParam != nullptr) {
            const String& secret = secretParam->value();
            if (secret.indexOf('*') == -1) {
                prefs.putString("opensky-secret", secret);
            }
        }

        TrySaveParam("airlabs-key");

        prefs.putString("scanline", request->hasParam("scanline", true) ? "true" : "false");
        prefs.putString("triangle", request->hasParam("triangle", true) ? "true" : "false");
        prefs.putString("infotext", request->hasParam("infotext", true) ? "true" : "false");
        TrySaveParam("units");

        prefs.end();

        request->send(200, "text/html", "Saved - restarting device...");
        ESP.restart();
        }
    );

    server.begin();
}

const String ConfigurationWebServer::GetStoredString(const char* key)
{
    prefs.begin("config", true);
    const String value = prefs.getString(key, "");
    prefs.end();
    return value;
}

void ConfigurationWebServer::SetStoredString(const char* key, String value)
{
    prefs.begin("config", false);
    prefs.putString(key, value);
    prefs.end();
}
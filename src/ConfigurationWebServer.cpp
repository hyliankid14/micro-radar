#include "ConfigurationWebServer.h"

#include <ESPmDNS.h>

void ConfigurationWebServer::Initialise() {
    MDNS.begin("microradar"); // hostname resolution for ip

    // Serve the config page
    server.on("/", HTTP_GET,
        [&](AsyncWebServerRequest* request) {
            Serial.println("[GET] Handling request to config web server...");

            prefs.begin("config", true);  // true = read only
            String location = prefs.getString("location", "");
            String radius = prefs.getString("radius", "50");
            String openskyKey = prefs.getString("opensky-key", "");
            prefs.end();

            String html = "<html><body>";
            html += "<h1>Configure Micro Radar</h1>";
            html += "<form action='/save' method='POST'>";
            html += "<label>Location: <input name='location' value='" + location + "'></label><br>";
            html += "<label>Radius: <input name='radius' value='" + radius + "'></label><br>";
            html += "<label>OpenSky API Key: <input name='opensky-key' value='" + openskyKey + "'></label><br>";
            html += "<input type='submit' value='Save'>";
            html += "</form></body></html>";

            request->send(200, "text/html", html);
        }
    );

    // Handle form submission
    server.on("/save", HTTP_POST,
        [&](AsyncWebServerRequest* request) {
            Serial.println("[POST] Handling form submission to config web server...");

            prefs.begin("config", false);
            if (request->hasParam("location", true))
                prefs.putString("location", request->getParam("location", true)->value());
            if (request->hasParam("radius", true))
                prefs.putString("radius", request->getParam("radius", true)->value());
            if (request->hasParam("opensky-key", true))
                prefs.putString("opensky-key", request->getParam("opensky-key", true)->value());
            prefs.end();

            request->send(200, "text/html", "<h1>Preferences saved - restarting...</h1>");

            delay(1000);
            ESP.restart();
        }
    );

    server.begin();
}
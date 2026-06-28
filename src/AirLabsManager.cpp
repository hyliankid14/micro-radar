#include "AirLabsManager.h"
#include <ArduinoJson.h>

AirLabsManager::AirLabsManager(ConfigurationWebServer& config, HttpRequestManager& httpManager) 
    : configServer(config), http(httpManager) {
    apiKey = configServer.GetStoredString("airlabs-key");
}

void AirLabsManager::setApiKey(String key) {
    apiKey = key;
    configServer.SetStoredString("airlabs-key", key);
}

bool AirLabsManager::updateSingle(const String& hex) {
    apiKey = configServer.GetStoredString("airlabs-key");
    if (apiKey.isEmpty()) {
        Serial.println("[AirLabs] No API key configured");
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
        "https://airlabs.co/api/v9/flights?api_key=%s&hex=%s",
        apiKey.c_str(), hex.c_str());

    Serial.printf("[AirLabs] Fetching: %s\n", url);

    HttpResult result = http.Get(url);

    if (!result.success) {
        Serial.printf("[AirLabs] HTTP error: %d — %s\n", result.statusCode, result.errorMessage.c_str());
        if (result.statusCode == 401 || result.statusCode == 403) {
            Serial.println("[AirLabs] API key invalid or unauthorized");
        } else if (result.statusCode == 429) {
            Serial.println("[AirLabs] Rate limit exceeded");
        }
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result.response);

    if (error) {
        Serial.printf("[AirLabs] JSON parse failed: %s\n", error.c_str());
        return false;
    }

    if (!doc["error"].isNull()) {
        String errorMsg = doc["error"]["message"].as<String>();
        int errorCode = doc["error"]["code"] | 0;
        Serial.printf("[AirLabs] API error (code %d): %s\n", errorCode, errorMsg.c_str());
        return false;
    }

    JsonArray flights;
    if (doc.is<JsonArray>()) {
        flights = doc.as<JsonArray>();
    } else {
        flights = doc["response"];
    }
    if (flights.isNull() || flights.size() == 0) {
        Serial.printf("[AirLabs] No data returned for hex %s\n", hex.c_str());
        return true;
    }

    JsonObject flight = flights[0];

    AirLabsAircraft ac;
    ac.hex = flight["hex"] | hex;
    ac.callsign = flight["flight_iata"] | "";
    ac.reg_number = flight["reg_number"] | "";
    ac.airline_icao = flight["airline_icao"] | "";
    ac.airline_iata = flight["airline_iata"] | "";
    ac.flight_icao = flight["flight_icao"] | "";

    ac.lat = flight["lat"] | 0.0;
    ac.lng = flight["lng"] | 0.0;
    ac.alt = flight["alt"] | 0;
    ac.dir = flight["dir"] | 0;

    float speedKmh = flight["speed"] | 0.0;
    ac.speed = speedKmh / 3.6;

    float vSpeedKmh = flight["v_speed"] | 0.0;
    ac.v_speed = vSpeedKmh / 3.6;

    ac.status = flight["status"] | "unknown";
    ac.dep_iata = flight["dep_iata"] | "";
    ac.arr_iata = flight["arr_iata"] | "";
    ac.dep_icao = flight["dep_icao"] | "";
    ac.arr_icao = flight["arr_icao"] | "";
    ac.dep_time = flight["dep_time"] | 0;
    ac.arr_time = flight["arr_time"] | 0;

    ac.updated = millis();

    ac.hex.toLowerCase();
    aircraft[ac.hex] = ac;

    Serial.printf("[AirLabs] Updated %s: callsign=%s, dep=%s, arr=%s\n",
        ac.hex.c_str(), ac.callsign.c_str(), ac.dep_iata.c_str(), ac.arr_iata.c_str());
    return true;
}

bool AirLabsManager::lookupByCallsign(const String& callsign, AirLabsAircraft& out) {
    apiKey = configServer.GetStoredString("airlabs-key");
    if (apiKey.isEmpty()) {
        return false;
    }

    String cs = callsign;
    cs.trim();
    if (cs.isEmpty()) {
        Serial.println("[AirLabs] Empty callsign, skipping");
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
        "https://airlabs.co/api/v9/flight?api_key=%s&flight_icao=%s",
        apiKey.c_str(), cs.c_str());

    Serial.printf("[AirLabs] Looking up flight: %s\n", cs.c_str());

    HttpResult result = http.Get(url);

    if (!result.success) {
        Serial.printf("[AirLabs] Flight lookup failed: %d — %s\n", result.statusCode, result.errorMessage.c_str());
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result.response);

    if (error) {
        Serial.printf("[AirLabs] Flight JSON parse failed: %s\n", error.c_str());
        return false;
    }

    if (!doc["error"].isNull()) {
        String errorMsg = doc["error"]["message"].as<String>();
        int errorCode = doc["error"]["code"] | 0;
        Serial.printf("[AirLabs] Flight API error (code %d): %s\n", errorCode, errorMsg.c_str());
        return false;
    }

    JsonObject flight;
    if (doc.containsKey("response")) {
        flight = doc["response"];
    }
    if (flight.isNull()) {
        Serial.printf("[AirLabs] No flight data for %s\n", cs.c_str());
        return false;
    }

    out.hex = flight["hex"] | "";
    out.flight_iata = flight["flight_iata"] | "";
    out.flight_icao = flight["flight_icao"] | "";
    out.dep_iata = flight["dep_iata"] | "";
    out.arr_iata = flight["arr_iata"] | "";
    out.dep_icao = flight["dep_icao"] | "";
    out.arr_icao = flight["arr_icao"] | "";
    out.dep_time = flight["dep_time_ts"] | 0;
    out.arr_time = flight["arr_time_ts"] | 0;
    out.status = flight["status"] | "";

    out.updated = millis();

    Serial.printf("[AirLabs] Found flight %s: %s -> %s\n",
        cs.c_str(), out.dep_iata.c_str(), out.arr_iata.c_str());
    return true;
}

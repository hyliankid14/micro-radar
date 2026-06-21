#include "AirLabsManager.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

AirLabsManager::AirLabsManager(ConfigurationWebServer& config, AirportDatabase* db) 
    : configServer(config), airportDb(db) {
    apiKey = configServer.GetStoredString("airlabs-key");
}

bool AirLabsManager::update(double centerLat, double centerLon, double radiusDeg) {
    apiKey = configServer.GetStoredString("airlabs-key");
    if (apiKey.isEmpty()) {
        Serial.println("[AirLabs] No API key configured");
        return false;
    }

    // Calculate bounding box
    double latMin = centerLat - radiusDeg;
    double latMax = centerLat + radiusDeg;
    double lngMin = centerLon - radiusDeg;
    double lngMax = centerLon + radiusDeg;

    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation for ESP32
    
    HTTPClient http;
    char url[256];
    snprintf(url, sizeof(url), 
        "https://airlabs.co/api/v9/flights?api_key=%s&bbox=%f,%f,%f,%f",
        apiKey.c_str(), latMin, lngMin, latMax, lngMax);

    Serial.println("[AirLabs] Fetching data...");
    Serial.printf("[AirLabs] URL: %s\n", url);
    
    http.begin(client, url);
    http.setTimeout(15000); // 15 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.printf("[AirLabs] JSON parse failed: %s\n", error.c_str());
            Serial.printf("[AirLabs] Payload: %s\n", payload.substring(0, 500).c_str());
            http.end();
            return false;
        }
        
        // Check for error response
        if (!doc["error"].isNull()) {
            Serial.printf("[AirLabs] API error: %s\n", 
                doc["error"]["message"].as<String>().c_str());
            http.end();
            return false;
        }
        
        // Response is a direct array of flights
        JsonArray flights = doc["response"];
        if (flights.isNull()) {
            Serial.println("[AirLabs] No response array found");
            http.end();
            return false;
        }
        
        aircraft.clear();
        
        for (JsonObject flight : flights) {
            AirLabsAircraft ac;
            
            ac.hex = flight["hex"] | "";
            ac.callsign = flight["flight_iata"] | "";
            ac.reg_number = flight["reg_number"] | "";
            ac.airline_icao = flight["airline_icao"] | "";
            ac.airline_iata = flight["airline_iata"] | "";
            ac.flight_icao = flight["flight_icao"] | "";
            
            ac.lat = flight["lat"] | 0.0;
            ac.lng = flight["lng"] | 0.0;
            ac.alt = flight["alt"] | 0;
            ac.dir = flight["dir"] | 0;
            
            // Convert speed from km/h to m/s for display
            float speedKmh = flight["speed"] | 0.0;
            ac.speed = speedKmh / 3.6;
            
            float vSpeedKmh = flight["v_speed"] | 0.0;
            ac.v_speed = vSpeedKmh / 3.6;
            
            ac.status = flight["status"] | "unknown";
            ac.dep_iata = flight["dep_iata"] | "";
            ac.arr_iata = flight["arr_iata"] | "";
            ac.dep_icao = flight["dep_icao"] | "";
            ac.arr_icao = flight["arr_icao"] | "";
            
            ac.updated = millis();
            
            aircraft[ac.hex] = ac;
        }
        
        Serial.printf("[AirLabs] Got %d aircraft\n", aircraft.size());
        http.end();
        return true;
        
    } else {
        Serial.printf("[AirLabs] HTTP error: %d\n", httpCode);
        if (httpCode > 0) {
            String errorPayload = http.getString();
            Serial.printf("[AirLabs] Error payload: %s\n", errorPayload.substring(0, 300).c_str());
        }
        http.end();
        return false;
    }
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

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    char url[256];
    snprintf(url, sizeof(url),
        "https://airlabs.co/api/v9/flights?api_key=%s&hex=%s",
        apiKey.c_str(), hex.c_str());

    Serial.printf("[AirLabs] Fetching single aircraft: %s\n", hex.c_str());

    http.begin(client, url);
    http.setTimeout(15000);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.printf("[AirLabs] JSON parse failed: %s\n", error.c_str());
            http.end();
            return false;
        }

        if (!doc["error"].isNull()) {
            Serial.printf("[AirLabs] API error: %s\n",
                doc["error"]["message"].as<String>().c_str());
            http.end();
            return false;
        }

        JsonArray flights = doc["response"];
        if (flights.isNull() || flights.size() == 0) {
            Serial.printf("[AirLabs] No data returned for hex %s\n", hex.c_str());
            http.end();
            return false;
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

        ac.updated = millis();

        aircraft[ac.hex] = ac;

        Serial.printf("[AirLabs] Updated single aircraft %s (%s)\n", ac.hex.c_str(), ac.callsign.c_str());
        http.end();
        return true;

    } else {
        Serial.printf("[AirLabs] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
}

AirportInfo AirLabsManager::getAirportInfo(const String& iataCode) {
    if (iataCode.isEmpty()) {
        return AirportInfo{"", ""};
    }
    
    // Check in-memory cache first
    auto it = airportCache.find(iataCode);
    if (it != airportCache.end()) {
        return it->second;
    }
    
    // Try local database if available
    if (airportDb != nullptr) {
        String name = airportDb->getAirportName(iataCode);
        if (!name.isEmpty()) {
            AirportInfo info;
            info.name = name;
            info.city = "";  // City not stored in local database
            airportCache[iataCode] = info;  // Cache for future lookups
            return info;
        }
    }
    
    // Not found in local database
    Serial.printf("[AirLabs] Airport %s not found in local database\n", iataCode.c_str());
    AirportInfo info{"", ""};
    airportCache[iataCode] = info;  // Cache negative result
    return info;
}

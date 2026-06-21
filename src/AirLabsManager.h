#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <map>
#include "ConfigurationWebServer.h"
#include "AirportDatabase.h"

struct AirLabsAircraft {
    String hex;           // ICAO address (e.g., "4CA8F2")
    String callsign;      // Flight number (e.g., "RYR1AB")
    String reg_number;    // Registration (e.g., "EI-DYG")
    String airline_icao;  // Airline code (e.g., "RYR")
    String airline_iata;  // Airline IATA (e.g., "FR")
    String flight_icao;   // Flight ICAO (e.g., "RYR1AB")
    String flight_iata;   // Flight IATA (e.g., "FR1234")
    
    float lat;
    float lng;
    int alt;              // Altitude in meters
    int dir;              // Heading in degrees
    float speed;          // Ground speed in km/h
    float v_speed;        // Vertical speed in km/h
    
    String status;        // "en-route", "landed", etc.
    String dep_iata;      // Departure airport IATA
    String arr_iata;      // Arrival airport IATA
    String dep_icao;      // Departure airport ICAO
    String arr_icao;      // Arrival airport ICAO
    
    unsigned long updated;
};

struct AirportInfo {
    String name;          // Full airport name
    String city;          // City name
};

class AirLabsManager {
private:
    String apiKey;
    std::map<String, AirLabsAircraft> aircraft;
    std::map<String, AirportInfo> airportCache;
    ConfigurationWebServer& configServer;
    AirportDatabase* airportDb;
    
public:
    AirLabsManager(ConfigurationWebServer& config, AirportDatabase* db = nullptr);
    
    void setAirportDatabase(AirportDatabase* db) { airportDb = db; }
    
    bool update(double centerLat, double centerLon, double radiusDeg);
    bool updateSingle(const String& hex);
    const std::map<String, AirLabsAircraft>& getAircraft() const { return aircraft; }
    
    AirportInfo getAirportInfo(const String& iataCode);
    
    void setApiKey(String key);
    String getApiKey() const { return apiKey; }
};

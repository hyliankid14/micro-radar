#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <map>
#include "ConfigurationWebServer.h"

struct AirLabsAircraft {
    String hex;
    String callsign;
    String reg_number;
    String airline_icao;
    String airline_iata;
    String flight_icao;
    String flight_iata;
    
    float lat;
    float lng;
    int alt;
    int dir;
    float speed;
    float v_speed;
    
    String status;
    String dep_iata;
    String arr_iata;
    String dep_icao;
    String arr_icao;
    long dep_time = 0;
    long arr_time = 0;
    
    unsigned long updated;
};

class AirLabsManager {
private:
    String apiKey;
    std::map<String, AirLabsAircraft> aircraft;
    ConfigurationWebServer& configServer;
    
public:
    AirLabsManager(ConfigurationWebServer& config);
    
    bool updateSingle(const String& hex);
    const std::map<String, AirLabsAircraft>& getAircraft() const { return aircraft; }
    
    void setApiKey(String key);
    String getApiKey() const { return apiKey; }
};

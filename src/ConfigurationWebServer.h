#pragma once

#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class AirportDatabase;

class ConfigurationWebServer {
private:
    AsyncWebServer server;
    Preferences prefs;
    AirportDatabase* airportDb;

public:
    ConfigurationWebServer() : server(80), prefs(), airportDb(nullptr) {}
    ConfigurationWebServer(int port) : server(port), prefs(), airportDb(nullptr) {}

    void Initialise();
    [[nodiscard]] const String GetStoredString(const char* key);
    void SetStoredString(const char* key, String value);
    
    void SetAirportDatabase(AirportDatabase* db);
};
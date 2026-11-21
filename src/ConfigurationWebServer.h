#pragma once

#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class AirportDatabase;
class AircraftManager;

class ConfigurationWebServer {
private:
    AsyncWebServer server;
    Preferences prefs;
    AirportDatabase* airportDb = nullptr;
    AircraftManager* aircraftMgr = nullptr;

public:
    ConfigurationWebServer() : server(80), prefs() {}
    ConfigurationWebServer(int port) : server(port), prefs() {}

    void Initialise();
    [[nodiscard]] const String GetStoredString(const char* key);
    void SetStoredString(const char* key, String value);

    void SetAirportDatabase(AirportDatabase* db) { airportDb = db; }
    void SetAircraftManager(AircraftManager* mgr) { aircraftMgr = mgr; }
};

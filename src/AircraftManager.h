#pragma once

#include <map>

#include "models/TrackedAircraft.h"
#include "ConfigurationWebServer.h"
#include "AirLabsManager.h"
#include "LGFX.h"

class AircraftManager
{
public:
    enum Units { METRIC, IMPERIAL };

    struct FlightDetail {
        bool loaded = false;
        bool attempted = false;
        String departureAirport;
        String departureAirportFull;
        String arrivalAirport;
        String arrivalAirportFull;
        long departureTime = 0;
        long arrivalTime = 0;
        unsigned long fetchedMs = 0;
    };

private:
    double lat = 0.0;
    double lon = 0.0;
    double rad = 0.2;
    Units units = METRIC;
    std::map<String, TrackedAircraft> trackedAircraft;
    std::map<String, FlightDetail> flightDetailCache;
    String selectedIcao;
    FlightDetail selectedDetail;

    bool displayInfoText = true;
    bool displayTriangles = true;

    unsigned long fetchInterval = 0;
    unsigned long lastFetch = 999999;

    static constexpr unsigned long SINGLE_FETCH_INTERVAL = 120000;
    unsigned long lastSingleFetch = 0;
    String lastSingleFetchHex;

    ConfigurationWebServer& configServer;
    AirLabsManager& airlabsManager;
    LGFX& tft;

    void DrawRadarCircles(LGFX_Sprite& backbuffer) const;
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;
    void DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const;
    void DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const;
    String LookupAirline(const String& callsign) const;

public:
    AircraftManager(ConfigurationWebServer& config, AirLabsManager& airlabs, LGFX& tftGfx)
        : configServer(config), airlabsManager(airlabs), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    void Initialise();
    void Update();
    void Draw(LGFX_Sprite& backbuffer);
    void DrawFlightDetail(LGFX_Sprite& backbuffer) const;

    double getRadius() const { return rad; }
    void adjustRadius(double delta);

    // Units conversion
    float ConvertSpeed(float mps) const { return units == IMPERIAL ? mps * 1.94384f : mps; }
    float ConvertAltitude(float m) const { return units == IMPERIAL ? m * 3.28084f : m; }
    String SpeedUnit() const { return units == IMPERIAL ? "kt" : "m/s"; }
    String AltUnit() const { return units == IMPERIAL ? "ft" : "m"; }

    // Flight detail interaction
    bool IsDetailOpen() const { return !selectedIcao.isEmpty(); }
    void SelectFlight(const String& icao24);
    void DismissDetail();
    String HitTestAircraft(int x, int y) const;
};
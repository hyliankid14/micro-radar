#pragma once

#include <map>
#include <set>
#include <vector>

#include "models/TrackedAircraft.h"
#include "ConfigurationWebServer.h"
#include "OpenSkyAuthTokenHandler.h"
#include "HttpRequestManager.h"
#include "AirLabsManager.h"
#include "AirportDatabase.h"
#include "LGFX.h"
#include "ThemePalette.h"
#include "AviationCatalog.h"
#include "VectorArt.h"
#include "MediaCacheManager.h"
#include "NetWorker.h"

class AircraftManager
{
public:
    enum Units { METRIC, IMPERIAL };

    struct FlightDetail {
        bool loaded = false;
        bool attempted = false;
        String departureAirport;
        String departureAirportFull;
        String departureCity;
        double depLat = 0.0;
        double depLon = 0.0;

        String arrivalAirport;
        String arrivalAirportFull;
        String arrivalCity;
        double arrLat = 0.0;
        double arrLon = 0.0;

        long departureTime = 0;
        long arrivalTime = 0;
        unsigned long fetchedMs = 0;

        // Airframe metadata discovered during the lookup (applied on the UI thread)
        String afReg;
        String afType;
        String afTypeDesc;
        String afOwner;
    };

    struct RareCatchEntry {
        String icao24;
        String callsign;
        String typeCode;
        String typeName;
        String badge;
        float distKm = 0.0f;
    };

    struct SpotterStats {
        size_t totalUniqueSeen = 0;
        float closestDistKm = 999999.0f;
        String closestCallsign;
        String closestType;

        float highestAltM = 0.0f;
        String highestCallsign;
        String highestType;

        float fastestSpeedMps = 0.0f;
        String fastestCallsign;
        String fastestType;

        std::vector<RareCatchEntry> rareCatches;
    };

private:
    double lat = 0.0;
    double lon = 0.0;
    double rad = 1.0;
    Units units = METRIC;
    String trackingProvider = "adsblol";

    DisplayViewMode viewMode = VIEW_RADAR_SPLIT;
    ThemeId themeId = THEME_RADAR_CRT;
    bool autoCycleViews = false;
    unsigned long lastViewCycleMs = 0;
    bool statsModalOpen = false;

    String giftTitle = "SKY FRAME";
    String giftSubtitle = "Live Aviation Display";
    int tzOffsetMinutes = 0;
    int dayBrightnessPct = 80;
    int nightBrightnessPct = 10;   // used during night quiet hours; 0 = screen off
    bool nightDimEnabled = false;
    int nightStartHour = 23;
    int nightEndHour = 7;

    String pinnedQuery;
    String pinnedIcao;
    unsigned long lastPinnedFetchMs = 0;

    std::map<String, TrackedAircraft> trackedAircraft;
    std::map<String, FlightDetail> flightDetailCache;
    std::set<String> seenIcaosSession;
    SpotterStats stats;

    String selectedIcao;
    FlightDetail selectedDetail;

    bool displayInfoText = true;
    bool displayTriangles = true;
    bool displayScanline = true;

    unsigned long fetchInterval = 0;
    unsigned long lastFetch = 999999;

    static constexpr unsigned long SINGLE_FETCH_INTERVAL = 120000;
    unsigned long lastSingleFetch = 0;
    String lastSingleFetchHex;

    static constexpr unsigned long ROUTE_LOOKUP_COOLDOWN_MS = 2000;
    unsigned long lastRouteLookupMs = 0;

    ConfigurationWebServer& configServer;
    AirLabsManager& airlabsManager;
    OpenSkyAuthTokenHandler& authHandler;
    HttpRequestManager& http;
    AirportDatabase* airportDb;
    LGFX& tft;
    MediaCacheManager mediaCache;

    // ---- Background networking (all HTTP runs on the NetWorker task) ----
    NetWorker net;
    SemaphoreHandle_t resultMutex = nullptr;
    std::map<String, FlightDetail> detailResults;   // completed lookups awaiting pickup
    std::vector<Aircraft> areaResult;
    bool areaReady = false;
    bool areaOk = false;
    Aircraft pinnedResult;
    bool pinnedReady = false;
    Aircraft singleResult;
    bool singleReady = false;
    String singleResultIcao;
    uint32_t dataVersion = 0;            // bumps whenever visible data changes (drives redraws)

    void ApplyNetworkResults();
    void RequestFlightDetail(const String& icao, bool allowAirlabs, int prio);
    void ApplyPinnedAircraft(const Aircraft& pinnedAc);
    void PrefetchNextInRotation(unsigned long now);

    void DrawRadarCircles(LGFX_Sprite& backbuffer, const ThemePalette& theme) const;
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;
    void DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected) const;
    void DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected, bool isPinned) const;
    void DrawAircraftTrail(LGFX_Sprite& backbuffer, const TrackedAircraft& tracked, const ThemePalette& theme, bool isSelected) const;

    void DrawSplitRadarView(LGFX_Sprite& backbuffer, const ThemePalette& theme);
    void DrawIdleWelcomePanel(LGFX_Sprite& backbuffer, const ThemePalette& theme) const;
    void DrawDepartureBoardView(LGFX_Sprite& backbuffer, const ThemePalette& theme);
    void DrawPortraitPosterView(LGFX_Sprite& backbuffer, const ThemePalette& theme);
    void DrawStatsModal(LGFX_Sprite& backbuffer, const ThemePalette& theme) const;

    bool ParseAdsbLolAircraftObj(JsonVariant v, Aircraft& ac) const;
    bool FetchAreaAdsbLol(std::vector<Aircraft>& out);
    bool FetchAreaOpenSky(std::vector<Aircraft>& out);
    bool FetchPinnedFlightAdsbLol(const String& query, Aircraft& outAc);
    void UpdateSpotterStats(const Aircraft& ac);

    void ResolveAirportNames(FlightDetail& detail) const;
    bool FetchFlightDetailAdsbdb(const String& callsign, const String& icao, bool needAirframe, FlightDetail& detail);
    bool FetchFlightDetailHexdb(const String& callsign, FlightDetail& detail);
    bool FetchFlightDetailAirlabs(const String& icao, const String& callsign, FlightDetail& detail);
    void PopulateFlightDetail(const String& icao, bool allowAirlabs);
    bool FetchSingleAircraftOpenSky(const String& icao, Aircraft& out);

    struct SortedFlightItem {
        String icao;
        float distKm;
        float bearingDeg;
        bool isPinned;
    };
    std::vector<SortedFlightItem> GetSortedActiveFlights() const;

#ifdef CROWPANEL_7
    bool automaticSelection = false;
    bool interactionPaused = false;
    unsigned long lastInteractionMs = 0;
    unsigned long lastRotationMs = 0;
    bool IsAircraftVisible(const TrackedAircraft& tracked) const;
#endif

public:
    AircraftManager(
        ConfigurationWebServer& config,
        AirLabsManager& airlabs,
        OpenSkyAuthTokenHandler& auth,
        HttpRequestManager& httpManager,
        AirportDatabase* airportDb,
        LGFX& tftGfx)
        : configServer(config), airlabsManager(airlabs), authHandler(auth), http(httpManager), airportDb(airportDb), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    void Initialise();
    void ReloadRuntimeSettings();
    void Update();
    void Draw(LGFX_Sprite& backbuffer);
    void DrawFlightDetail(LGFX_Sprite& backbuffer);
    // Monotonic counter: changes when new network data/images arrive (lets the UI redraw only when needed)
    uint32_t ContentVersion() const { return dataVersion + mediaCache.Version(); }
    NetWorker& Net() { return net; }

    // Cheap fingerprint of everything that changes what is on screen besides continuous animation.
    uint32_t RenderSignature() const {
        uint32_t h = static_cast<uint32_t>(viewMode) * 31u + static_cast<uint32_t>(themeId);
        h = h * 31u + (statsModalOpen ? 1u : 0u);
        for (size_t i = 0; i < selectedIcao.length(); ++i) h = h * 131u + static_cast<uint8_t>(selectedIcao[i]);
        h = h * 31u + ContentVersion();
        return h;
    }
    bool IsAnimatedView() const { return viewMode == VIEW_RADAR_SPLIT; }

    double getRadius() const { return rad; }
    void adjustRadius(double delta);
    void setRadius(double newRad);

    DisplayViewMode GetViewMode() const { return viewMode; }
    void SetViewMode(DisplayViewMode mode, bool persist = true);
    void CycleViewMode();

    ThemeId GetThemeId() const { return themeId; }
    const ThemePalette& GetTheme() const { return GetThemePalette(themeId); }
    void SetThemeId(ThemeId id, bool persist = true);
    void CycleTheme();

    bool IsStatsModalOpen() const { return statsModalOpen; }
    void ToggleStatsModal() { statsModalOpen = !statsModalOpen; }
    void CloseStatsModal() { statsModalOpen = false; }

    void SetPinnedFlightQuery(const String& query, bool persist = true);
    String GetPinnedFlightQuery() const { return pinnedQuery; }
    String GetPinnedIcao() const { return pinnedIcao; }

    void SelectNextFlight(int direction = 1);
    bool HandleContentTap(int x, int y);

    float ConvertSpeed(float mps) const { return units == IMPERIAL ? mps * 1.94384f : mps; }
    float ConvertAltitude(float m) const { return units == IMPERIAL ? m * 3.28084f : m; }
    float ConvertDistance(float km) const { return units == IMPERIAL ? km * 0.621371f : km; }
    String SpeedUnit() const { return units == IMPERIAL ? "kt" : "m/s"; }
    String AltUnit() const { return units == IMPERIAL ? "ft" : "m"; }
    String DistUnit() const { return units == IMPERIAL ? "mi" : "km"; }

    bool IsDetailOpen() const { return !selectedIcao.isEmpty(); }
    String GetSelectedIcao() const { return selectedIcao; }
    size_t GetTrackedCount() const { return trackedAircraft.size(); }
    bool HasLocationConfigured() const { return lat != 0.0 || lon != 0.0; }
    double GetLatitude() const { return lat; }
    double GetLongitude() const { return lon; }
    String GetGiftTitle() const { return giftTitle; }
    String GetGiftSubtitle() const { return giftSubtitle; }
    bool IsNightQuietHours() const;

    int GetBrightness() const { return dayBrightnessPct; }
    int GetNightBrightness() const { return nightBrightnessPct; }
    void SetBrightness(int pct, bool persist);

    const SpotterStats& GetSpotterStats() const { return stats; }
    String BuildStatusJson() const;

    void SelectFlight(const String& icao24);
    void DismissDetail();
    String HitTestAircraft(int x, int y) const;
#ifdef CROWPANEL_7
    void NoteInteraction(unsigned long now);
    void UpdateIdleSelection(unsigned long now);
#endif
};

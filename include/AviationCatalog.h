#pragma once

#include <Arduino.h>
#include <cmath>
#include "LGFX.h"

enum SilhouetteCategory {
    SIL_NARROWBODY = 0,
    SIL_WIDEBODY_TWIN = 1,
    SIL_WIDEBODY_QUAD = 2,
    SIL_REGIONAL_PROP = 3,
    SIL_BIZJET = 4,
    SIL_GA_LIGHT = 5,
    SIL_HELICOPTER = 6,
    SIL_MILITARY = 7
};

struct AirlineInfo {
    String icao;
    String iata;
    String name;
    String formattedFlight;
    uint32_t primaryColor;
    uint32_t accentColor;
    uint32_t textColor;
    bool matched = false;
};

struct AircraftTypeInfo {
    String code;
    String fullName;
    SilhouetteCategory category = SIL_NARROWBODY;
    bool isRareCatch = false;
    String rareBadge;
};

namespace AviationCatalog
{
    struct StaticAirline {
        const char* icao;
        const char* iata;
        const char* name;
        uint8_t pr, pg, pb;
        uint8_t ar, ag, ab;
        uint8_t tr, tg, tb;
    };

    static const StaticAirline AIRLINES[] = {
        // UK & Ireland
        {"BAW", "BA", "British Airways",       12, 35, 64,   198, 12, 48,  255, 255, 255},
        {"SHT", "BA", "BA Shuttle",            12, 35, 64,   198, 12, 48,  255, 255, 255},
        {"CFE", "BA", "BA CityFlyer",          12, 35, 64,   198, 12, 48,  255, 255, 255},
        {"EFW", "BA", "BA Euroflyer",          12, 35, 64,   198, 12, 48,  255, 255, 255},
        {"VIR", "VS", "Virgin Atlantic",      180, 12, 58,   115, 20, 95,  255, 255, 255},
        {"EZY", "U2", "easyJet",              255, 102, 0,    35, 35, 35,  255, 255, 255},
        {"EZS", "DS", "easyJet Switzerland",  255, 102, 0,   200, 16, 46,  255, 255, 255},
        {"EJU", "EC", "easyJet Europe",       255, 102, 0,    35, 35, 35,  255, 255, 255},
        {"RYR", "FR", "Ryanair",                7, 53, 144,  241, 201, 51, 255, 255, 255},
        {"RUK", "RK", "Ryanair UK",             7, 53, 144,  241, 201, 51, 255, 255, 255},
        {"EXS", "LS", "Jet2.com",             215, 25, 32,   130, 140, 150,255, 255, 255},
        {"TOM", "BY", "TUI Airways",            0, 155, 223, 212, 14, 20,  255, 255, 255},
        {"EIN", "EI", "Aer Lingus",             0, 114, 110, 115, 200, 44, 255, 255, 255},
        {"LOG", "LM", "Loganair",              20, 28, 52,   212, 32, 40,  255, 255, 255},
        {"AUR", "GR", "Aurigny",              242, 185, 0,   200, 20, 20,   20,  20,  20},
        {"EAI", "EA", "Emerald Airlines",       0, 114, 110, 115, 200, 44, 255, 255, 255},

        // Europe
        {"DLH", "LH", "Lufthansa",              5, 22, 77,   255, 173, 0,  255, 255, 255},
        {"CLH", "CL", "Lufthansa CityLine",     5, 22, 77,   255, 173, 0,  255, 255, 255},
        {"AFR", "AF", "Air France",             0, 33, 87,   237, 41, 57,  255, 255, 255},
        {"KLM", "KL", "KLM Royal Dutch",        0, 161, 222,   0, 91, 130, 255, 255, 255},
        {"KLC", "WA", "KLM Cityhopper",         0, 161, 222,   0, 91, 130, 255, 255, 255},
        {"SWR", "LX", "SWISS",                204,  0,  0,   255, 255, 255,255, 255, 255},
        {"AUA", "OS", "Austrian Airlines",    216, 30,  5,   255, 255, 255,255, 255, 255},
        {"BEL", "SN", "Brussels Airlines",     14, 34, 84,   226,  0, 26,  255, 255, 255},
        {"IBE", "IB", "Iberia",               215, 25, 45,   247, 171, 0,  255, 255, 255},
        {"IBS", "I2", "Iberia Express",       215, 25, 45,   247, 171, 0,  255, 255, 255},
        {"VLG", "VY", "Vueling",              246, 200,  0,   75, 79, 84,   25,  25,  25},
        {"AEA", "UX", "Air Europa",             0, 114, 206, 220, 30, 40,  255, 255, 255},
        {"TAP", "TP", "TAP Air Portugal",       0, 150, 64,  228,  0, 43,  255, 255, 255},
        {"ITY", "AZ", "ITA Airways",            0, 68, 148,    0, 146, 70, 255, 255, 255},
        {"AZA", "AZ", "ITA Airways",            0, 68, 148,    0, 146, 70, 255, 255, 255},
        {"SAS", "SK", "SAS Scandinavian",       0,  0, 153,  212, 32, 39,  255, 255, 255},
        {"SZS", "SK", "SAS Connect",            0,  0, 153,  212, 32, 39,  255, 255, 255},
        {"FIN", "AY", "Finnair",               11, 21, 96,   255, 255, 255,255, 255, 255},
        {"NOZ", "DY", "Norwegian",            216, 25, 57,     0, 38, 62,  255, 255, 255},
        {"NSZ", "D8", "Norwegian Intl",       216, 25, 57,     0, 38, 62,  255, 255, 255},
        {"ICE", "FI", "Icelandair",             0, 56, 118,  245, 168, 0,  255, 255, 255},
        {"WZZ", "W6", "Wizz Air",             198,  0, 126,    6,  3, 141, 255, 255, 255},
        {"WUK", "W9", "Wizz Air UK",          198,  0, 126,    6,  3, 141, 255, 255, 255},
        {"WMT", "W4", "Wizz Air Malta",       198,  0, 126,    6,  3, 141, 255, 255, 255},
        {"EWG", "EW", "Eurowings",            130, 18, 72,     0, 140, 205,255, 255, 255},
        {"CFG", "DE", "Condor",               235, 170,  0,   30, 50, 90,   20,  20,  20},
        {"TUI", "X3", "TUIfly",                 0, 155, 223, 212, 14, 20,  255, 255, 255},
        {"TRA", "HV", "Transavia",              0, 214, 108,  15, 45, 95,   15,  35,  25},
        {"TVF", "TO", "Transavia France",       0, 214, 108,  15, 45, 95,   15,  35,  25},
        {"LOT", "LO", "LOT Polish Airlines",   16, 38, 112,  212, 24, 38,  255, 255, 255},
        {"BTI", "BT", "airBaltic",            152, 202,  0,   18, 42, 72,   18,  32,  18},
        {"AEE", "A3", "Aegean Airlines",        0, 44, 119,  215, 25, 32,  255, 255, 255},
        {"THY", "TK", "Turkish Airlines",     199, 10, 12,    26, 34, 48,  255, 255, 255},
        {"PGT", "PC", "Pegasus Airlines",     245, 179,  0,   42, 42, 42,   25,  25,  25},
        {"SXS", "XQ", "SunExpress",           242, 145,  0,    0, 88, 168, 255, 255, 255},
        {"TAR", "TU", "Tunisair",             208, 22, 36,   255, 255, 255,255, 255, 255},
        {"RAM", "AT", "Royal Air Maroc",      184, 18, 38,     0, 110, 65, 255, 255, 255},
        {"LGL", "LG", "Luxair",                 0, 142, 198,  18, 46, 92,  255, 255, 255},
        {"CSA", "OK", "Czech Airlines",       196, 18, 48,    14, 42, 96,  255, 255, 255},
        {"ROT", "RO", "TAROM",                 12, 42, 104,  230, 180, 20, 255, 255, 255},
        {"ASL", "JU", "Air Serbia",            14, 36, 82,   196, 22, 42,  255, 255, 255},
        {"CTN", "OU", "Croatia Airlines",      12, 44, 102,  208, 20, 36,  255, 255, 255},
        {"KMM", "KM", "KM Malta Airlines",    208, 18, 36,   255, 255, 255,255, 255, 255},
        {"NOS", "NO", "Neos",                   0, 128, 200,  16, 40, 80,  255, 255, 255},

        // Middle East & Africa
        {"UAE", "EK", "Emirates",             215, 25, 33,   198, 160, 82, 255, 255, 255},
        {"QTR", "QR", "Qatar Airways",         92,  6, 50,   142, 148, 156,255, 255, 255},
        {"ETD", "EY", "Etihad Airways",       189, 139, 19,   58, 38, 24,  255, 255, 255},
        {"SVA", "SV", "Saudia",                 0, 103, 71,  184, 152, 82, 255, 255, 255},
        {"GFA", "GF", "Gulf Air",             168, 132, 68,   18, 38, 72,  255, 255, 255},
        {"OMA", "WY", "Oman Air",               0, 114, 152, 184, 148, 76, 255, 255, 255},
        {"KAC", "KU", "Kuwait Airways",        12, 52, 122,  190, 160, 95, 255, 255, 255},
        {"RJA", "RJ", "Royal Jordanian",       34, 34, 38,   204, 24, 38,  255, 255, 255},
        {"MEA", "ME", "Middle East Airlines",  16, 118, 56,  204, 24, 36,  255, 255, 255},
        {"ELY", "LY", "El Al",                 12, 54, 138,  180, 195, 220,255, 255, 255},
        {"FDB", "FZ", "flydubai",               0, 102, 179, 245, 130, 32, 255, 255, 255},
        {"ABY", "G9", "Air Arabia",           214, 22, 38,   110, 116, 124,255, 255, 255},
        {"ETH", "ET", "Ethiopian Airlines",    16, 124, 52,  238, 190, 18, 255, 255, 255},
        {"MSR", "MS", "EgyptAir",              14, 42, 98,   196, 158, 78, 255, 255, 255},
        {"SAA", "SA", "South African",         14, 56, 108,  220, 165, 25, 255, 255, 255},
        {"KQA", "KQ", "Kenya Airways",        196, 20, 36,    22, 98, 48,  255, 255, 255},

        // North America & Latin America
        {"AAL", "AA", "American Airlines",     12, 54, 108,  198, 24, 42,  255, 255, 255},
        {"DAL", "DL", "Delta Air Lines",       14, 34, 68,   208, 20, 44,  255, 255, 255},
        {"UAL", "UA", "United Airlines",        0, 51, 160,   65, 150, 230,255, 255, 255},
        {"SWA", "WN", "Southwest Airlines",    48, 76, 178,  228, 32, 42,  255, 255, 255},
        {"JBU", "B6", "JetBlue",                0, 56, 118,    0, 165, 225,255, 255, 255},
        {"ASA", "AS", "Alaska Airlines",        1, 66, 106,   68, 178, 112,255, 255, 255},
        {"NKS", "NK", "Spirit Airlines",      255, 215,  0,   24, 24, 24,   20,  20,  20},
        {"FFT", "F9", "Frontier Airlines",      0, 114, 78,  125, 185, 85, 255, 255, 255},
        {"HAL", "HA", "Hawaiian Airlines",     88, 24, 118,  214, 28, 112, 255, 255, 255},
        {"SKW", "OO", "SkyWest Airlines",      16, 48, 98,   160, 175, 195,255, 255, 255},
        {"RPA", "YX", "Republic Airways",      14, 42, 86,   196, 24, 42,  255, 255, 255},
        {"EDV", "9E", "Endeavor Air",          14, 34, 68,   208, 20, 44,  255, 255, 255},
        {"ENY", "MQ", "Envoy Air",             12, 54, 108,  198, 24, 42,  255, 255, 255},
        {"ACA", "AC", "Air Canada",            24, 28, 34,   216, 24, 38,  255, 255, 255},
        {"ROU", "RV", "Air Canada Rouge",     168, 18, 44,    24, 28, 34,  255, 255, 255},
        {"WJA", "WS", "WestJet",                0, 114, 132,  14, 46, 78,  255, 255, 255},
        {"TSC", "TS", "Air Transat",            0, 118, 190,  16, 44, 88,  255, 255, 255},
        {"POE", "PD", "Porter Airlines",       18, 38, 72,   160, 175, 195,255, 255, 255},
        {"AMX", "AM", "Aeromexico",            11, 35, 67,   208, 22, 42,  255, 255, 255},
        {"VOI", "Y4", "Volaris",              142, 24, 126,   28, 28, 32,  255, 255, 255},
        {"CMP", "CM", "Copa Airlines",          0, 74, 148,  196, 158, 76, 255, 255, 255},
        {"AVA", "AV", "Avianca",              212, 18, 38,    34, 34, 38,  255, 255, 255},
        {"LAN", "LA", "LATAM Airlines",        27,  0, 136,  232, 17, 75,  255, 255, 255},
        {"TAM", "JJ", "LATAM Brasil",          27,  0, 136,  232, 17, 75,  255, 255, 255},
        {"AZU", "AD", "Azul Airlines",          0, 96, 178,   90, 190, 235,255, 255, 255},
        {"GLO", "G3", "GOL Linhas Aereas",    242, 108, 18,   68, 68, 72,  255, 255, 255},
        {"ARG", "AR", "Aerolineas Argentinas",  0, 130, 202, 180, 215, 245,255, 255, 255},

        // Asia-Pacific
        {"SIA", "SQ", "Singapore Airlines",    18, 38, 86,   232, 168, 24, 255, 255, 255},
        {"CPA", "CX", "Cathay Pacific",         0, 101, 100, 168, 180, 178,255, 255, 255},
        {"QFA", "QF", "Qantas",               228,  0,  0,   255, 255, 255,255, 255, 255},
        {"JST", "JQ", "Jetstar",              255, 88,  0,    32, 32, 36,  255, 255, 255},
        {"VOZ", "VA", "Virgin Australia",     196, 18, 52,    78, 22, 92,  255, 255, 255},
        {"ANZ", "NZ", "Air New Zealand",       18, 22, 28,     0, 152, 166,255, 255, 255},
        {"ANA", "NH", "All Nippon Airways",     0, 56, 148,    0, 150, 225,255, 255, 255},
        {"JAL", "JL", "Japan Airlines",       198, 16, 32,    36, 36, 40,  255, 255, 255},
        {"KAL", "KE", "Korean Air",             0, 102, 179, 208, 24, 42,  255, 255, 255},
        {"AAR", "OZ", "Asiana Airlines",      122, 98, 78,   204, 24, 42,  255, 255, 255},
        {"EVA", "BR", "EVA Air",                0, 118, 74,  238, 118, 28, 255, 255, 255},
        {"CAL", "CI", "China Airlines",        24, 52, 118,  214, 48, 102, 255, 255, 255},
        {"CCA", "CA", "Air China",            198, 20, 36,    16, 54, 118, 255, 255, 255},
        {"CES", "MU", "China Eastern",         16, 48, 112,  208, 24, 42,  255, 255, 255},
        {"CSN", "CZ", "China Southern",         0, 112, 192, 212, 22, 42,  255, 255, 255},
        {"CHH", "HU", "Hainan Airlines",      198, 22, 36,   232, 178, 24, 255, 255, 255},
        {"CSC", "3U", "Sichuan Airlines",     196, 22, 36,   225, 175, 30, 255, 255, 255},
        {"CXA", "MF", "XiamenAir",              0, 108, 190, 220, 235, 250,255, 255, 255},
        {"THA", "TG", "Thai Airways",          88, 24, 118,  232, 168, 32, 255, 255, 255},
        {"MAS", "MH", "Malaysia Airlines",     12, 42, 98,   208, 22, 42,  255, 255, 255},
        {"AXM", "AK", "AirAsia",              220, 16, 28,   255, 255, 255,255, 255, 255},
        {"GIA", "GA", "Garuda Indonesia",       0, 92, 142,    0, 165, 185,255, 255, 255},
        {"PAL", "PR", "Philippine Airlines",   12, 46, 108,  208, 24, 42,  255, 255, 255},
        {"HVN", "VN", "Vietnam Airlines",       0, 94, 126,  222, 172, 46, 255, 255, 255},
        {"AIC", "AI", "Air India",            188, 18, 44,   108, 24, 96,  255, 255, 255},
        {"IGO", "6E", "IndiGo",                 0, 38, 148,   85, 155, 235,255, 255, 255},

        // Cargo & Special Operators
        {"FDX", "FX", "FedEx Express",         77, 20, 140,  255, 102,  0, 255, 255, 255},
        {"UPS", "5X", "UPS Airlines",          53, 28, 21,   255, 181,  0, 255, 255, 255},
        {"DHK", "D0", "DHL Air UK",           245, 196,  0,  212,  5, 17,   20,  20,  20},
        {"BCS", "QY", "European Air Transport",245,196,  0,  212,  5, 17,   20,  20,  20},
        {"BOX", "3S", "AeroLogic",            245, 196,  0,  212,  5, 17,   20,  20,  20},
        {"CLX", "CV", "Cargolux",             208, 20, 38,    20, 56, 118, 255, 255, 255},
        {"GTI", "5Y", "Atlas Air",             14, 46, 102,  212, 168, 42, 255, 255, 255},
        {"CKS", "K4", "Kalitta Air",          184, 24, 38,   216, 168, 44, 255, 255, 255},
        {"NJE", "NJ", "NetJets",               26, 40, 68,   165, 175, 190,255, 255, 255},
        {"VJT", "VJ", "VistaJet",             145, 152, 160, 192, 18, 36,   20,  20,  25},
        {"RCH", "MC", "US Air Force AMC",      24, 46, 78,   160, 175, 190,255, 255, 255},
        {"RRR", "RR", "Royal Air Force",       20, 48, 92,   204, 24, 44,  255, 255, 255}
    };

    inline AirlineInfo LookupAirlineInfo(const String& callsign, const String& fallbackOp = "")
    {
        AirlineInfo info;
        String cs = callsign;
        cs.trim();
        cs.toUpperCase();

        // Extract alphabetic prefix and trailing flight digits/alphanumerics
        String alphaPrefix = "";
        int digitStart = -1;
        for (unsigned int i = 0; i < cs.length(); ++i) {
            char c = cs.charAt(i);
            if (c >= '0' && c <= '9') {
                digitStart = i;
                break;
            }
            alphaPrefix += c;
        }
        String flightSuffix = (digitStart >= 0) ? cs.substring(digitStart) : "";

        const int count = sizeof(AIRLINES) / sizeof(AIRLINES[0]);

        // 1. Try 3-letter ICAO prefix match (standard for ADS-B callsigns)
        if (alphaPrefix.length() >= 3) {
            String prefix3 = alphaPrefix.substring(0, 3);
            for (int i = 0; i < count; ++i) {
                if (prefix3 == AIRLINES[i].icao) {
                    info.icao = AIRLINES[i].icao;
                    info.iata = AIRLINES[i].iata;
                    info.name = AIRLINES[i].name;
                    info.formattedFlight = flightSuffix.isEmpty() ? cs : (String(AIRLINES[i].iata) + " " + flightSuffix);
                    info.primaryColor = lgfx::color888(AIRLINES[i].pr, AIRLINES[i].pg, AIRLINES[i].pb);
                    info.accentColor = lgfx::color888(AIRLINES[i].ar, AIRLINES[i].ag, AIRLINES[i].ab);
                    info.textColor = lgfx::color888(AIRLINES[i].tr, AIRLINES[i].tg, AIRLINES[i].tb);
                    info.matched = true;
                    return info;
                }
            }
        }

        // 2. Try 2-letter IATA prefix match
        if (cs.length() >= 2) {
            String prefix2 = cs.substring(0, 2);
            for (int i = 0; i < count; ++i) {
                if (prefix2 == AIRLINES[i].iata) {
                    info.icao = AIRLINES[i].icao;
                    info.iata = AIRLINES[i].iata;
                    info.name = AIRLINES[i].name;
                    info.formattedFlight = cs;
                    info.primaryColor = lgfx::color888(AIRLINES[i].pr, AIRLINES[i].pg, AIRLINES[i].pb);
                    info.accentColor = lgfx::color888(AIRLINES[i].ar, AIRLINES[i].ag, AIRLINES[i].ab);
                    info.textColor = lgfx::color888(AIRLINES[i].tr, AIRLINES[i].tg, AIRLINES[i].tb);
                    info.matched = true;
                    return info;
                }
            }
        }

        // 3. Fallback: Private / GA / Unlisted operator
        info.icao = alphaPrefix.length() >= 3 ? alphaPrefix.substring(0, 3) : cs.substring(0, min((int)cs.length(), 3));
        info.iata = info.icao.length() >= 2 ? info.icao.substring(0, 2) : "GA";
        if (!fallbackOp.isEmpty()) {
            info.name = fallbackOp;
        } else if (cs.startsWith("N") && cs.length() >= 4 && isDigit(cs.charAt(1))) {
            info.name = "Private / US Registry";
            info.iata = "GA";
        } else if (cs.startsWith("G") && cs.length() == 5) {
            info.name = "Private / UK Registry";
            info.iata = "GA";
        } else if (!info.icao.isEmpty()) {
            info.name = "Operator " + info.icao;
        } else {
            info.name = "General Aviation";
        }
        info.formattedFlight = cs;
        // Deterministic rich badge color from callsign hash
        uint32_t h = 2166136261u;
        for (unsigned int i = 0; i < cs.length(); ++i) {
            h = (h ^ (uint8_t)cs.charAt(i)) * 16777619u;
        }
        uint8_t r = 25 + (h & 0x5F);
        uint8_t g = 40 + ((h >> 8) & 0x5F);
        uint8_t b = 75 + ((h >> 16) & 0x6F);
        info.primaryColor = lgfx::color888(r, g, b);
        info.accentColor = lgfx::color888(min(255, r + 90), min(255, g + 80), min(255, b + 60));
        info.textColor = lgfx::color888(255, 255, 255);
        info.matched = false;
        return info;
    }

    struct StaticAircraftType {
        const char* code;
        const char* fullName;
        SilhouetteCategory category;
        bool isRare;
        const char* rareBadge;
    };

    static const StaticAircraftType AIRCRAFT_TYPES[] = {
        // Airbus Narrowbody
        {"A318", "Airbus A318",              SIL_NARROWBODY,    true,  "BABY BUS"},
        {"A319", "Airbus A319",              SIL_NARROWBODY,    false, ""},
        {"A19N", "Airbus A319neo",           SIL_NARROWBODY,    false, ""},
        {"A320", "Airbus A320",              SIL_NARROWBODY,    false, ""},
        {"A20N", "Airbus A320neo",           SIL_NARROWBODY,    false, ""},
        {"A321", "Airbus A321",              SIL_NARROWBODY,    false, ""},
        {"A21N", "Airbus A321neo",           SIL_NARROWBODY,    false, ""},
        {"A221", "Airbus A220-100",          SIL_NARROWBODY,    false, ""},
        {"A223", "Airbus A220-300",          SIL_NARROWBODY,    false, ""},
        {"BCS1", "Airbus A220-100",          SIL_NARROWBODY,    false, ""},
        {"BCS3", "Airbus A220-300",          SIL_NARROWBODY,    false, ""},

        // Airbus Widebody
        {"A306", "Airbus A300-600",          SIL_WIDEBODY_TWIN, true,  "CLASSIC JET"},
        {"A332", "Airbus A330-200",          SIL_WIDEBODY_TWIN, false, ""},
        {"A333", "Airbus A330-300",          SIL_WIDEBODY_TWIN, false, ""},
        {"A338", "Airbus A330-800neo",       SIL_WIDEBODY_TWIN, true,  "RARE A338"},
        {"A339", "Airbus A330-900neo",       SIL_WIDEBODY_TWIN, false, ""},
        {"A343", "Airbus A340-300",          SIL_WIDEBODY_QUAD, true,  "QUAD JET"},
        {"A346", "Airbus A340-600",          SIL_WIDEBODY_QUAD, true,  "QUAD JET"},
        {"A359", "Airbus A350-900",          SIL_WIDEBODY_TWIN, false, ""},
        {"A35K", "Airbus A350-1000",         SIL_WIDEBODY_TWIN, false, ""},
        {"A388", "Airbus A380-800",          SIL_WIDEBODY_QUAD, true,  "SUPERJUMBO"},
        {"A3ST", "Airbus Beluga ST",         SIL_WIDEBODY_TWIN, true,  "BELUGA"},
        {"A337", "Airbus BelugaXL",          SIL_WIDEBODY_TWIN, true,  "BELUGA XL"},

        // Boeing Narrowbody
        {"B712", "Boeing 717-200",           SIL_NARROWBODY,    false, ""},
        {"B733", "Boeing 737-300",           SIL_NARROWBODY,    false, ""},
        {"B734", "Boeing 737-400",           SIL_NARROWBODY,    false, ""},
        {"B735", "Boeing 737-500",           SIL_NARROWBODY,    false, ""},
        {"B736", "Boeing 737-600",           SIL_NARROWBODY,    false, ""},
        {"B737", "Boeing 737-700",           SIL_NARROWBODY,    false, ""},
        {"B738", "Boeing 737-800",           SIL_NARROWBODY,    false, ""},
        {"B739", "Boeing 737-900",           SIL_NARROWBODY,    false, ""},
        {"B37M", "Boeing 737 MAX 7",         SIL_NARROWBODY,    false, ""},
        {"B38M", "Boeing 737 MAX 8",         SIL_NARROWBODY,    false, ""},
        {"B39M", "Boeing 737 MAX 9",         SIL_NARROWBODY,    false, ""},
        {"B3XM", "Boeing 737 MAX 10",        SIL_NARROWBODY,    false, ""},
        {"B752", "Boeing 757-200",           SIL_NARROWBODY,    false, ""},
        {"B753", "Boeing 757-300",           SIL_NARROWBODY,    false, ""},

        // Boeing Widebody
        {"B744", "Boeing 747-400",           SIL_WIDEBODY_QUAD, true,  "QUEEN 747"},
        {"B748", "Boeing 747-8",             SIL_WIDEBODY_QUAD, true,  "QUEEN 747-8"},
        {"BLCF", "Boeing 747 Dreamlifter",   SIL_WIDEBODY_QUAD, true,  "DREAMLIFTER"},
        {"B762", "Boeing 767-200",           SIL_WIDEBODY_TWIN, false, ""},
        {"B763", "Boeing 767-300",           SIL_WIDEBODY_TWIN, false, ""},
        {"B764", "Boeing 767-400ER",         SIL_WIDEBODY_TWIN, false, ""},
        {"B772", "Boeing 777-200",           SIL_WIDEBODY_TWIN, false, ""},
        {"B77L", "Boeing 777-200LR",         SIL_WIDEBODY_TWIN, false, ""},
        {"B773", "Boeing 777-300",           SIL_WIDEBODY_TWIN, false, ""},
        {"B77W", "Boeing 777-300ER",         SIL_WIDEBODY_TWIN, false, ""},
        {"B788", "Boeing 787-8 Dreamliner",  SIL_WIDEBODY_TWIN, false, ""},
        {"B789", "Boeing 787-9 Dreamliner",  SIL_WIDEBODY_TWIN, false, ""},
        {"B78X", "Boeing 787-10 Dreamliner", SIL_WIDEBODY_TWIN, false, ""},
        {"MD11", "McDonnell Douglas MD-11",  SIL_WIDEBODY_TWIN, true,  "TRIJET MD-11"},

        // Regional Jets & Turboprops
        {"E170", "Embraer E170",             SIL_NARROWBODY,    false, ""},
        {"E75L", "Embraer E175",             SIL_NARROWBODY,    false, ""},
        {"E75S", "Embraer E175",             SIL_NARROWBODY,    false, ""},
        {"E190", "Embraer E190",             SIL_NARROWBODY,    false, ""},
        {"E195", "Embraer E195",             SIL_NARROWBODY,    false, ""},
        {"E290", "Embraer E190-E2",          SIL_NARROWBODY,    false, ""},
        {"E295", "Embraer E195-E2",          SIL_NARROWBODY,    false, ""},
        {"E135", "Embraer ERJ-135",          SIL_BIZJET,        false, ""},
        {"E145", "Embraer ERJ-145",          SIL_BIZJET,        false, ""},
        {"CRJ2", "Bombardier CRJ-200",       SIL_BIZJET,        false, ""},
        {"CRJ7", "Bombardier CRJ-700",       SIL_BIZJET,        false, ""},
        {"CRJ9", "Bombardier CRJ-900",       SIL_BIZJET,        false, ""},
        {"CRJX", "Bombardier CRJ-1000",      SIL_BIZJET,        false, ""},
        {"AT45", "ATR 42-500",               SIL_REGIONAL_PROP, false, ""},
        {"AT46", "ATR 42-600",               SIL_REGIONAL_PROP, false, ""},
        {"AT72", "ATR 72",                   SIL_REGIONAL_PROP, false, ""},
        {"AT75", "ATR 72-500",               SIL_REGIONAL_PROP, false, ""},
        {"AT76", "ATR 72-600",               SIL_REGIONAL_PROP, false, ""},
        {"DH8A", "Dash 8-100",               SIL_REGIONAL_PROP, false, ""},
        {"DH8B", "Dash 8-200",               SIL_REGIONAL_PROP, false, ""},
        {"DH8C", "Dash 8-300",               SIL_REGIONAL_PROP, false, ""},
        {"DH8D", "De Havilland Dash 8 Q400", SIL_REGIONAL_PROP, false, ""},
        {"SF34", "Saab 340",                 SIL_REGIONAL_PROP, false, ""},
        {"SB20", "Saab 2000",                SIL_REGIONAL_PROP, false, ""},

        // Business Jets
        {"C25A", "Cessna Citation CJ2",      SIL_BIZJET,        false, ""},
        {"C25B", "Cessna Citation CJ3",      SIL_BIZJET,        false, ""},
        {"C25C", "Cessna Citation CJ4",      SIL_BIZJET,        false, ""},
        {"C525", "Cessna CitationJet",       SIL_BIZJET,        false, ""},
        {"C56X", "Cessna Citation Excel",    SIL_BIZJET,        false, ""},
        {"C680", "Cessna Citation Sovereign",SIL_BIZJET,        false, ""},
        {"C68A", "Cessna Citation Latitude", SIL_BIZJET,        false, ""},
        {"C700", "Cessna Citation Longitude",SIL_BIZJET,        false, ""},
        {"CL30", "Bombardier Challenger 300",SIL_BIZJET,        false, ""},
        {"CL35", "Bombardier Challenger 350",SIL_BIZJET,        false, ""},
        {"CL60", "Bombardier Challenger 600",SIL_BIZJET,        false, ""},
        {"GLEX", "Bombardier Global Express",SIL_BIZJET,        false, ""},
        {"GL5T", "Bombardier Global 5000",   SIL_BIZJET,        false, ""},
        {"GL6T", "Bombardier Global 6000",   SIL_BIZJET,        false, ""},
        {"GL7T", "Bombardier Global 7500",   SIL_BIZJET,        false, ""},
        {"GLF4", "Gulfstream IV",            SIL_BIZJET,        false, ""},
        {"GLF5", "Gulfstream V / G550",      SIL_BIZJET,        false, ""},
        {"GLF6", "Gulfstream G650",          SIL_BIZJET,        false, ""},
        {"GA5C", "Gulfstream G500",          SIL_BIZJET,        false, ""},
        {"GA6C", "Gulfstream G600",          SIL_BIZJET,        false, ""},
        {"E50P", "Embraer Phenom 100",       SIL_BIZJET,        false, ""},
        {"E55P", "Embraer Phenom 300",       SIL_BIZJET,        false, ""},
        {"E550", "Embraer Legacy 500",       SIL_BIZJET,        false, ""},
        {"F900", "Dassault Falcon 900",      SIL_BIZJET,        false, ""},
        {"F2TH", "Dassault Falcon 2000",     SIL_BIZJET,        false, ""},
        {"FA7X", "Dassault Falcon 7X",       SIL_BIZJET,        false, ""},
        {"FA8X", "Dassault Falcon 8X",       SIL_BIZJET,        false, ""},
        {"PC24", "Pilatus PC-24",            SIL_BIZJET,        false, ""},
        {"LJ45", "Learjet 45",               SIL_BIZJET,        false, ""},
        {"LJ75", "Learjet 75",               SIL_BIZJET,        false, ""},

        // Light GA & Turboprop Singles
        {"PC12", "Pilatus PC-12",            SIL_GA_LIGHT,      false, ""},
        {"TBM9", "Daher TBM 900",            SIL_GA_LIGHT,      false, ""},
        {"C152", "Cessna 152",               SIL_GA_LIGHT,      false, ""},
        {"C172", "Cessna 172 Skyhawk",       SIL_GA_LIGHT,      false, ""},
        {"C182", "Cessna 182 Skylane",       SIL_GA_LIGHT,      false, ""},
        {"C206", "Cessna 206 Stationair",    SIL_GA_LIGHT,      false, ""},
        {"C208", "Cessna 208 Caravan",       SIL_GA_LIGHT,      false, ""},
        {"SR20", "Cirrus SR20",              SIL_GA_LIGHT,      false, ""},
        {"SR22", "Cirrus SR22",              SIL_GA_LIGHT,      false, ""},
        {"SF50", "Cirrus Vision SF50",       SIL_BIZJET,        true,  "VISION JET"},
        {"PA28", "Piper PA-28 Cherokee",     SIL_GA_LIGHT,      false, ""},
        {"PA34", "Piper PA-34 Seneca",       SIL_GA_LIGHT,      false, ""},
        {"PA46", "Piper M600 / Malibu",      SIL_GA_LIGHT,      false, ""},
        {"DA40", "Diamond DA40",             SIL_GA_LIGHT,      false, ""},
        {"DA42", "Diamond DA42 Twin Star",   SIL_GA_LIGHT,      false, ""},
        {"DA62", "Diamond DA62",             SIL_GA_LIGHT,      false, ""},
        {"BE20", "Beechcraft King Air 200",  SIL_REGIONAL_PROP, false, ""},
        {"B350", "Beechcraft King Air 350",  SIL_REGIONAL_PROP, false, ""},
        {"BE36", "Beechcraft Bonanza",       SIL_GA_LIGHT,      false, ""},
        {"BE58", "Beechcraft Baron 58",      SIL_GA_LIGHT,      false, ""},

        // Helicopters
        {"EC35", "Airbus H135 Helicopter",   SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"EC45", "Airbus H145 Helicopter",   SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"AS50", "Airbus H125 Ecureuil",     SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"EC55", "Airbus EC155 Dauphin",     SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"AS32", "Airbus Super Puma",        SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"A109", "Leonardo AW109",           SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"A139", "Leonardo AW139",           SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"A189", "Leonardo AW189",           SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"S76",  "Sikorsky S-76",            SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"S92",  "Sikorsky S-92",            SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"H60",  "Sikorsky UH-60 Black Hawk",SIL_HELICOPTER,    true,  "BLACK HAWK"},
        {"R22",  "Robinson R22",             SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"R44",  "Robinson R44",             SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"R66",  "Robinson R66",             SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"B06",  "Bell 206 JetRanger",       SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"B407", "Bell 407",                 SIL_HELICOPTER,    true,  "HELICOPTER"},
        {"B429", "Bell 429",                 SIL_HELICOPTER,    true,  "HELICOPTER"},

        // Military & Special
        {"A400", "Airbus A400M Atlas",       SIL_MILITARY,      true,  "MILITARY"},
        {"A332", "Airbus A330 Voyager",      SIL_WIDEBODY_TWIN, false, ""},
        {"C17",  "Boeing C-17 Globemaster",  SIL_MILITARY,      true,  "MILITARY"},
        {"C130", "Lockheed C-130 Hercules",  SIL_MILITARY,      true,  "HERCULES"},
        {"C30J", "Lockheed C-130J Hercules", SIL_MILITARY,      true,  "HERCULES"},
        {"K35R", "Boeing KC-135 Stratotanker",SIL_MILITARY,     true,  "TANKER"},
        {"KC46", "Boeing KC-46 Pegasus",     SIL_MILITARY,      true,  "TANKER"},
        {"P8",   "Boeing P-8 Poseidon",      SIL_MILITARY,      true,  "POSEIDON"},
        {"E3CF", "Boeing E-3 Sentry AWACS",  SIL_MILITARY,      true,  "AWACS"},
        {"EUFI", "Eurofighter Typhoon",      SIL_MILITARY,      true,  "FIGHTER"},
        {"F16",  "General Dynamics F-16",    SIL_MILITARY,      true,  "FIGHTER"},
        {"F15",  "Boeing F-15 Eagle",        SIL_MILITARY,      true,  "FIGHTER"},
        {"F35",  "Lockheed F-35 Lightning",  SIL_MILITARY,      true,  "FIGHTER"},
        {"H47",  "Boeing CH-47 Chinook",     SIL_HELICOPTER,    true,  "CHINOOK"},
        {"TEX2", "Beechcraft T-6 Texan II",  SIL_MILITARY,      true,  "TRAINER"},
        {"HAWK", "BAE Hawk",                 SIL_MILITARY,      true,  "MILITARY"}
    };

    inline AircraftTypeInfo LookupAircraftType(const String& typeCode, const String& typeDesc = "", const String& emitterCat = "")
    {
        AircraftTypeInfo info;
        String code = typeCode;
        code.trim();
        code.toUpperCase();
        info.code = code;

        const int count = sizeof(AIRCRAFT_TYPES) / sizeof(AIRCRAFT_TYPES[0]);
        for (int i = 0; i < count; ++i) {
            if (code == AIRCRAFT_TYPES[i].code) {
                info.fullName = AIRCRAFT_TYPES[i].fullName;
                info.category = AIRCRAFT_TYPES[i].category;
                info.isRareCatch = AIRCRAFT_TYPES[i].isRare;
                info.rareBadge = AIRCRAFT_TYPES[i].rareBadge;
                return info;
            }
        }

        // Prefix heuristics when exact ICAO code is not in table
        if (!typeDesc.isEmpty()) {
            info.fullName = typeDesc;
        } else if (code.startsWith("A38")) {
            info.fullName = "Airbus A380";
            info.category = SIL_WIDEBODY_QUAD;
            info.isRareCatch = true;
            info.rareBadge = "SUPERJUMBO";
        } else if (code.startsWith("B74")) {
            info.fullName = "Boeing 747";
            info.category = SIL_WIDEBODY_QUAD;
            info.isRareCatch = true;
            info.rareBadge = "QUEEN 747";
        } else if (code.startsWith("A35") || code.startsWith("A33") || code.startsWith("B77") || code.startsWith("B78") || code.startsWith("B76")) {
            info.fullName = "Widebody Jet (" + code + ")";
            info.category = SIL_WIDEBODY_TWIN;
        } else if (code.startsWith("A31") || code.startsWith("A32") || code.startsWith("A2") || code.startsWith("B73") || code.startsWith("B3") || code.startsWith("B75")) {
            info.fullName = "Airliner (" + code + ")";
            info.category = SIL_NARROWBODY;
        } else if (code.startsWith("E1") || code.startsWith("E2") || code.startsWith("CRJ")) {
            info.fullName = "Regional Jet (" + code + ")";
            info.category = SIL_NARROWBODY;
        } else if (code.startsWith("AT") || code.startsWith("DH8") || code.startsWith("BE2") || code.startsWith("B35")) {
            info.fullName = "Turboprop (" + code + ")";
            info.category = SIL_REGIONAL_PROP;
        } else if (code.startsWith("GLF") || code.startsWith("CL") || code.startsWith("C25") || code.startsWith("C5") || code.startsWith("C6") || code.startsWith("FA") || code.startsWith("LJ")) {
            info.fullName = "Business Jet (" + code + ")";
            info.category = SIL_BIZJET;
        } else if (code.startsWith("EC") || code.startsWith("AS") || code.startsWith("A1") || code.startsWith("R4") || code.startsWith("R6") || code.startsWith("S7") || code.startsWith("S9") || code.startsWith("H6")) {
            info.fullName = "Helicopter (" + code + ")";
            info.category = SIL_HELICOPTER;
            info.isRareCatch = true;
            info.rareBadge = "HELICOPTER";
        } else if (code.startsWith("C1") || code.startsWith("C2") || code.startsWith("PA") || code.startsWith("SR") || code.startsWith("DA")) {
            info.fullName = "Light Aircraft (" + code + ")";
            info.category = SIL_GA_LIGHT;
        } else if (!code.isEmpty()) {
            info.fullName = "Aircraft " + code;
        } else {
            info.fullName = "Commercial Aircraft";
        }

        // Emitter category override if available (A7 = Rotorcraft, A1 = Light, A5 = Heavy)
        if (emitterCat == "A7") {
            info.category = SIL_HELICOPTER;
            info.isRareCatch = true;
            info.rareBadge = "ROTORCRAFT";
        } else if (emitterCat == "A5" && info.category == SIL_NARROWBODY) {
            info.category = SIL_WIDEBODY_TWIN;
        } else if (emitterCat == "A1" && code.isEmpty()) {
            info.category = SIL_GA_LIGHT;
            info.fullName = "Light Aircraft";
        }

        return info;
    }

    // Haversine Great-Circle distance in kilometres
    inline float ComputeDistanceKm(double lat1, double lon1, double lat2, double lon2)
    {
        if (lat1 == 0.0 && lon1 == 0.0) return 0.0f;
        if (lat2 == 0.0 && lon2 == 0.0) return 0.0f;
        constexpr double R = 6371.0;
        double dLat = radians(lat2 - lat1);
        double dLon = radians(lon2 - lon1);
        double a = std::sin(dLat * 0.5) * std::sin(dLat * 0.5) +
                   std::cos(radians(lat1)) * std::cos(radians(lat2)) *
                   std::sin(dLon * 0.5) * std::sin(dLon * 0.5);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        return static_cast<float>(R * c);
    }

    // Initial compass bearing in degrees [0..360) from (lat1,lon1) to (lat2,lon2)
    inline float ComputeBearingDeg(double lat1, double lon1, double lat2, double lon2)
    {
        double phi1 = radians(lat1);
        double phi2 = radians(lat2);
        double dLon = radians(lon2 - lon1);
        double y = std::sin(dLon) * std::cos(phi2);
        double x = std::cos(phi1) * std::sin(phi2) -
                   std::sin(phi1) * std::cos(phi2) * std::cos(dLon);
        double brng = degrees(std::atan2(y, x));
        return static_cast<float>(std::fmod(brng + 360.0, 360.0));
    }

    inline const char* BearingToCardinal(float deg)
    {
        static const char* DIRS[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int idx = static_cast<int>(std::floor((deg + 22.5f) / 45.0f)) & 7;
        return DIRS[idx];
    }

    // Returns route progress percentage [0..100] when origin & destination coordinates are known, or -1 if unknown
    inline float ComputeRouteProgressPct(double depLat, double depLon, double arrLat, double arrLon, double curLat, double curLon, float& outTotalKm, float& outRemKm)
    {
        outTotalKm = 0.0f;
        outRemKm = 0.0f;
        if ((depLat == 0.0 && depLon == 0.0) || (arrLat == 0.0 && arrLon == 0.0)) {
            return -1.0f;
        }
        float totalKm = ComputeDistanceKm(depLat, depLon, arrLat, arrLon);
        if (totalKm < 5.0f) return -1.0f;

        float flownKm = ComputeDistanceKm(depLat, depLon, curLat, curLon);
        float remKm = ComputeDistanceKm(curLat, curLon, arrLat, arrLon);
        outTotalKm = totalKm;
        outRemKm = remKm;

        float denom = flownKm + remKm;
        if (denom < 1.0f) return 0.0f;
        float pct = (flownKm / denom) * 100.0f;
        if (pct < 2.0f) pct = 2.0f;
        if (pct > 99.0f) pct = 99.0f;
        return pct;
    }
}

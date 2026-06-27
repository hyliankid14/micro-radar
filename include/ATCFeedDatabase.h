#pragma once

#include <Arduino.h>

struct ATCFeedEntry {
    const char* icao;
    const char* label;
    const char* description;
    const char* mount;
};

#define FEED(ICAO, LABEL, DESC, MOUNT) { ICAO, LABEL, DESC, MOUNT }

// Only feeds with verified mount points are included.
// To add more: verify with `curl -s -o /dev/null -w "%{http_code}" -L http://d.liveatc.net/<mount>.mp3`
static const ATCFeedEntry ATC_FEEDS[] PROGMEM = {
    FEED("KJFK", "JFK Tower 4L/22R", "New York Kennedy Tower Rwy 4L/22R", "kjfk_twr"),
    FEED("KJFK", "JFK Tower Rwy 13L/31R", "New York Kennedy Tower Rwy 13L/31R", "kjfk_twr3"),
    FEED("KJFK", "JFK Ground", "New York Kennedy Ground", "kjfk_gnd"),

    FEED("KLAX", "LAX Tower West", "Los Angeles International Tower West", "klax_twr"),
    FEED("KLAX", "LAX Ground", "Los Angeles International Ground", "klax_gnd"),

    FEED("KATL", "Atlanta Tower", "Hartsfield-Jackson Atlanta Tower", "katl_twr"),

    FEED("KSFO", "SFO Tower", "San Francisco International Tower", "ksfo_twr"),
    FEED("KSFO", "SFO Tower Rwy 10R/28L", "San Francisco Tower Rwy 10R/28L", "ksfo_twr2"),

    FEED("KDCA", "Reagan National", "Ronald Reagan Washington National", "kdca"),
};

#define ATC_FEED_COUNT (sizeof(ATC_FEEDS) / sizeof(ATC_FEEDS[0]))

inline size_t getATCFeedCount() {
    return ATC_FEED_COUNT;
}

inline const ATCFeedEntry& getATCFeed(size_t index) {
    static ATCFeedEntry empty = {"", "", "", ""};
    if (index >= ATC_FEED_COUNT) return empty;
    return ATC_FEEDS[index];
}

inline String getATCStreamURL(size_t index) {
    if (index >= ATC_FEED_COUNT) return "";
    ATCFeedEntry entry;
    memcpy_P(&entry, &ATC_FEEDS[index], sizeof(ATCFeedEntry));
    if (entry.mount[0] == '\0') return "";
    String url = "http://d.liveatc.net/";
    url += entry.mount;
    url += ".mp3";
    return url;
}

inline String getATCStreamURLByMount(const char* mount) {
    if (!mount || mount[0] == '\0') return "";
    String url = "http://d.liveatc.net/";
    url += mount;
    url += ".mp3";
    return url;
}

inline int findATCFeedIndex(const char* icao, const char* description) {
    if (!icao || icao[0] == '\0') return -1;
    for (size_t i = 0; i < ATC_FEED_COUNT; i++) {
        ATCFeedEntry entry;
        memcpy_P(&entry, &ATC_FEEDS[i], sizeof(ATCFeedEntry));
        if (strcmp_P(icao, entry.icao) == 0) {
            if (!description || description[0] == '\0') return (int)i;
            if (strcmp_P(description, entry.description) == 0) return (int)i;
        }
    }
    return -1;
}

inline String getATCFeedLabel(size_t index) {
    if (index >= ATC_FEED_COUNT) return "";
    ATCFeedEntry entry;
    memcpy_P(&entry, &ATC_FEEDS[index], sizeof(ATCFeedEntry));
    String label = entry.icao;
    label += " - ";
    label += entry.label;
    return label;
}

inline String getATCFeedDescription(size_t index) {
    if (index >= ATC_FEED_COUNT) return "";
    ATCFeedEntry entry;
    memcpy_P(&entry, &ATC_FEEDS[index], sizeof(ATCFeedEntry));
    return String(entry.description);
}

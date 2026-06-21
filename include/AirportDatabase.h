#pragma once

#include <Arduino.h>
#include "AirportData.h"

class AirportDatabase {
private:
    bool databaseLoaded = false;

public:
    AirportDatabase() {}

    // Load the static database (just validates it's available)
    bool begin() {
        databaseLoaded = true;
        Serial.printf("[Airports] Static database ready: %d airports loaded\n", getAirportCount());
        return true;
    }

    // Get airport name by IATA code (binary search on sorted array)
    String getAirportName(const String& iataCode) {
        if (!databaseLoaded || iataCode.length() != 3) {
            return "";
        }

        String upper = iataCode;
        upper.toUpperCase();

        int lo = 0;
        int hi = AIRPORTS_COUNT - 1;

        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            const AirportData& entry = AIRPORTS[mid];

            int cmp = upper.compareTo(entry.iata);

            if (cmp == 0) {
                return entry.name;
            } else if (cmp < 0) {
                hi = mid - 1;
            } else {
                lo = mid + 1;
            }
        }

        return "";
    }

    // Get number of airports in database
    size_t getAirportCount() const {
        return AIRPORTS_COUNT;
    }

    // Check if database is loaded
    bool isLoaded() const {
        return databaseLoaded;
    }
};

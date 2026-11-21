#pragma once

#include "Aircraft.h"

struct TrackedAircraft {
    Aircraft state;
    unsigned long lastSeen;

    struct TrailPoint {
        float lat = 0.0f;
        float lon = 0.0f;
    };
    static constexpr int MAX_TRAIL_POINTS = 10;
    TrailPoint trail[MAX_TRAIL_POINTS];
    int trailCount = 0;

    // blending state
    float blendFromLat = 0.0f;
    float blendFromLon = 0.0f;
    float blendAlpha = 1.0f;  // 1.0 = blend complete, no interpolation active

    unsigned long lastTick = 0;

    // first appearance, no blend needed
    TrackedAircraft(const Aircraft& ac, unsigned long now)
        : state(ac), lastSeen(now),
        blendFromLat(ac.latitude),
        blendFromLon(ac.longitude),
        blendAlpha(1.0f) {
        trail[0] = { ac.latitude, ac.longitude };
        trailCount = 1;
    }

    // subsequent update — blend from current visual position
    void Update(const Aircraft& newState, unsigned long now) {
        // capture visual position at moment of update before switching state
        auto [curLat, curLon] = GetDisplayPosition();
        blendFromLat = curLat;
        blendFromLon = curLon;
        blendAlpha = 0.0f;  // restart blend

        // Preserve richer fields if the update came from a basic source
        String prevReg = state.registration;
        String prevType = state.typeCode;
        String prevDesc = state.typeDescription;
        String prevOp = state.operatorName;
        String prevCat = state.emitterCategory;

        state = newState;
        if (state.registration.isEmpty()) state.registration = prevReg;
        if (state.typeCode.isEmpty()) state.typeCode = prevType;
        if (state.typeDescription.isEmpty()) state.typeDescription = prevDesc;
        if (state.operatorName.isEmpty()) state.operatorName = prevOp;
        if (state.emitterCategory.isEmpty()) state.emitterCategory = prevCat;

        // Append breadcrumb if moved enough
        if (trailCount == 0) {
            trail[0] = { newState.latitude, newState.longitude };
            trailCount = 1;
        } else {
            const TrailPoint& lastPt = trail[trailCount - 1];
            float dLat = newState.latitude - lastPt.lat;
            float dLon = newState.longitude - lastPt.lon;
            if ((dLat * dLat + dLon * dLon) > 0.000004f) { // ~200m
                if (trailCount < MAX_TRAIL_POINTS) {
                    trail[trailCount++] = { newState.latitude, newState.longitude };
                } else {
                    for (int i = 1; i < MAX_TRAIL_POINTS; ++i) {
                        trail[i - 1] = trail[i];
                    }
                    trail[MAX_TRAIL_POINTS - 1] = { newState.latitude, newState.longitude };
                }
            }
        }

        lastSeen = now;
    }

    void Tick() {
        unsigned long now = millis();
        float deltaSeconds = (now - lastTick) / 1000.0f;
        lastTick = now;

        const float blendSpeed = 0.15f; // lower = slower, higher = faster
        blendAlpha = min(blendAlpha + deltaSeconds * blendSpeed, 1.0f);
    }

    std::pair<float, float> GetDisplayPosition() const {
        auto [deadLat, deadLon] = PredictPosition();

        if (blendAlpha >= 1.0f)
            return { deadLat, deadLon };

        // ease-in-out for smoother feel
        float t = blendAlpha * blendAlpha * (3.0f - 2.0f * blendAlpha);

        return {
            blendFromLat + t * (deadLat - blendFromLat),
            blendFromLon + t * (deadLon - blendFromLon)
        };
    }

    std::pair<float, float> PredictPosition() const {
        float dataAgeOnArrival = 0.0f;
        if (state.timePosition > 0 && state.lastContact > 0)
            dataAgeOnArrival = (float)(state.lastContact - state.timePosition);

        float localElapsed = (millis() - lastSeen) / 1000.0f;
        float dt = localElapsed + dataAgeOnArrival;

        float headingRad = radians(state.trueTrack);
        const float latMetersPerDeg = 111320.0f;
        float deltaLat = (state.velocity * dt * cos(headingRad)) / latMetersPerDeg;
        float deltaLon = (state.velocity * dt * sin(headingRad)) / (latMetersPerDeg * cos(radians(state.latitude)));

        return { state.latitude + deltaLat, state.longitude + deltaLon };
    }
};
#include "ATCAudioManager.h"
#include "ATCFeedDatabase.h"

void audio_info(const char* info) {
    Serial.printf("[ATC-INFO] %s\n", info);
}

void audio_id3data(const char* info) {
    Serial.printf("[ATC-ID3] %s\n", info);
}

void audio_showstation(const char* info) {
    Serial.printf("[ATC-STATION] %s\n", info);
}

void audio_showstreamtitle(const char* info) {
    Serial.printf("[ATC-STREAM] %s\n", info);
}

ATCAudioManager::ATCAudioManager(ES8311Config& codec, int paPin)
    : codec(codec), paPin(paPin), volume(DEFAULT_VOLUME), playing(false), enabled(false) {}

void ATCAudioManager::init() {
    Serial.printf("[ATC] I2S pins: BCLK=GPIO9, LRC=GPIO10, DOUT=GPIO12, MCLK=GPIO8\n");
    audio.setPinout(9, 10, 12, 8);
    audio.setVolume(volume);
    audio.setConnectionTimeout(8000, 20000);

    Serial.println("[ATC] Audio manager initialised");
}

bool ATCAudioManager::playStream(const char* url) {
    if (!url || url[0] == '\0') {
        Serial.println("[ATC] Empty stream URL");
        return false;
    }

    if (playing) {
        audio.stopSong();
        delay(100);
    }

    if (paPin >= 0) {
        codec.enablePA(paPin);
    }

    currentURL = url;
    Serial.printf("[ATC] Connecting to stream: %s\n", url);

    bool connected = audio.connecttohost(url);
    Serial.printf("[ATC] connecttohost returned: %s\n", connected ? "true" : "false");

    if (connected) {
        playing = true;
        enabled = true;
        Serial.printf("[ATC] Stream playback initiated: %s\n",
                      currentName.isEmpty() ? currentURL.c_str() : currentName.c_str());
    } else {
        Serial.println("[ATC] Failed to initiate stream playback");
        playing = false;
        enabled = false;
        if (paPin >= 0) {
            codec.disablePA(paPin);
        }
    }
    return connected;
}

bool ATCAudioManager::playStreamByIndex(size_t feedIndex) {
    if (feedIndex >= ATC_FEED_COUNT) {
        Serial.printf("[ATC] Invalid feed index: %zu\n", feedIndex);
        return false;
    }

    ATCFeedEntry entry;
    memcpy_P(&entry, &ATC_FEEDS[feedIndex], sizeof(ATCFeedEntry));

    currentName = String(entry.icao) + " - " + String(entry.label);

    String url = getATCStreamURL(feedIndex);
    if (url.isEmpty()) {
        Serial.println("[ATC] Empty stream URL for feed index");
        return false;
    }

    Serial.printf("[ATC] Playing feed: %s (mount: %s, url: %s)\n",
                  currentName.c_str(), entry.mount, url.c_str());
    return playStream(url.c_str());
}

void ATCAudioManager::stop() {
    if (playing) {
        audio.stopSong();
        playing = false;
        enabled = false;
        Serial.println("[ATC] Audio stopped");
    }
    if (paPin >= 0) {
        codec.disablePA(paPin);
    }
}

void ATCAudioManager::setVolume(uint8_t percent) {
    volume = percent > 100 ? 100 : percent;

    uint8_t audioVol = map(volume, 0, 100, 0, 21);
    audio.setVolume(audioVol);

    if (codec.isReady()) {
        codec.setVolume(volume);
    }

    Serial.printf("[ATC] Volume set to %d%% (audio=%d/21)\n", volume, audioVol);
}

void ATCAudioManager::update() {
    static unsigned long lastLoopLog = 0;
    static unsigned long loopCount = 0;
    unsigned long now = millis();
    
    if (playing) {
        audio.loop();
        loopCount++;
        
        if (now - lastLoopLog > 5000) {
            Serial.printf("[ATC] audio.loop() called %lu times in last 5s\n", loopCount);
            loopCount = 0;
            lastLoopLog = now;
        }
    }
}

bool ATCAudioManager::playTestStream() {
    if (playing) {
        audio.stopSong();
        delay(100);
    }
    
    if (paPin >= 0) {
        codec.enablePA(paPin);
        Serial.println("[ATC] PA enabled - should hear pop from speaker");
    }
    
    currentURL = "http://ice1.somafm.com/groovesalad-128-mp3";
    currentName = "Test Stream (SomaFM)";
    
    Serial.println("[ATC] ========== TEST STREAM DIAGNOSTIC ==========");
    Serial.printf("[ATC] Test stream URL: %s\n", currentURL.c_str());
    Serial.printf("[ATC] WiFi status: %s\n", WiFi.isConnected() ? "CONNECTED" : "DISCONNECTED");
    Serial.printf("[ATC] Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    Serial.printf("[ATC] PSRAM free: %lu bytes\n", (unsigned long)ESP.getFreePsram());
    
    playing = true;
    enabled = true;
    
    Serial.println("[ATC] Calling audio.connecttohost()...");
    bool connected = audio.connecttohost(currentURL.c_str());
    Serial.printf("[ATC] connecttohost returned: %s\n", connected ? "TRUE (success)" : "FALSE (failed)");
    
    if (!connected) {
        Serial.println("[ATC] Stream connection FAILED - check WiFi/internet");
        playing = false;
        enabled = false;
        if (paPin >= 0) {
            codec.disablePA(paPin);
        }
    } else {
        Serial.println("[ATC] Stream connection initiated - audio should start within 5-10 seconds");
        Serial.println("[ATC] Watch for [ATC-INFO] messages below");
    }
    
    return connected;
}

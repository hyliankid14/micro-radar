#pragma once

#include <Arduino.h>
#include <Audio.h>
#include "ES8311Config.h"

class ATCAudioManager {
public:
    ATCAudioManager(ES8311Config& codec, int paPin = -1);

    void init();
    bool playStream(const char* url);
    bool playStreamByIndex(size_t feedIndex);
    bool playTestStream();
    void stop();
    void setVolume(uint8_t percent);
    uint8_t getVolume() const { return volume; }
    bool isPlaying() const { return playing; }
    const String& getCurrentURL() const { return currentURL; }
    const String& getCurrentName() const { return currentName; }
    void setFeedName(const String& name) { currentName = name; }
    void update();

private:
    Audio audio;
    ES8311Config& codec;
    int paPin;
    uint8_t volume;
    bool playing;
    bool enabled;
    String currentURL;
    String currentName;

    static constexpr uint8_t DEFAULT_VOLUME = 40;
};

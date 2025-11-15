#pragma once

#include <Arduino.h>

// Lightweight bridge that mirrors Ableton/NeoTrellis state to a USB Serial GUI.
// The on-board firmware compiles even if no desktop GUI is connected; the class
// simply prints concise messages that higher level tools can parse.
class GUIInterface {
public:
    void begin();
    void update();

    void sendGridColors7bit(const uint8_t* data, int length);
    void sendGridColors14bit(const uint8_t* data, int length);
    void sendPadColor14bit(int padIndex,
                           uint8_t rMsb, uint8_t rLsb,
                           uint8_t gMsb, uint8_t gLsb,
                           uint8_t bMsb, uint8_t bLsb);
    void sendClipName(uint8_t track, uint8_t scene, const char* name);
    void sendTrackName(uint8_t track, const char* name);
    void sendBPM(float bpm);
    void sendTransportState(bool isPlaying, bool isRecording);

private:
    void sendTag(const char* tag);
    void printHexPreview(const uint8_t* data, int length, int maxBytes = 16);

    unsigned long lastHeartbeatMs = 0;
};


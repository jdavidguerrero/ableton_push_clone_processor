#include "GUIInterface/GUIInterface.h"

namespace {
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
}

void GUIInterface::begin() {
    lastHeartbeatMs = millis();
    sendTag("GUI");
    Serial.println("interface ready");
}

void GUIInterface::update() {
    unsigned long now = millis();
    if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatMs = now;
        sendTag("GUI");
        Serial.println("alive");
    }
}

void GUIInterface::sendGridColors7bit(const uint8_t* data, int length) {
    if (!data || length <= 0) {
        return;
    }
    sendTag("GRID7");
    Serial.print("bytes=");
    Serial.print(length);
    Serial.print(" preview=");
    printHexPreview(data, length);
    Serial.println();
}

void GUIInterface::sendGridColors14bit(const uint8_t* data, int length) {
    if (!data || length <= 0) {
        return;
    }
    sendTag("GRID14");
    Serial.print("bytes=");
    Serial.print(length);
    Serial.print(" preview=");
    printHexPreview(data, length);
    Serial.println();
}

void GUIInterface::sendPadColor14bit(int padIndex,
                                     uint8_t rMsb, uint8_t rLsb,
                                     uint8_t gMsb, uint8_t gLsb,
                                     uint8_t bMsb, uint8_t bLsb) {
    sendTag("PAD");
    Serial.print(padIndex);
    Serial.print(" -> R(");
    Serial.print(rMsb, HEX);
    Serial.print(",");
    Serial.print(rLsb, HEX);
    Serial.print(") G(");
    Serial.print(gMsb, HEX);
    Serial.print(",");
    Serial.print(gLsb, HEX);
    Serial.print(") B(");
    Serial.print(bMsb, HEX);
    Serial.print(",");
    Serial.print(bLsb, HEX);
    Serial.println(")");
}

void GUIInterface::sendClipName(uint8_t track, uint8_t scene, const char* name) {
    sendTag("CLIP");
    Serial.print("T");
    Serial.print(track);
    Serial.print(" S");
    Serial.print(scene);
    Serial.print(" name=");
    Serial.println(name ? name : "");
}

void GUIInterface::sendTrackName(uint8_t track, const char* name) {
    sendTag("TRACK");
    Serial.print(track);
    Serial.print(" name=");
    Serial.println(name ? name : "");
}

void GUIInterface::sendBPM(float bpm) {
    sendTag("BPM");
    Serial.println(bpm, 1);
}

void GUIInterface::sendTransportState(bool isPlaying, bool isRecording) {
    sendTag("TRANSPORT");
    Serial.print("play=");
    Serial.print(isPlaying ? "1" : "0");
    Serial.print(" rec=");
    Serial.println(isRecording ? "1" : "0");
}

void GUIInterface::sendTag(const char* tag) {
    Serial.print("[");
    Serial.print(tag);
    Serial.print("] ");
}

void GUIInterface::printHexPreview(const uint8_t* data, int length, int maxBytes) {
    if (!data || length <= 0) {
        return;
    }
    int limit = length;
    if (maxBytes > 0 && maxBytes < limit) {
        limit = maxBytes;
    }
    for (int i = 0; i < limit; ++i) {
        if (i > 0) {
            Serial.print(" ");
        }
        Serial.print(data[i], HEX);
    }
    if (limit < length) {
        Serial.print(" ...");
    }
}


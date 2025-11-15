#pragma once

#include <Arduino.h>

class NeoTrellisLink {
public:
    void sendCommand(uint8_t command, const uint8_t* data, int dataLength);
    void sendRaw(const uint8_t* data, int length);
    void setPixelColor(int key, uint32_t color);
    void triggerConnectionAnimation();
    void runConnectionSweep();

    bool initializeCommunication();
    void update();

    void setConnected(bool connected);
    bool isConnected() const { return m4Connected; }

    void handleHandshakeAck();
    void handlePingResponse();

private:
    void requestHandshake();

    bool m4Connected = false;
    bool handshakePending = false;
    unsigned long lastHandshakeRequestMs = 0;
    unsigned long lastReconnectAttemptMs = 0;
    unsigned long lastPingSentMs = 0;
    unsigned long lastPongMs = 0;
    uint8_t handshakeAttempts = 0;
};

#include "GUIInterface/GUIInterface.h"
#include "../UIBridge.h"
#include <math.h>
#include <cstring>

namespace {
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
constexpr unsigned long GUI_PING_INTERVAL_MS = 3000;
constexpr unsigned long GUI_TIMEOUT_MS = GUI_PING_INTERVAL_MS * 3;
constexpr unsigned long GUI_HANDSHAKE_RETRY_MS = 2000;
const uint8_t GUI_HANDSHAKE_PAYLOAD[] = {
    'P','U','S','H','C','L','O','N','E','_','G','U','I'
};
}

extern UIBridge uiBridge;

void GUIInterface::begin(Stream& serialPort) {
    io = &serialPort;
    lastHeartbeatMs = millis();
    rxIndex = 0;
    expectedLength = 0;
    lastPingMs = 0;
    lastPongMs = 0;
    guiConnected = false;
    handshakePending = false;
    disconnectNotified = false;
    everConnected = false;
    if (io) {
        sendDisconnectEvent();
        sendHandshake();
    }
}

void GUIInterface::update() {
    if (!io) return;
    unsigned long now = millis();
    processIncoming();
    now = millis();

    if (!guiConnected) {
        if (!handshakePending || (now - lastPingMs) > GUI_HANDSHAKE_RETRY_MS) {
            sendHandshake();
        }
    } else {
        if (now - lastPingMs > GUI_PING_INTERVAL_MS) {
            sendBinary(CMD_PING, nullptr, 0);
            lastPingMs = now;
        }
        if (lastPongMs != 0 && (now - lastPongMs) > GUI_TIMEOUT_MS) {
            Serial.println("GUIInterface: Ping timeout tras 3 intentos sin respuesta");
            sendDisconnectEvent();
            guiConnected = false;
            handshakePending = false;
            lastPongMs = 0;
        }
    }

    if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatMs = now;
    }
}

void GUIInterface::sendGridColors7bit(const uint8_t* data, int length) {
    if (!io || !data || length <= 0 || length > 255) {
        return;
    }
    sendBinary(CMD_LED_GRID_UPDATE, data, static_cast<uint8_t>(length));
}

void GUIInterface::sendGridColors14bit(const uint8_t* data, int length) {
    if (!io || !data || length <= 0 || length > 255) {
        return;
    }
    sendBinary(CMD_LED_GRID_UPDATE_14, data, static_cast<uint8_t>(length));
}

void GUIInterface::sendPadColor14bit(int padIndex,
                                     uint8_t rMsb, uint8_t rLsb,
                                     uint8_t gMsb, uint8_t gLsb,
                                     uint8_t bMsb, uint8_t bLsb) {
    if (!io) return;
    uint8_t payload[] = {
        static_cast<uint8_t>(padIndex & 0x7F),
        rMsb, rLsb,
        gMsb, gLsb,
        bMsb, bLsb
    };
    sendBinary(CMD_LED_PAD_UPDATE_14, payload, sizeof(payload));
}

void GUIInterface::sendClipName(uint8_t track, uint8_t scene, const char* name) {
    if (!io || !name) return;
    size_t len = strnlen(name, 240);
    if (len > 240) len = 240;
    uint8_t payload[242];
    payload[0] = track & 0x7F;
    payload[1] = scene & 0x7F;
    memcpy(&payload[2], name, len);
    sendBinary(CMD_CLIP_NAME, payload, static_cast<uint8_t>(len + 2));
}

void GUIInterface::sendTrackName(uint8_t track, const char* name) {
    if (!io || !name) return;
    size_t len = strnlen(name, 250);
    if (len > 250) len = 250;
    uint8_t payload[251];
    payload[0] = track & 0x7F;
    memcpy(&payload[1], name, len);
    sendBinary(CMD_TRACK_NAME, payload, static_cast<uint8_t>(len + 1));
}

void GUIInterface::sendBPM(float bpm) {
    if (!io) return;
    uint16_t fixed = static_cast<uint16_t>(roundf(bpm * 10.0f));
    uint8_t payload[] = {
        static_cast<uint8_t>((fixed >> 7) & 0x7F),
        static_cast<uint8_t>(fixed & 0x7F)
    };
    sendBinary(CMD_TRANSPORT_TEMPO, payload, sizeof(payload));
}

void GUIInterface::sendTransportState(bool isPlaying, bool isRecording) {
    if (!io) return;
    uint8_t play = isPlaying ? 1 : 0;
    uint8_t rec = isRecording ? 1 : 0;
    sendBinary(CMD_TRANSPORT_PLAY, &play, 1);
    sendBinary(CMD_TRANSPORT_RECORD, &rec, 1);
}

void GUIInterface::sendTag(const char* tag) {}

void GUIInterface::printHexPreview(const uint8_t* data, int length, int maxBytes) {}

static const char* commandName(uint8_t cmd) {
    switch (cmd) {
        case CMD_HANDSHAKE: return "HANDSHAKE";
        case CMD_HANDSHAKE_REPLY: return "HANDSHAKE_REPLY";
        case CMD_PING: return "PING";
        case CMD_LED_GRID_UPDATE: return "LED_GRID_UPDATE";
        case CMD_LED_GRID_UPDATE_14: return "LED_GRID_UPDATE_14";
        case CMD_LED_PAD_UPDATE_14: return "LED_PAD_UPDATE_14";
        case CMD_CLIP_NAME: return "CLIP_NAME";
        case CMD_TRACK_NAME: return "TRACK_NAME";
        case CMD_TRANSPORT_TEMPO: return "TRANSPORT_TEMPO";
        case CMD_TRANSPORT_PLAY: return "TRANSPORT_PLAY";
        case CMD_TRANSPORT_RECORD: return "TRANSPORT_RECORD";
        case CMD_DISCONNECT: return "DISCONNECT";
        default: return "UNKNOWN";
    }
}

void GUIInterface::sendBinary(uint8_t cmd, const uint8_t* payload, uint8_t len) {
    if (!io) return;
    uint8_t buffer[260];
    uint16_t written = BinaryProtocol::buildMessage(cmd, payload, len, buffer, sizeof(buffer));
    if (written > 0) {
        io->write(buffer, written);
        io->flush();
        if (cmd != CMD_PING) {
            Serial.print("GUI TX ");
            Serial.print(commandName(cmd));
            Serial.print(" (0x");
            Serial.print(cmd, HEX);
            Serial.print(") len=");
            Serial.print(len);
            Serial.print(" frame: ");
            for (uint16_t i = 0; i < written; ++i) {
                if (i) Serial.print(' ');
                Serial.print(buffer[i], HEX);
            }
            Serial.println();
        }
    }
}

void GUIInterface::sendHandshake() {
    if (!io || handshakePending) return;
    sendBinary(CMD_HANDSHAKE, GUI_HANDSHAKE_PAYLOAD, sizeof(GUI_HANDSHAKE_PAYLOAD));
    handshakePending = true;
    lastPingMs = millis();
}

void GUIInterface::sendDisconnectEvent() {
    if (!io || disconnectNotified || !everConnected) return;
    sendBinary(CMD_DISCONNECT, nullptr, 0);
    disconnectNotified = true;
}

void GUIInterface::processIncoming() {
    if (!io) return;
    while (io->available()) {
        uint8_t byte = static_cast<uint8_t>(io->read());
        if (byte == BinaryProtocol::BINARY_SYNC_BYTE) {
            rxIndex = 0;
            expectedLength = 0;
            rxBuffer[rxIndex++] = byte;
            continue;
        }

        if (rxIndex == 0) {
            continue;
        }

        if (rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = byte;
        } else {
            rxIndex = 0;
            expectedLength = 0;
            continue;
        }

        if (rxIndex >= 3 && expectedLength == 0) {
            uint8_t payloadLen = rxBuffer[2];
            expectedLength = BinaryProtocol::getMessageSize(payloadLen);
            if (expectedLength > sizeof(rxBuffer)) {
                rxIndex = 0;
                expectedLength = 0;
                continue;
            }
        }

        if (expectedLength != 0 && rxIndex >= expectedLength) {
            uint8_t cmd = 0;
            const uint8_t* payload = nullptr;
            uint8_t payloadLen = 0;
            bool valid = BinaryProtocol::parseMessage(rxBuffer, expectedLength, cmd, payload, payloadLen);
            if (valid) {
                if (cmd != CMD_PING) {
                    Serial.print("GUI RX ");
                    Serial.print(commandName(cmd));
                    Serial.print(" (0x");
                    Serial.print(cmd, HEX);
                    Serial.print(") len=");
                    Serial.print(payloadLen);
                    Serial.print(" frame: ");
                    for (uint16_t i = 0; i < expectedLength; ++i) {
                        if (i) Serial.print(' ');
                        Serial.print(rxBuffer[i], HEX);
                    }
                    Serial.println();
                }
                handleIncomingCommand(cmd, const_cast<uint8_t*>(payload), payloadLen);
            }
            rxIndex = 0;
            expectedLength = 0;
        }
    }
}

void GUIInterface::handleIncomingCommand(uint8_t cmd, uint8_t* payload, uint8_t len) {
    switch (cmd) {
        case CMD_HANDSHAKE:
        case CMD_HANDSHAKE_REPLY:
            guiConnected = true;
            handshakePending = false;
            lastPongMs = millis();
            if (cmd == CMD_HANDSHAKE_REPLY) {
                Serial.println("GUIInterface: Handshake ACK from GUI");
            }
            disconnectNotified = false;
            everConnected = true;
            break;
        case CMD_PING:
            guiConnected = true;
            lastPongMs = millis();
            break;
        case CMD_DISCONNECT:
            guiConnected = false;
            handshakePending = false;
            lastPongMs = 0;
            disconnectNotified = true;
            break;
        default:
            uiBridge.handleUARTCommand(cmd, payload, len);
            break;
    }
}

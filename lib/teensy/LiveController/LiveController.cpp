#include "LiveController/LiveController.h"
#include "MidiCommands.h"
#include "shared/Config.h"
#include "LiveControllerStates.h"
#include "UIPanelCommands.h"
#include "../UIBridge.h"
#include "../NeoTrellisLink/NeoTrellisLink.h"
#include "../GUIInterface/GUIInterface.h"
#include <cstring>
#include <usb_midi.h>

namespace {
bool parseLiveSysExFrame(const uint8_t* data,
                         uint16_t length,
                         uint8_t& command,
                         uint8_t& sequence,
                         uint16_t& payloadLen,
                         const uint8_t*& payload) {
    if (!data || length < 10) {
        return false;
    }
    if (data[0] != SYSEX_START || data[1] != MANUFACTURER_ID || data[2] != DEVICE_ID || data[3] != 0x7F) {
        return false;
    }
    if (data[length - 1] != SYSEX_END) {
        return false;
    }
    command = data[4] & 0x7F;
    sequence = data[5] & 0x7F;
    payloadLen = (uint16_t)(((data[6] & 0x7F) << 7) | (data[7] & 0x7F));
    const uint16_t payloadStart = 8;
    const uint16_t expectedLength = payloadStart + payloadLen + 2; // checksum + F7
    if (length != expectedLength) {
        return false;
    }
    payload = &data[payloadStart];
    const uint8_t checksum = data[payloadStart + payloadLen] & 0x7F;
    uint8_t computed = command ^ sequence;
    for (uint16_t i = 0; i < payloadLen; ++i) {
        computed ^= payload[i];
    }
    return ((computed & 0x7F) == checksum);
}
}

// Constructor
LiveController::LiveController() {
    // This is now empty as there are no I2C objects to initialize.
}

// Destructor - nothing to free (global lifetime on MCU)
LiveController::~LiveController() {}

// Global reference to the main LiveController instance (defined in Teensy Hardware.cpp)
extern LiveController liveController;
extern UIBridge uiBridge;
extern NeoTrellisLink neoTrellisLink;
extern class GUIInterface guiInterface;

// The I2C-based key callback has been removed.
// Key events are now handled by the M4 board and sent via UART.

void LiveController::begin() {
    // This function is now a placeholder.
    // The actual hardware (M4) is initialized via UART.
    // The I2C-based NeoTrellis initialization has been removed
    // to match the UART-based architecture.
    Serial.println("LiveController class initialized (UART mode).");
}

void LiveController::read() {
    // Watchdog: if Live connected but no grid yet, request it after a timeout
    if (liveConnected && !gridSeen) {
        unsigned long now = millis();
        if (liveConnectedAt == 0) liveConnectedAt = now;
        if ((now - liveConnectedAt) > 2000 && (now - gridRequestLastAttempt) > 2000 && gridRequestRetries < 3) {
            Serial.println("Teensy: No grid received yet — send a manual 'grid' command via Serial");
            gridRequestLastAttempt = now;
            gridRequestRetries++;
        }
    }
}

void LiveController::processMIDI() {
    // Limit MIDI processing to avoid blocking UART
    const int MAX_MIDI_PER_LOOP = 5;
    int processedCount = 0;

    while (usbMIDI.read() && processedCount < MAX_MIDI_PER_LOOP) {
        if (usbMIDI.getType() != usbMIDI.SystemExclusive) {
            continue;
        }
        processedCount++;

        uint8_t* sysexData = usbMIDI.getSysExArray();
        uint16_t length = usbMIDI.getSysExArrayLength();
        if (!sysexData || length == 0) {
            continue;
        }

        #ifdef DEBUG_LIVE_LOG
        Serial.print("Live command (");
        Serial.print(length);
        Serial.print(" bytes): ");
        for (int i = 0; i < min(length, 10); i++) {
            Serial.print(sysexData[i], HEX);
            Serial.print(" ");
        }
        if (length > 10) Serial.print("...");
        Serial.println();
        #endif

        bool isLiveFrame = (length >= 10 &&
                            sysexData[0] == SYSEX_START &&
                            sysexData[1] == MANUFACTURER_ID &&
                            sysexData[2] == DEVICE_ID &&
                            sysexData[3] == 0x7F);
        if (!isLiveFrame) {
            processSysEx(sysexData, length);
            if (neoTrellisLink.isConnected()) {
                neoTrellisLink.sendRaw(sysexData, length);
            }
            continue;
        }

        uint8_t command = 0;
        uint8_t sequence = 0;
        uint16_t payloadLen = 0;
        const uint8_t* payload = nullptr;
        if (!parseLiveSysExFrame(sysexData, length, command, sequence, payloadLen, payload)) {
            Serial.println("Teensy: Ignoring malformed Live SysEx frame (length/checksum mismatch)");
            continue;
        }

        #ifdef DEBUG_LIVE_LOG
        Serial.print("Live SysEx CMD:0x");
        Serial.print(command, HEX);
        Serial.print(" PAYLOAD:");
        Serial.println(payloadLen);
        #endif

        switch (command) {
            case CMD_GRID_UPDATE: {
                if (payloadLen == 96) {
                    neoTrellisLink.sendCommand(CMD_LED_GRID_UPDATE, payload, static_cast<int>(payloadLen));
                    guiInterface.sendGridColors7bit(payload, static_cast<int>(payloadLen));
                    Serial.println("Teensy: Forwarded grid bulk (96 bytes, 7-bit RGB) to M4 + GUI");
                } else if (payloadLen == 192) {
                    neoTrellisLink.sendCommand(CMD_LED_GRID_UPDATE_14, payload, static_cast<int>(payloadLen));
                    guiInterface.sendGridColors14bit(payload, static_cast<int>(payloadLen));
                    Serial.println("Teensy: Forwarded grid bulk (192 bytes, 14-bit RGB) to M4 + GUI");
                } else {
                    Serial.print("Live Grid: payload length invalid (expected 96 or 192, got ");
                    Serial.print(payloadLen);
                    Serial.println(")");
                    break;
                }

                if (!gridSeen) {
                    gridSeen = true;
                    Serial.println("Teensy: First grid seen — enabling key scanning on M4");
                    neoTrellisLink.sendCommand(CMD_ENABLE_KEYS, nullptr, 0);
                }
                break;
            }

            case CMD_GRID_SINGLE_PAD: {
                if (payloadLen != 7) {
                    Serial.print("Live: CMD_GRID_SINGLE_PAD invalid length ");
                    Serial.println(payloadLen);
                    break;
                }
                uint8_t padIndex = payload[0] & 0x7F;
                if (padIndex >= TOTAL_KEYS) {
                    Serial.print("Live: CMD_GRID_SINGLE_PAD pad out of range ");
                    Serial.println(padIndex);
                    break;
                }
                auto decode14 = [&](int offset) -> uint16_t {
                    return (uint16_t)(((payload[offset] & 0x7F) << 7) | (payload[offset + 1] & 0x7F));
                };
                uint16_t r14 = decode14(1);
                uint16_t g14 = decode14(3);
                uint16_t b14 = decode14(5);

                auto compressTo8 = [](uint16_t value) -> uint8_t {
                    return (value <= 0xFF) ? static_cast<uint8_t>(value) : static_cast<uint8_t>(value >> 6);
                };
                uint8_t r8 = compressTo8(r14);
                uint8_t g8 = compressTo8(g14);
                uint8_t b8 = compressTo8(b14);

                Serial.printf("Single-pad update %u -> RGB %u,%u,%u\n", padIndex, r8, g8, b8);

                uint8_t m4Data[] = {
                    padIndex,
                    payload[1], payload[2],
                    payload[3], payload[4],
                    payload[5], payload[6]
                };
                neoTrellisLink.sendCommand(CMD_LED_PAD_UPDATE_14, m4Data, sizeof(m4Data));
                guiInterface.sendPadColor14bit(padIndex, payload[1], payload[2], payload[3], payload[4], payload[5], payload[6]);
                break;
            }

            case CMD_RING_POSITION: {
                if (payloadLen >= 7) {
                    uint16_t track = static_cast<uint16_t>(((payload[0] & 0x7F) << 7) | (payload[1] & 0x7F));
                    uint16_t scene = static_cast<uint16_t>(((payload[2] & 0x7F) << 7) | (payload[3] & 0x7F));
                    uint8_t width = payload[4] & 0x7F;
                    uint8_t height = payload[5] & 0x7F;
                    uint8_t overview = payload[6] & 0x7F;
                    Serial.printf("Ring position -> track %u scene %u w=%u h=%u ov=%u\n",
                                  track, scene, width, height, overview);
                } else {
                    Serial.println("Ring position payload too short");
                }
                uiBridge.processLiveSysEx(CMD_RING_POSITION, payload, static_cast<uint8_t>(payloadLen));
                break;
            }

            case CMD_HANDSHAKE_REPLY: {
                Serial.println("Live: Handshake final (0x61) received.");
                break;
            }

            case CMD_CLIP_STATE: {
                if (payloadLen < 9) {
                    Serial.println("Live: CMD_CLIP_STATE payload too short");
                    break;
                }
                uint8_t track = payload[0] & 0x7F;
                uint8_t scene = payload[1] & 0x7F;
                uint8_t state = payload[2] & 0x7F;
                auto decodeColor = [&](uint16_t offset) -> uint8_t {
                    return (uint8_t)(((payload[offset] & 0x7F) << 7) | (payload[offset + 1] & 0x7F));
                };
                uint8_t r = decodeColor(3);
                uint8_t g = decodeColor(5);
                uint8_t b = decodeColor(7);
                int padIndex = scene * GRID_TRACKS + track;
                if (padIndex < TOTAL_KEYS) {
                    Serial.printf("CLIP_STATE pad %02d (T%d,S%d) state=%u RGB=%u,%u,%u\n",
                                  padIndex, track, scene, state, r, g, b);
                    uint8_t m4Data[] = {
                        static_cast<uint8_t>(padIndex),
                        payload[3], payload[4],
                        payload[5], payload[6],
                        payload[7], payload[8]
                    };
                    neoTrellisLink.sendCommand(CMD_LED_PAD_UPDATE_14, m4Data, sizeof(m4Data));
                    guiInterface.sendPadColor14bit(padIndex, payload[3], payload[4], payload[5], payload[6], payload[7], payload[8]);
                    uint8_t statePayload[] = { static_cast<uint8_t>(padIndex), state };
                    neoTrellisLink.sendCommand(CMD_LED_CLIP_STATE, statePayload, sizeof(statePayload));
                }
                break;
            }

            case CMD_CLIP_NAME: {
                if (payloadLen < 3) {
                    Serial.println("Live: Clip name payload too short");
                    break;
                }
                uint8_t track = payload[0] & 0x7F;
                uint8_t scene = payload[1] & 0x7F;
                // Payload format: [track] [scene] [name_chars...]
                // Extract null-terminated name string
                char clipName[64] = {0};
                int nameLen = payloadLen - 2; // Subtract track and scene bytes
                if (nameLen > 0 && nameLen < 63) {
                    for (int i = 0; i < nameLen; i++) {
                        clipName[i] = static_cast<char>(payload[2 + i] & 0x7F);
                    }
                    clipName[nameLen] = '\0';
                    guiInterface.sendClipName(track, scene, clipName);
                    Serial.printf("Clip name -> track %u scene %u: %s\n", track, scene, clipName);
                } else {
                    Serial.printf("Clip name -> track %u scene %u (len=%u)\n", track, scene, payloadLen);
                }
                break;
            }

            case CMD_CLIP_LOOP:
            case CMD_CLIP_MUTED:
            case CMD_CLIP_WARP:
            case CMD_CLIP_START:
            case CMD_CLIP_END: {
                if (payloadLen < 3) {
                    Serial.println("Live: Clip meta payload too short");
                    break;
                }
                uint8_t track = payload[0] & 0x7F;
                uint8_t scene = payload[1] & 0x7F;
                Serial.printf("Clip CMD 0x%02X -> track %u scene %u len=%u\n", command, track, scene, payloadLen);
                break;
            }

            case CMD_CLIP_TRIGGER:
            case CMD_CLIP_STOP:
            case CMD_SCENE_FIRE: {
                // These normally originate from hardware; log if Live echoes them
                Serial.printf("Live issued clip/scene command 0x%02X (len=%u)\n", command, payloadLen);
                break;
            }

            case CMD_SCENE_NAME:
            case CMD_SCENE_COLOR:
            case CMD_SCENE_IS_TRIGGERED:
                Serial.printf("Scene CMD 0x%02X len=%u\n", command, payloadLen);
                break;

            case CMD_TRACK_NAME: {
                if (payloadLen < 2) {
                    Serial.println("Live: Track name payload too short");
                    break;
                }
                uint8_t track = payload[0] & 0x7F;
                // Payload format: [track] [name_chars...]
                char trackName[64] = {0};
                int nameLen = payloadLen - 1; // Subtract track byte
                if (nameLen > 0 && nameLen < 63) {
                    for (int i = 0; i < nameLen; i++) {
                        trackName[i] = static_cast<char>(payload[1 + i] & 0x7F);
                    }
                    trackName[nameLen] = '\0';
                    guiInterface.sendTrackName(track, trackName);
                    Serial.printf("Track name -> track %u: %s\n", track, trackName);
                } else {
                    Serial.printf("Track name -> track %u (len=%u)\n", track, payloadLen);
                }
                break;
            }

            case CMD_TRACK_VOLUME:
            case CMD_TRACK_PAN:
            case CMD_TRACK_MUTE:
            case CMD_TRACK_SOLO:
            case CMD_TRACK_ARM:
            case CMD_TRACK_SEND_A:
#if CMD_TRACK_SEND_A != CMD_TRACK_SEND_B
            case CMD_TRACK_SEND_B:
#endif
            {
                if (payloadLen < 1) {
                    Serial.println("Live: Track command payload too short");
                    break;
                }
                uint8_t track = payload[0] & 0x7F;
                uint8_t value = (payloadLen > 1) ? (payload[1] & 0x7F) : 0;
                Serial.printf("Track CMD 0x%02X -> track %u value %u\n", command, track, value);
                break;
            }

            case CMD_TRACK_COLOR:
                Serial.printf("Track color update track %u\n", payloadLen ? (payload[0] & 0x7F) : 0);
                break;

            case CMD_TRACK_CROSSFADE:
                Serial.printf("Track-wide CMD 0x%02X len=%u\n", command, payloadLen);
                break;

            case CMD_SELECTED_TRACK:
            case CMD_SELECTED_SCENE: {
                uint8_t value = payloadLen ? (payload[0] & 0x7F) : 0;
                Serial.printf("Selection CMD 0x%02X -> %u\n", command, value);
                break;
            }

            case CMD_TEMPO: {
                // BPM sent as 16-bit value (MSB, LSB) in units of 0.1 BPM
                if (payloadLen >= 2) {
                    uint16_t bpm_fixed = (uint16_t)(((payload[0] & 0x7F) << 7) | (payload[1] & 0x7F));
                    float bpm = static_cast<float>(bpm_fixed) / 10.0f;
                    guiInterface.sendBPM(bpm);
                    Serial.printf("Tempo -> %.1f BPM\n", bpm);
                } else {
                    Serial.println("Tempo payload too short");
                }
                break;
            }

            case CMD_TRANSPORT_PLAY:
            case CMD_TRANSPORT_RECORD: {
                uint8_t value = payloadLen ? (payload[0] & 0x7F) : 0;
                bool isPlaying = (command == CMD_TRANSPORT_PLAY) ? (value > 0) : false;
                bool isRecording = (command == CMD_TRANSPORT_RECORD) ? (value > 0) : false;
                guiInterface.sendTransportState(isPlaying, isRecording);
                Serial.printf("Transport CMD 0x%02X -> value %u (sent to GUI)\n", command, value);
                break;
            }

            case CMD_TRANSPORT_LOOP:
            case CMD_TRANSPORT_METRONOME:
            case CMD_TRANSPORT_SIGNATURE:
            case CMD_TRANSPORT_POSITION:
            case CMD_RECORD_QUANTIZATION:
            case CMD_QUANTIZE_CLIP:
            case CMD_BACK_TO_ARRANGER:
            case CMD_ARRANGEMENT_RECORD:
            case CMD_TRANSPORT: {
                uint8_t value = payloadLen ? (payload[0] & 0x7F) : 0;
                Serial.printf("Transport CMD 0x%02X -> value %u len=%u\n", command, value, payloadLen);
                break;
            }

            case CMD_DISCONNECT: {
                Serial.println("Live: Disconnect command received — clearing state.");
                liveConnected = false;
                liveConnectedAt = 0;
                gridSeen = false;
                gridRequestRetries = 0;
                gridRequestLastAttempt = 0;
                uint8_t clearFrame[TOTAL_KEYS * 3] = {0};
                neoTrellisLink.sendCommand(CMD_LED_GRID_UPDATE, clearFrame, sizeof(clearFrame));
                neoTrellisLink.sendCommand(CMD_DISABLE_KEYS, nullptr, 0);
                break;
            }

            case CMD_HANDSHAKE:
                Serial.println("Live: secondary handshake ping received");
                break;

            default:
                // Custom vendor messages (F0 7D ...) still go through processSysEx
                Serial.print("Unhandled Live CMD:0x");
                Serial.println(command, HEX);
                break;
        }
    }
}

void LiveController::updateClipState(int padIndex, int state) {
    if (padIndex < 0 || padIndex >= TOTAL_KEYS) return;
    
    uint32_t color = COLOR_EMPTY;
    switch (state) {
        case CLIP_STATE_EMPTY:     color = COLOR_EMPTY; break;
        case CLIP_STATE_STOPPED:   color = COLOR_LOADED; break;
        case CLIP_STATE_PLAYING:   color = COLOR_PLAYING; break;
        case CLIP_STATE_RECORDING: color = COLOR_RECORDING; break;
        case CLIP_STATE_QUEUED:    color = COLOR_TRIGGERED; break; // yellow for queued
        default:                    color = COLOR_EMPTY; break;
    }
    
    neoTrellisLink.setPixelColor(padIndex, color);
}

void LiveController::sendClipTrigger(uint8_t track, uint8_t scene) {
    if (!liveConnected) {
        Serial.println("Teensy: Ignoring clip trigger before Live handshake");
        return;
    }
    uint8_t data[] = {track, scene};
    sendSysExToAbleton(CMD_CLIP_TRIGGER, data, 2);
}

void LiveController::sendSysExToAbleton(uint8_t command, const uint8_t* data, int dataLength, bool requireLiveConnection) {
    if (requireLiveConnection && !liveConnected) {
        return;
    }
    if (dataLength < 0) dataLength = 0;

    static uint8_t sequence = 0;
    sequence = (sequence + 1) & 0x7F;

    const uint8_t lenMsb = (dataLength >> 7) & 0x7F;
    const uint8_t lenLsb = dataLength & 0x7F;
    const int messageSize = 4 + 4 + dataLength + 2; // header + cmd/seq/len + checksum + end
    uint8_t sysexMsg[messageSize];

    int index = 0;
    sysexMsg[index++] = SYSEX_START;
    sysexMsg[index++] = MANUFACTURER_ID;
    sysexMsg[index++] = DEVICE_ID;
    sysexMsg[index++] = 0x7F;
    sysexMsg[index++] = command & 0x7F;
    sysexMsg[index++] = sequence;
    sysexMsg[index++] = lenMsb;
    sysexMsg[index++] = lenLsb;

    for (int i = 0; i < dataLength; ++i) {
        uint8_t value = data ? data[i] : 0;
        sysexMsg[index++] = value & 0x7F;
    }

    uint8_t checksum = command ^ sequence;
    for (int i = 0; i < dataLength; ++i) {
        checksum ^= data ? data[i] : 0;
    }
    sysexMsg[index++] = checksum & 0x7F;
    sysexMsg[index++] = SYSEX_END;

    #ifdef DEBUG_LIVE_LOG
    Serial.print("SYSEX_OUT: ");
    for (int i = 0; i < index; i++) {
        Serial.print(sysexMsg[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    #endif

    usbMIDI.sendSysEx(index, sysexMsg);
}

void LiveController::sendTransportCommand(uint8_t command) {
    sendSysExToAbleton(command, nullptr, 0);
}

// === HELPER FUNCTIONS ===

void LiveController::handleKeyPress(int board, int key) {
    int globalKey = (board * 16) + key;
    // Map 1D key index to (track, scene) using 8x4 layout
    int track = globalKey % GRID_TRACKS;
    int scene = globalKey / GRID_TRACKS;
    
    Serial.print("Pad "); Serial.print(globalKey);
    Serial.print(" (Track "); Serial.print(track);
    Serial.print(", Scene "); Serial.print(scene);
    Serial.println(") pressed");
    
    sendClipTrigger(track, scene);
    neoTrellisLink.setPixelColor(globalKey, COLOR_TRIGGERED);
}

void LiveController::handleKeyRelease(int board, int key) {
    int globalKey = (board * 16) + key;
    Serial.print("Pad "); Serial.print(globalKey); Serial.println(" released");
    neoTrellisLink.setPixelColor(globalKey, COLOR_LOADED);
}

int LiveController::getBoardFromKey(int globalKey) {
    return globalKey / 16;
}

int LiveController::getLocalKeyFromGlobal(int globalKey) {
    return globalKey % 16;
}

void LiveController::processSysEx(uint8_t* data, int length) {
    if (length < 5 || data[0] != SYSEX_START || data[length - 1] != SYSEX_END) return;
    if (data[1] != MANUFACTURER_ID || data[2] != DEVICE_ID) return;
    
    uint8_t command = data[3];
    switch (command) {
        case CMD_LED_CLIP_STATE:
            if (length >= 6) {
                int padIndex = data[4];
                int state = data[5];
                updateClipState(padIndex, state);
            }
            break;
    }
}

void LiveController::processHandshakeMessage(uint8_t* data, int length) {
    uint8_t command = 0;
    uint8_t sequence = 0;
    uint16_t payloadLen = 0;
    const uint8_t* payload = nullptr;
    if (!parseLiveSysExFrame(data, length, command, sequence, payloadLen, payload) || command != CMD_HANDSHAKE) {
        return;
    }

    if (!isHardwareReady()) {
        Serial.println("Teensy: Live handshake received but hardware not ready — deferring response");
        return;
    }

    if (payloadLen < 2) {
        Serial.println("Teensy: Handshake payload too short, continuing anyway");
    }

    Serial.println("Teensy: Ableton Live handshake detected. Responding...");
    sendHandshakeResponse();

    // Allow Live to re-establish state if it reopens ports
    liveConnected = true;
    liveConnectedAt = millis();
    gridSeen = false;
    gridRequestRetries = 0;
    gridRequestLastAttempt = 0;
    Serial.println("Teensy: ✓ Live connection established (ack sent).");
    if (neoTrellisLink.isConnected()) {
        Serial.println("Teensy: Live connected — enabling NeoTrellis key scanning.");
        neoTrellisLink.sendCommand(CMD_ENABLE_KEYS, nullptr, 0);
    }
}

void LiveController::sendHandshakeResponse() {
    const uint8_t sequence = 0x01;
    const uint8_t payload[2] = {0x54, 0x53}; // "TS"
    const uint8_t lenMsb = 0x00;
    const uint8_t lenLsb = 0x02;
    uint8_t checksum = CMD_HANDSHAKE ^ sequence ^ payload[0] ^ payload[1];
    checksum &= 0x7F;
    uint8_t message[] = {
        SYSEX_START, MANUFACTURER_ID, DEVICE_ID, 0x7F,
        CMD_HANDSHAKE, sequence, lenMsb, lenLsb,
        payload[0], payload[1],
        checksum,
        SYSEX_END
    };
    usbMIDI.sendSysEx(sizeof(message), message);
    usbMIDI.send_now();
    Serial.println("Teensy: Handshake reply sent (TS).");
}

void LiveController::waitForLiveHandshake() {
    if (liveConnected) return;

    // This loop now only looks for one thing: the SysEx handshake from Live.
    // It's much faster and less prone to timing errors.
    while (usbMIDI.read()) {
        if (usbMIDI.getType() == usbMIDI.SystemExclusive) {
            uint8_t* data = usbMIDI.getSysExArray();
            uint16_t length = usbMIDI.getSysExArrayLength();
            
            // A valid handshake message will not be null and will have a minimum length.
            if (data && length >= 6) {
                // Pass the message to the dedicated handler.
                processHandshakeMessage(data, length);
            }
        }
    }
}

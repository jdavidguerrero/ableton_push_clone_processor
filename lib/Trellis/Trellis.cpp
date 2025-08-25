#include "Trellis.h"
#include "MidiCommands.h"
#include "shared/Config.h"

// Constructor
Trellis::Trellis() {
    // This is now empty as there are no I2C objects to initialize.
}

// Destructor - nothing to free (global lifetime on MCU)
Trellis::~Trellis() {}

// Global reference to the main Trellis instance (defined in Teensy Hardware.cpp)
extern Trellis trellis;

// The I2C-based key callback has been removed.
// Key events are now handled by the M4 board and sent via UART.

void Trellis::begin() {
    // This function is now a placeholder.
    // The actual hardware (M4) is initialized via UART.
    // The I2C-based NeoTrellis initialization has been removed
    // to match the UART-based architecture.
    Serial.println("Trellis class initialized (UART mode).");
}

void Trellis::read() {
    // This function is now a placeholder.
    // The UartHandler is responsible for reading data from the M4.
    // The I2C-based read has been removed.
}

void Trellis::setM4Connected(bool connected) {
    bool was = m4Connected;
    m4Connected = connected;
    if (connected != was) {
        Serial.print("Teensy: M4 UART connection state -> ");
        Serial.println(connected ? "CONNECTED" : "DISCONNECTED");
        // Animation disabled to prevent visual disruptions
    }
}

void Trellis::setPixelColor(int key, uint32_t color) {
    if (key < 0 || key >= TOTAL_KEYS) return;

    // This function now sends a SysEx command over UART to the M4
    // instead of controlling I2C hardware directly.
    byte r = (color >> 16) & 0xFF;
    byte g = (color >> 8) & 0xFF;
    byte b = color & 0xFF;

    byte data[] = { (byte)key, r, g, b };
    sendSysExToM4(CMD_LED_RGB_STATE, data, sizeof(data));
}

void Trellis::testAllPads() {
    Serial.println("Teensy: Sending minimal test to M4...");
    
    // Just trigger the M4's quick test (corners flash)
    // M4 will handle its own minimal visual feedback
    
    Serial.println("Teensy: M4 test triggered - waiting for M4 to complete");
}

void Trellis::processMIDI() {
    // Process all pending USB MIDI messages
    while (usbMIDI.read()) {
        if (usbMIDI.getType() == usbMIDI.SystemExclusive) {
            uint8_t* sysexData = usbMIDI.getSysExArray();
            uint16_t length = usbMIDI.getSysExArrayLength();
            if (sysexData && length > 0) {
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
                
                // Process Universal Real Time SysEx from Live (F0 7F ...)
                if (length >= 5 && sysexData[0] == 0xF0 && sysexData[1] == 0x7F) {
                    uint8_t command = sysexData[4];
                    
                    #ifdef DEBUG_LIVE_LOG
                    Serial.print("Live SysEx CMD:0x"); Serial.print(command, HEX);
                    Serial.print(" LEN:"); Serial.println(length);
                    #endif
                    
                    // Handle different Live commands
                    switch (command) {
                        case 0x10: // LED command
                            if (length >= 15) {
                                uint8_t track = sysexData[7];
                                uint8_t scene = sysexData[8]; 
                                uint8_t r = sysexData[10];
                                uint8_t g = sysexData[11];
                                uint8_t b = sysexData[12];
                                
                                int padIndex = scene * 4 + track;
                                if (padIndex < TOTAL_KEYS) {
                                    uint32_t color = (r << 16) | (g << 8) | b;
                                    Serial.print("LED: Pad "); Serial.print(padIndex);
                                    Serial.print(" -> RGB("); Serial.print(r); Serial.print(",");
                                    Serial.print(g); Serial.print(","); Serial.print(b); Serial.println(")");
                                    
                                    // Send to M4 in our format
                                    uint8_t m4Data[] = { (uint8_t)padIndex, r, g, b };
                                    sendSysExToM4(CMD_LED_RGB_STATE, m4Data, sizeof(m4Data));
                                }
                            }
                            break;
                            
                        case 0x1B: // Clip state (track-based)
                            if (length >= 11) {
                                uint8_t track = sysexData[7];
                                uint8_t state = sysexData[8];
                                Serial.print("Clip state: Track "); Serial.print(track);
                                Serial.print(" State "); Serial.println(state);
                                // Could update track-wide indicators here
                            }
                            break;
                            
                        case 0x1C: // Clip color/properties 
                            if (length >= 13) {
                                uint8_t track = sysexData[7];
                                Serial.print("Clip color: Track "); Serial.println(track);
                                // Handle clip color updates
                            }
                            break;
                            
                        case 0x23: // Track mute
                        case 0x24: // Track solo  
                        case 0x25: // Track arm
                        case 0x21: // Track volume
                        case 0x22: // Track pan
                        case 0x26: // Track send
                        case 0x27: // Track name
                        case 0x28: // Track color
                        case 0x29: // Track send A
                        case 0x2A: // Track send B
                            Serial.print("Track update CMD:0x"); Serial.print(command, HEX);
                            if (length >= 11) {
                                uint8_t track = sysexData[7];
                                Serial.print(" Track:"); Serial.println(track);
                            } else {
                                Serial.println(" (short message)");
                            }
                            break;
                            
                        case 0x40: // Transport play
                        case 0x41: // Transport stop
                        case 0x42: // Transport record
                        case 0x43: // BPM
                        case 0x44: // Time signature
                        case 0x45: // Song position
                            Serial.print("Transport CMD:0x"); Serial.println(command, HEX);
                            break;
                            
                        default:
                            Serial.print("Unhandled Live CMD:0x"); Serial.println(command, HEX);
                            break;
                    }
                } else {
                    // Process our custom format (F0 7D ...)
                    processSysEx(sysexData, length);
                    
                    // Forward custom commands to M4
                    if (m4Connected) {
                        Serial1.write(sysexData, length);
                    }
                }
            }
        }
    }
}

void Trellis::updateClipState(int padIndex, int state) {
    if (padIndex < 0 || padIndex >= TOTAL_KEYS) return;
    
    uint32_t color = COLOR_EMPTY;
    switch (state) {
        case CLIP_STATE_EMPTY:    color = COLOR_EMPTY; break;
        case CLIP_STATE_LOADED:   color = COLOR_LOADED; break;
        case CLIP_STATE_PLAYING:  color = COLOR_PLAYING; break;
        case CLIP_STATE_RECORDING: color = COLOR_RECORDING; break;
        case CLIP_STATE_SELECTED: color = COLOR_SELECTED; break;
        case CLIP_STATE_TRIGGERED: color = COLOR_TRIGGERED; break;
    }
    
    setPixelColor(padIndex, color);
}

void Trellis::sendClipTrigger(uint8_t track, uint8_t scene) {
    uint8_t data[] = {track, scene};
    sendSysExToAbleton(CMD_CLIP_TRIGGER, data, 2);
}

void Trellis::sendSysExToAbleton(uint8_t command, uint8_t* data, int dataLength) {
    // Remote Script expects: [HEADER] [CMD] [SEQ] [LENGTH] [DATA...] [CHECKSUM] [END]
    static uint8_t sequence = 0; // Static sequence counter
    sequence = (sequence + 1) & 0x7F; // Keep in MIDI range 0-127
    
    int messageSize = dataLength + 8; // Header(4) + CMD + SEQ + LENGTH + DATA + CHECKSUM + END
    uint8_t sysexMsg[messageSize];
    
    int index = 0;
    // Header (4 bytes): F0 7D 00 ?? (4th byte unknown, using 0x00)
    sysexMsg[index++] = SYSEX_START;    // 0xF0
    sysexMsg[index++] = MANUFACTURER_ID; // 0x7D  
    sysexMsg[index++] = DEVICE_ID;       // 0x00
    sysexMsg[index++] = 0x00;            // Header 4th byte (unknown)
    
    // Command, Sequence, Length
    sysexMsg[index++] = command & 0x7F;
    sysexMsg[index++] = sequence;
    sysexMsg[index++] = dataLength & 0x7F;
    
    // Data payload
    for (int i = 0; i < dataLength; i++) {
        sysexMsg[index++] = data[i] & 0x7F; // Ensure MIDI range
    }
    
    // Calculate checksum: CMD ^ SEQ ^ DATA[0] ^ DATA[1] ^ ...
    uint8_t checksum = command ^ sequence;
    for (int i = 0; i < dataLength; i++) {
        checksum ^= data[i];
    }
    sysexMsg[index++] = checksum & 0x7F;
    
    // End
    sysexMsg[index++] = SYSEX_END;
    
    // Debug: Show what we're sending to Remote Script
    Serial.print("SYSEX_OUT: ");
    for (int i = 0; i < index; i++) {
        Serial.print(sysexMsg[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    usbMIDI.sendSysEx(index, sysexMsg);
}

void Trellis::sendSysExToM4(uint8_t command, uint8_t* data, int dataLength) {
    int messageSize = dataLength + 5;
    uint8_t sysexMsg[messageSize];
    
    int index = 0;
    sysexMsg[index++] = SYSEX_START;
    sysexMsg[index++] = MANUFACTURER_ID;
    sysexMsg[index++] = DEVICE_ID;
    sysexMsg[index++] = command;
    
    for (int i = 0; i < dataLength; i++) {
        sysexMsg[index++] = data[i];
    }
    
    sysexMsg[index++] = SYSEX_END;
    
    // Debug logging before sending
    // Silent M4 communication
    Serial.print(command, HEX);
    Serial.print(", Total bytes: ");
    Serial.print(index);
    Serial.print(" [");
    for (int i = 0; i < index; i++) {
        Serial.print("0x");
        Serial.print(sysexMsg[i], HEX);
        if (i < index - 1) Serial.print(" ");
    }
    Serial.println("]");
    
    // Send via Serial1 (hardware UART)
    int bytesWritten = Serial1.write(sysexMsg, index);
    Serial1.flush(); // Ensure data is sent immediately
    
    Serial.print("Teensy: Sent ");
    Serial.print(bytesWritten);
    Serial.print(" bytes to M4 via Serial1 (TX1=Pin 1)");
    Serial.println();
}

void Trellis::sendTransportCommand(uint8_t command) {
    sendSysExToAbleton(command, nullptr, 0);
}

// === HELPER FUNCTIONS ===

void Trellis::handleKeyPress(int board, int key) {
    int globalKey = (board * 16) + key;
    int track = globalKey % 4;
    int scene = globalKey / 4;
    
    Serial.print("Pad "); Serial.print(globalKey);
    Serial.print(" (Track "); Serial.print(track);
    Serial.print(", Scene "); Serial.print(scene);
    Serial.println(") pressed");
    
    sendClipTrigger(track, scene);
    setPixelColor(globalKey, COLOR_TRIGGERED);
}

void Trellis::handleKeyRelease(int board, int key) {
    int globalKey = (board * 16) + key;
    Serial.print("Pad "); Serial.print(globalKey); Serial.println(" released");
    setPixelColor(globalKey, COLOR_LOADED);
}

int Trellis::getBoardFromKey(int globalKey) {
    return globalKey / 16;
}

int Trellis::getLocalKeyFromGlobal(int globalKey) {
    return globalKey % 16;
}

void Trellis::processSysEx(uint8_t* data, int length) {
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

void Trellis::processHandshakeMessage(uint8_t* data, int length) {
    if (length < 5 || data[0] != SYSEX_START || data[length - 1] != SYSEX_END) {
        Serial.println("Teensy: Invalid SysEx format in handshake");
        return;
    }

    Serial.print("Teensy: Processing handshake message (");
    Serial.print(length);
    Serial.print(" bytes): ");
    for (int i = 0; i < min(length, 16); i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    if (length > 16) Serial.print("...");
    Serial.println();

    // 1) Handshake de tu API: F0 7D 00 XX 01 ... F7  
    if (data[1] == MANUFACTURER_ID && data[2] == DEVICE_ID && 
        (data[4] == CMD_HANDSHAKE || (data[3] == 0x00 && data[4] == CMD_HANDSHAKE))) {
        Serial.println("Teensy: Live handshake received (CMD_HANDSHAKE). Sending REPLY...");
        
        // Send handshake REPLY with "LV" payload as expected by Remote Script  
        uint8_t responsePayload[] = {0x4C, 0x56}; // "LV" for Live response
        sendSysExToAbleton(CMD_HANDSHAKE_REPLY, responsePayload, sizeof(responsePayload));
        usbMIDI.send_now();
        
        liveConnected = true;
        Serial.println("Teensy: ✓ Live connection established via API handshake!");
        return;
    }

    // 2) Universal Device Inquiry (Non-RealTime): F0 7E 7F 06 01 F7 (u otros de 0x7E)
    if (data[1] == 0x7E && length >= 6 && data[3] == 0x06 && data[4] == 0x01) {
        Serial.println("Teensy: Universal SysEx inquiry detected. Sending device info response...");
        uint8_t response[] = {
            0xF0, 0x7E, 0x7F, 0x06, 0x02, // header: non-realtime, device ID all, device inquiry response
            0x7D, 0x00,                   // manufacturer ID (2 bytes)
            0x00, 0x01,                   // device member (2 bytes)
            0x00, 0x00, 0x00, 0x01,       // software revision (4 bytes)
            0xF7
        };
        usbMIDI.sendSysEx(sizeof(response), response);
        usbMIDI.send_now();
        liveConnected = true;
        Serial.println("Teensy: ✓ Live connection established via Universal Device Inquiry!");
        return;
    }

    // 3) Ableton Push-style realtime handshake (0x7F) con subcomando 0x60 XX (dinámico)
    // Expected: F0 7F 00 7F 60 01 02 50 43 72 F7 (11 bytes)
    Serial.print("Teensy: Checking Push-style: data[1]=");
    Serial.print(data[1], HEX);
    Serial.print(" data[2]=");
    Serial.print(data[2], HEX);
    Serial.print(" data[3]=");
    Serial.print(data[3], HEX);
    Serial.print(" data[4]=");
    Serial.print(data[4], HEX);
    Serial.print(" length=");
    Serial.println(length);
    
    // Check for exact Live handshake pattern: F0 7F XX 7F 60 ...
    if (data[1] == 0x7F && data[3] == 0x7F && data[4] == 0x60 && length >= 6) {
        Serial.println("Teensy: Ableton realtime handshake detected. Sending EXACT response...");
        
        // Send EXACTLY the same message that Pocket MIDI sends successfully
        // Based on working example: F0 7F 00 7F 60 01 02 50 43 72 F7
        uint8_t workingResponse[] = {0xF0, 0x7F, 0x00, 0x7F, 0x60, 0x01, 0x02, 0x50, 0x43, 0x72, 0xF7};
        
        Serial.print("PUSH_RESPONSE: ");
        for (int i = 0; i < sizeof(workingResponse); i++) {
            Serial.print(workingResponse[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        usbMIDI.sendSysEx(sizeof(workingResponse), workingResponse);
        usbMIDI.send_now();
        
        liveConnected = true;
        Serial.println("Teensy: ✓ Live connection established with exact Push response!");
        return;
    }
    
    Serial.println("Teensy: Unknown handshake message type");
}

void Trellis::initializeM4Communication() {
    Serial.println("Teensy: Initializing UART communication with M4...");
    Serial.println("Teensy: This will send data via Serial1 (pins TX1=1, RX1=0)");
    
    // Wait a moment for stability
    delay(100);
    
    // Send handshake to M4 with "PUSHCLONE" signature
    uint8_t handshakeData[] = {0x50, 0x55, 0x53, 0x48, 0x43, 0x4C, 0x4F, 0x4E, 0x45}; // "PUSHCLONE"
    
    Serial.println("Teensy: About to send handshake to M4...");
    Serial.print("Teensy: Handshake data (9 bytes): ");
    for (int i = 0; i < 9; i++) {
        Serial.print("0x");
        Serial.print(handshakeData[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    sendSysExToM4(CMD_HANDSHAKE, handshakeData, 9);
    
    Serial.println("Teensy: Handshake sent to M4 via Serial1");
    
    // Send UART confirmation animation to verify M4 is responding
    delay(200);
    Serial.println("Teensy: Waiting for M4 handshake echo to confirm UART link...");
}

void Trellis::waitForLiveHandshake() {
    if (liveConnected) return;
    
    // Debug: Show we're actively checking (less frequent)
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 10000) {
        Serial.println("Teensy: waitForLiveHandshake() active - checking for messages...");
        lastDebug = millis();
    }
    
    // Process all available USB MIDI messages - LOG EVERYTHING
    while (usbMIDI.read()) {
        // Small delay to ensure complete message reception
        delayMicroseconds(100);
        
        // LOG ALL MIDI MESSAGES REGARDLESS OF TYPE
        Serial.print("MIDI_IN: Type=");
        Serial.print(usbMIDI.getType());
        Serial.print(" Chan=");
        Serial.print(usbMIDI.getChannel());
        Serial.print(" Data1=");
        Serial.print(usbMIDI.getData1());
        Serial.print(" Data2=");
        Serial.print(usbMIDI.getData2());
        Serial.print(" Time=");
        Serial.println(millis());
        
        if (usbMIDI.getType() != usbMIDI.SystemExclusive) {
            continue;
        }
        
        Serial.println("Teensy: SysEx message detected!");
        uint8_t* data = usbMIDI.getSysExArray();
        uint16_t length = usbMIDI.getSysExArrayLength();
        
        Serial.print("Teensy: SysEx fragment - length: ");
        Serial.println(length);
        
        if (!data) {
            Serial.println("Teensy: NULL data pointer, skipping");
            continue;
        }
        
        // Always print the raw data we got
        Serial.print("Teensy: Raw SysEx fragment: ");
        for (int i = 0; i < length; i++) {
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        // Handle SysEx fragmentation - accumulate data
        for (int i = 0; i < length; i++) {
            if (data[i] == 0xF0) {
                // Start of SysEx - reset buffer
                sysexInProgress = true;
                sysexBufferIndex = 0;
                Serial.println("Teensy: SysEx start detected, beginning accumulation");
            }
            
            if (sysexInProgress && sysexBufferIndex < SYSEX_BUFFER_SIZE) {
                sysexBuffer[sysexBufferIndex++] = data[i];
            }
            
            if (data[i] == 0xF7) {
                // End of SysEx - process complete message
                sysexInProgress = false;
                Serial.print("Teensy: Complete SysEx accumulated (");
                Serial.print(sysexBufferIndex);
                Serial.print(" bytes): ");
                for (int j = 0; j < sysexBufferIndex; j++) {
                    Serial.print(sysexBuffer[j], HEX);
                    Serial.print(" ");
                }
                Serial.println();
                
                // Now process the complete message
                if (sysexBufferIndex >= 5) {
                    // Process handshake messages directly here, not through processSysEx
                    // which only handles messages with our MANUFACTURER_ID
                    processHandshakeMessage(sysexBuffer, sysexBufferIndex);
                }
                sysexBufferIndex = 0;
                return; // Exit after processing complete message
            }
        }
        
        if (sysexInProgress) {
            Serial.print("Teensy: SysEx in progress, accumulated ");
            Serial.print(sysexBufferIndex);
            Serial.println(" bytes so far");
            continue; // Keep accumulating
        }
        
        // If we get here, it's a complete short message (like F0 F7)
        if (length < 5) {
            Serial.println("Teensy: Short SysEx message, skipping");
            continue;
        }
        
        // Process complete message through dedicated handshake handler
        processHandshakeMessage(data, length);
    }
}

void Trellis::sendUartConfirmationAnimation() {
    Serial.println("Teensy: Starting UART confirmation animation...");
    
    // Animation 1: Wave effect across all pads
    Serial.println("Teensy: Wave animation - Blue wave across grid");
    for (int wave = 0; wave < 2; wave++) { // 2 waves
        for (int i = 0; i < TOTAL_KEYS; i++) {
            // Light up current pad in blue
            setPixelColor(i, 0x0080FF); // Blue
            delay(30);
            
            // Turn off previous pad (keep a trail of 3 pads lit)
            if (i >= 3) {
                setPixelColor(i - 3, 0x000000); // Off
            }
        }
        // Clear remaining trail
        for (int i = TOTAL_KEYS - 3; i < TOTAL_KEYS; i++) {
            setPixelColor(i, 0x000000);
            delay(30);
        }
    }
    
    delay(300);
    
    // Animation 2: Corners to center (uses X_DIM/Y_DIM)
    Serial.println("Teensy: Corner convergence animation - Green");
    // Light up corners based on configured grid
    int corners[] = {
        0,                              // top-left
        X_DIM - 1,                      // top-right
        (Y_DIM - 1) * X_DIM,            // bottom-left
        (Y_DIM - 1) * X_DIM + (X_DIM - 1) // bottom-right
    };
    for (int i = 0; i < 4; i++) {
        setPixelColor(corners[i], 0x00FF00); // Green
    }
    delay(200);
    
    // Expand from corners
    for (int step = 0; step < max(X_DIM, Y_DIM); step++) {
        for (int i = 0; i < TOTAL_KEYS; i++) {
            int row = i / X_DIM;
            int col = i % X_DIM;
            
            // Distance from corners
            int dist_tl = abs(row - 0) + abs(col - 0);
            int dist_tr = abs(row - 0) + abs(col - (X_DIM - 1));
            int dist_bl = abs(row - (Y_DIM - 1)) + abs(col - 0);
            int dist_br = abs(row - (Y_DIM - 1)) + abs(col - (X_DIM - 1));
            
            int min_dist = min(min(dist_tl, dist_tr), min(dist_bl, dist_br));
            
            if (min_dist == step + 1) {
                setPixelColor(i, 0x00FF00); // Green
            }
        }
        delay(150);
    }
    
    delay(300);
    
    // Animation 3: All flash confirmation
    Serial.println("Teensy: Final flash confirmation - White");
    for (int flash = 0; flash < 3; flash++) {
        // All white
        for (int i = 0; i < TOTAL_KEYS; i++) {
            setPixelColor(i, 0xFFFFFF); // White
        }
        delay(100);
        
        // All off
        for (int i = 0; i < TOTAL_KEYS; i++) {
            setPixelColor(i, 0x000000); // Off
        }
        delay(100);
    }
    
    delay(200);
    
    // Final state: Set all to default loaded state
    Serial.println("Teensy: Setting pads to default loaded state - Orange");
    for (int i = 0; i < TOTAL_KEYS; i++) {
        setPixelColor(i, COLOR_LOADED); // Orange
        delay(20);
    }
    
    // Animation complete - no logs to keep Serial clean
}

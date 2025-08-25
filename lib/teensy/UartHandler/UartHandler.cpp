#include "UartHandler.h"
#include "MidiCommands.h"
#include "shared/Config.h"
#include "../../Trellis/Trellis.h"

extern Trellis trellis;

void UartHandler::begin() {
    // Serial1 on Teensy 4.1: Pin 0 (RX1), Pin 1 (TX1)
    // Connect to NeoTrellis M4: Pin 22 (SCL/TX) -> Teensy Pin 0 (RX1)
    //                          Pin 21 (SDA/RX) -> Teensy Pin 1 (TX1)
    Serial1.begin(UART_BAUD_RATE);  // hardware UART at configured speed
    rxIndex = 0;
    messageComplete = false;
    lastPingMs = millis();
    lastSeenMs = millis();
    
    Serial.print("Teensy 4.1: UART Handler initialized @ ");
    Serial.print(UART_BAUD_RATE);
    Serial.println(" bps");
    Serial.println("Serial1 Pins: RX1=0, TX1=1 (Hardware UART)");
    Serial.print("Baud Rate: "); Serial.println(UART_BAUD_RATE);
    Serial.println("Connection: NeoTrellis M4 Pin 22 (SCL/TX) -> Teensy Pin 0 (RX1)");
    Serial.println("Connection: NeoTrellis M4 Pin 21 (SDA/RX) -> Teensy Pin 1 (TX1)");
}

void UartHandler::read() {
    // Disable automatic heartbeat ping to prevent animations
    // Connection status will be managed through handshake and data flow
    unsigned long now = millis();
    
    // Only check timeout (much longer interval)
    if (now - lastSeenMs > UART_LINK_TIMEOUT_MS) {
        // No UART activity recently; mark link down
        trellis.setM4Connected(false);
    }

    while (Serial1.available()) {
        uint8_t byte = Serial1.read();
        lastSeenMs = millis();
        
        if (byte == 0xF0) { // Start of message
            rxIndex = 0;
            rxBuffer[rxIndex++] = byte;
        } else if (byte == 0xF7) { // End of message
            if (rxIndex < BUFFER_SIZE) {
                rxBuffer[rxIndex++] = byte;
                messageComplete = true;
                parseMessage();
            } else {
                Serial.println("Teensy: UART buffer overflow, message dropped");
                rxIndex = 0;
            }
        } else if (rxIndex < BUFFER_SIZE - 1) { // Leave space for F7
            rxBuffer[rxIndex++] = byte;
        } else {
            Serial.println("Teensy: UART message too long, dropping");
            rxIndex = 0;
        }
    }
}

void UartHandler::parseMessage() {
    if (rxIndex < 4) return; // Invalid message
    // Support both simple and full SysEx frames.
    // Simple: F0 CMD [DATA...] F7
    // Full:   F0 7D 00 CMD [DATA...] F7
    uint8_t command = 0;
    uint8_t* data = nullptr;
    int dataLength = 0;

    if (rxIndex >= 5 && rxBuffer[1] == MANUFACTURER_ID && rxBuffer[2] == DEVICE_ID) {
        command = rxBuffer[3];
        data = &rxBuffer[4];
        dataLength = rxIndex - 5;
    } else {
        command = rxBuffer[1];
        data = &rxBuffer[2];
        dataLength = rxIndex - 3;
    }

    handleNeoTrellisCommand(command, data, dataLength);
    messageComplete = false;
}

void UartHandler::handleNeoTrellisCommand(uint8_t command, uint8_t* data, int length) {
    if (command != CMD_PING) {
        Serial.print("Teensy: Received UART command 0x");
        Serial.print(command, HEX);
        Serial.print(" with ");
        Serial.print(length);
        Serial.println(" bytes");
    }
    
    switch (command) {
        case CMD_CLIP_TRIGGER:
            // Forward clip trigger to USB MIDI for Ableton
            if (length >= 2) {
                uint8_t track = data[0];
                uint8_t scene = data[1];
                Serial.print("Teensy: Forwarding clip trigger - Track: ");
                Serial.print(track);
                Serial.print(", Scene: ");
                Serial.println(scene);
                
                // Send to Ableton via USB MIDI using your command structure
                uint8_t msg[7] = {SYSEX_START, MANUFACTURER_ID, DEVICE_ID, CMD_CLIP_TRIGGER, track, scene, SYSEX_END};
                usbMIDI.sendSysEx(7, msg);
            }
            break;
            
        case CMD_PLAY:
        case CMD_STOP:
        case CMD_RECORD:
        case CMD_LOOP:
            // Forward transport commands to USB MIDI
            {
                Serial.print("Teensy: Forwarding transport command: 0x");
                Serial.println(command, HEX);
                
                uint8_t msg[5] = {SYSEX_START, MANUFACTURER_ID, DEVICE_ID, command, SYSEX_END};
                usbMIDI.sendSysEx(5, msg);
            }
            break;
            
        case CMD_NAV_TRACK_LEFT:
        case CMD_NAV_TRACK_RIGHT:
        case CMD_NAV_SCENE_UP:
        case CMD_NAV_SCENE_DOWN:
            // Forward navigation commands to USB MIDI
            {
                Serial.print("Teensy: Forwarding navigation command: 0x");
                Serial.println(command, HEX);
                
                uint8_t msg[5] = {SYSEX_START, MANUFACTURER_ID, DEVICE_ID, command, SYSEX_END};
                usbMIDI.sendSysEx(5, msg);
            }
            break;
            
        case CMD_HANDSHAKE:
            Serial.print("Teensy: Handshake received from M4 (");
            Serial.print(length);
            Serial.print(" bytes) - ");
            
            // Print received handshake data for debugging
            for (int i = 0; i < length && i < 16; i++) {
                Serial.print((char)data[i]);
            }
            Serial.println();
            
            // Dynamic response - echo back whatever was received
            Serial.println("Teensy: Sending handshake echo response to M4");
            sendToNeoTrellis(CMD_HANDSHAKE, data, length);
            // Mark UART link as established on Teensy side
            trellis.setM4Connected(true);
            break;
        case CMD_PING:
            // Heartbeat received; mark link alive (no echo to avoid ping-pong)
            trellis.setM4Connected(true);
            break;
        
        default:
            Serial.print("Teensy: Unknown UART command: 0x");
            Serial.println(command, HEX);
            break;
    }
}

void UartHandler::sendToNeoTrellis(uint8_t command, uint8_t* data, int length) {
    // Send full SysEx over UART: F0 7D 00 CMD ... F7
    Serial1.write((uint8_t)0xF0);
    Serial1.write((uint8_t)MANUFACTURER_ID);
    Serial1.write((uint8_t)DEVICE_ID);
    Serial1.write(command);
    if (data && length > 0) {
        Serial1.write(data, length);
    }
    Serial1.write((uint8_t)0xF7);
}

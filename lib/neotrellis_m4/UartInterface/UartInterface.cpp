#include "UartInterface.h"
#include "shared/Config.h"
#include "MidiCommands.h"
#include "NeoTrellisController.h"

extern NeoTrellisController controller;

// Use built-in Serial1 (SERCOM4) on pins 22 (RX) and 21 (TX)

void UartInterface::begin() {
    Serial.println("=== NeoTrellis M4 UART Configuration ===");
    Serial.println("Using built-in Serial1 (SERCOM4) on pins 21/22");
    
    Serial.print("Initializing UART @ ");
    Serial.print(UART_BAUD_RATE);
    Serial.println(" bps");
    
    // Initialize built-in Serial1
    Serial1.begin(UART_BAUD_RATE);
    
    // Small delay to ensure initialization
    delay(200);
    
    rxIndex = 0;
    messageComplete = false;
    everReceived = false;
    beginMs = millis();
    
    Serial.println("NeoTrellis M4: Serial1 initialized successfully!");
    Serial.println("Pin configuration:");
    Serial.println("  Pin 21 (SDA/JST) -> UART TX (to Teensy RX1)");
    Serial.println("  Pin 22 (SCL/JST) -> UART RX (from Teensy TX1)");
    Serial.println("Expected connections:");
    Serial.println("  Teensy Pin 1 (TX1) -> M4 Pin 22 (RX)");
    Serial.println("  Teensy Pin 0 (RX1) -> M4 Pin 21 (TX)");
    Serial.println("  GND -> GND");
    Serial.println("Waiting for data from Teensy (Serial1)...");
}

void UartInterface::read() {
    // Add periodic debugging to check if SERCOM UART is receiving anything
    static unsigned long lastDebugTime = 0;
    static bool debugPrinted = false;
    
    if (!debugPrinted && (millis() - lastDebugTime > 5000)) {
        Serial.print("NeoTrellis M4: Serial1 status - Available: ");
        Serial.print(Serial1.available());
        Serial.println(" bytes (Checking for Teensy data via pins 21/22)");
        lastDebugTime = millis();
        debugPrinted = (millis() > 30000); // Stop debug after 30 seconds
    }
    
    while (Serial1.available()) {
        if (!everReceived) {
            Serial.println("NeoTrellis M4: First SERCOM UART byte received from Teensy!");
            everReceived = true;
        }
        
        uint8_t byte = Serial1.read();
        Serial.print("NeoTrellis M4: Received byte: 0x");
        Serial.println(byte, HEX);

        if (byte == 0xF0) { // Start of message
            rxIndex = 0;
            rxBuffer[rxIndex++] = byte;
        } else if (byte == 0xF7) { // End of message
            if (rxIndex < BUFFER_SIZE) {
                rxBuffer[rxIndex++] = byte;
                messageComplete = true;
                parseMessage();
            } else {
                Serial.println("NeoTrellis M4: UART buffer overflow, message dropped");
                rxIndex = 0;
            }
        } else if (rxIndex < BUFFER_SIZE - 1) { // Leave space for F7
            rxBuffer[rxIndex++] = byte;
        } else {
            Serial.println("NeoTrellis M4: UART message too long, dropping");
            rxIndex = 0;
        }
    }
}

void UartInterface::parseMessage() {
    if (rxIndex < 4) return; // Invalid message

    // Check if this is a full SysEx message (F0 MFG DEV CMD ... F7)
    if (rxIndex >= 5 && rxBuffer[1] == MANUFACTURER_ID && rxBuffer[2] == DEVICE_ID) {
        // This is a complete SysEx message from Ableton (via Teensy)
        processSysExFromTeensy(rxBuffer, rxIndex);
    } else {
        // This is a simple command message (F0 CMD DATA F7)
        uint8_t command = rxBuffer[1];
        uint8_t* data = &rxBuffer[2];
        int dataLength = rxIndex - 3; // Exclude F0, command, F7

        handleTeensyCommand(command, data, dataLength);
    }

    messageComplete = false;
}

void UartInterface::handleTeensyCommand(uint8_t command, uint8_t* data, int length) {
    switch (command) {
        case CMD_HANDSHAKE:
            Serial.print("NeoTrellis M4: Handshake response received from Teensy (");
            Serial.print(length);
            Serial.print(" bytes) - ");
            
            // Print response for debugging
            for (int i = 0; i < length && i < 16; i++) {
                Serial.print((char)data[i]);
            }
            Serial.println();
            
            // Any valid response means successful handshake
            if (length > 0) {
                Serial.println("NeoTrellis M4: UART connection established successfully!");
                // Flash LEDs to indicate successful connection
                controller.testAllPads();
            } else {
                Serial.println("NeoTrellis M4: Empty handshake response");
            }
            break;
        case CMD_PING:
            // Echo ping back to Teensy
            sendToTeensy(CMD_PING, nullptr, 0);
            break;
        case CMD_LED_RGB_STATE:
            if (length >= 4) {
                int padIndex = data[0];
                uint8_t r = data[1];
                uint8_t g = data[2];
                uint8_t b = data[3];
                uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
                controller.setPixelColor(padIndex, color);
                Serial.print("NeoTrellis M4: Set RGB pad ");
                Serial.print(padIndex);
                Serial.print(" -> ");
                Serial.print(r);
                Serial.print(",");
                Serial.print(g);
                Serial.print(",");
                Serial.println(b);
            }
            break;
        case CMD_LED_CLIP_STATE:
            if (length >= 2) {
                int padIndex = data[0];
                int state = data[1];
                controller.updateClipState(padIndex, state);

                Serial.print("NeoTrellis M4: Updating pad ");
                Serial.print(padIndex);
                Serial.print(" to state ");
                Serial.println(state);
            }
            break;

        case CMD_LED_TRANSPORT_STATE:
            Serial.println("NeoTrellis M4: Transport state update received");
            break;

        case CMD_LED_DEVICE_STATE:
            Serial.println("NeoTrellis M4: Device state update received");
            break;

        default:
            Serial.print("NeoTrellis M4: Unknown command received: 0x");
            Serial.println(command, HEX);
            break;
    }
}

void UartInterface::processSysExFromTeensy(uint8_t* data, int length) {
    if (length < 5 || data[0] != 0xF0 || data[length - 1] != 0xF7) return;
    if (data[1] != MANUFACTURER_ID || data[2] != DEVICE_ID) return;

    uint8_t command = data[3];
    uint8_t* payload = &data[4];
    int payloadLength = length - 5; // Exclude F0, MFG, DEV, CMD, F7

    Serial.print("NeoTrellis M4: Processing SysEx command 0x");
    Serial.print(command, HEX);
    Serial.print(" with ");
    Serial.print(payloadLength);
    Serial.println(" bytes");

    handleTeensyCommand(command, payload, payloadLength);
}

void UartInterface::sendToTeensy(uint8_t command, uint8_t* data, int length) {
    // Send full SysEx framing via Serial1 (pins 21/22): F0 7D 00 CMD ... F7
    Serial1.write((uint8_t)0xF0);
    Serial1.write((uint8_t)MANUFACTURER_ID);
    Serial1.write((uint8_t)DEVICE_ID);
    Serial1.write(command);
    if (data && length > 0) {
        Serial1.write(data, length);
    }
    Serial1.write((uint8_t)0xF7);
    Serial1.flush(); // Ensure data is sent immediately

    Serial.print("NeoTrellis M4: Sent command 0x");
    Serial.print(command, HEX);
    Serial.print(" with ");
    Serial.print(length);
    Serial.print(" bytes via SERCOM UART (pins 21/22)");
    Serial.print(" - Full SysEx bytes: ");
    Serial.println(length + 5);
}

// No custom pin mux needed; Serial1 handles pin configuration per variant mapping

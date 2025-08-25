#include <Arduino.h>
// Include wrappers from lib to ensure PlatformIO links private libs
#include "NeoTrellisController/NeoTrellisController.h"
#include "UartInterface/UartInterface.h"
#include "MidiCommands.h"

NeoTrellisController controller;
UartInterface uartInterface;

void setup() {
    // Force serial even if USB CDC is not defined
    Serial.begin(115200);
    delay(3000); // Wait for serial connection
    
    Serial.println("M4 STARTUP - TEST 1");
    Serial.println("M4 STARTUP - TEST 2"); 
    Serial.println("M4 STARTUP - TEST 3");
    Serial.println("=== Push Clone - NeoTrellis M4 ===");
    Serial.println("UART Communication with Teensy 4.1");
    Serial.println("Ableton Live Integration Ready");
    
    Serial.println("M4: Starting controller.begin()...");
    controller.begin();
    
    Serial.println("M4: Starting uartInterface.begin()...");
    uartInterface.begin();
    
    // Wait for UART to stabilize
    delay(500);
    
    Serial.println("NeoTrellis M4 initialized successfully!");
    Serial.println("Initiating handshake with Teensy...");
    
    // Send handshake to Teensy
    uint8_t handshakeData[] = {0x50, 0x55, 0x53, 0x48, 0x43, 0x4C, 0x4F, 0x4E, 0x45}; // "PUSHCLONE"
    uartInterface.sendToTeensy(CMD_HANDSHAKE, handshakeData, 9);
    
    Serial.println("Handshake sent to Teensy");
    Serial.println("32-pad grid active for clip launching");
}

void loop() {
    controller.read();
    uartInterface.read();
    delay(1);
}

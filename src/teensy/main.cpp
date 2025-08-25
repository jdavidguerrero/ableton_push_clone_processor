#include <Arduino.h>
#include "Hardware.h"
#include "Trellis.h"
#include "MidiCommands.h"
#include "shared/Config.h"

// Declare external reference to global instances
extern Trellis trellis;


// This function is unused and has been removed.

// MIDI Test code removed.
void setup() {
    Serial.begin(115200);
    delay(2000); // Give time for serial monitor to connect
    
    Serial.println("TEENSY STARTUP - TEST 1");
    Serial.println("TEENSY STARTUP - TEST 2");
    Serial.println("TEENSY STARTUP - TEST 3");
    Serial.println("=== Push Clone - Teensy 4.1 ===");
    Serial.println("USB MIDI to Ableton Live Integration Ready");
    Serial.println("Serial debug logging enabled (does not interfere with USB MIDI)");
    
    // Initialize Serial1 for M4 communication
    Serial1.begin(UART_BAUD_RATE); // UART for M4
    Serial.print("Serial1 (UART to M4) initialized @ ");
    Serial.print(UART_BAUD_RATE);
    Serial.print(" bps");
    Serial.println(" (TX1=Pin 1, RX1=Pin 0)");

    delay(1000);
    
    setupHardware();
    
    // Step 1: Initialize M4 UART communication
    Serial.println("\n=== Step 1: M4 Setup ===");
    trellis.initializeM4Communication();
    
    if (trellis.isM4Connected()) {
        Serial.println("✓ M4 Ready");
        delay(500);
        trellis.testAllPads();
    } else {
        Serial.println("✗ M4 Failed");
    }
    
    // Step 2: Wait before connecting to Live (let hardware settle)
    Serial.println("\n=== Step 2: System Stabilization ===");
    Serial.println("Allowing hardware connections to stabilize...");
    delay(2000); // 2 second pause for all hardware to be ready
    
    Serial.println("\n=== Step 3: USB MIDI Ready ===");
    Serial.println("System ready - now open Live and watch the logs...");
    Serial.println("Note: Live integration will start automatically when Live sends handshake");
}

void loop() {
    // Process MIDI messages based on system state - DO THIS FIRST AND FREQUENTLY
    if (trellis.isLiveConnected()) {
        // Full operation mode - process all MIDI
        trellis.processMIDI();
    } else {
        // Waiting for Live - check for handshake MORE FREQUENTLY
        trellis.waitForLiveHandshake();
    }
    
    // Debug: Check USB MIDI availability less frequently
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        Serial.print("Loop: Status - Live connected: ");
        Serial.println(trellis.isLiveConnected() ? "YES" : "NO");
        lastCheck = millis();
    }
    
    loopHardware();
    // NO DELAY - process MIDI as fast as possible to catch handshakes
}

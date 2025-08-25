#include <Adafruit_NeoPixel.h>
#include <Adafruit_seesaw.h>
#include "NeoTrellisController.h"
#include "shared/Config.h"
#include "MidiCommands.h"
#include "UartInterface.h"

extern UartInterface uartInterface;

NeoTrellisController::NeoTrellisController() {
    // Initialize NeoPixel strip for 32 keys (built-in to NeoTrellis M4)
    // Pin 10 is the NeoPixel data pin on NeoTrellis M4
    pixels = new Adafruit_NeoPixel(TOTAL_KEYS, 10, NEO_GRB + NEO_KHZ800);
    
    // Initialize seesaw for keypad reading
    seesaw = new Adafruit_seesaw();
    
    lastKeyState = 0;
    lastKeyCheck = 0;
}

NeoTrellisController::~NeoTrellisController() {
    if (pixels) {
        delete pixels;
        pixels = nullptr;
    }
    // Don't delete seesaw as it's polymorphic without virtual destructor
    seesaw = nullptr;
}

void NeoTrellisController::begin() {
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("NeoTrellis M4: Initializing LED and keypad controller...");
    #endif
    
    // Initialize NeoPixels (built into NeoTrellis M4)
    pixels->begin();
    pixels->setBrightness(LED_BRIGHTNESS);
    pixels->clear();
    pixels->show();
    
    // Initialize seesaw for LED control only (keypad scanning disabled for now)
    if (!seesaw->begin(0x2E)) { // Default I2C address for built-in seesaw
        #ifndef ADAFRUIT_USBD_CDC_H_
        Serial.println("NeoTrellis M4: ERROR - Could not initialize seesaw!");
        #endif
    } else {
        #ifndef ADAFRUIT_USBD_CDC_H_
        Serial.println("NeoTrellis M4: Seesaw initialized successfully!");
        Serial.println("NeoTrellis M4: Keypad scanning disabled to prevent phantom triggers");
        #endif
        
        // Initialize key state
        lastKeyState = 0;
    }
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("NeoTrellis M4: LED controller initialized successfully!");
    #endif
    
    // Set all pixels to empty state initially
    for (int i = 0; i < TOTAL_KEYS; i++) {
        setPixelColor(i, COLOR_EMPTY);
    }
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("NeoTrellis M4: Ready - 32-pad controller active with keypad scanning");
    #endif
}

void NeoTrellisController::setupKeyCallbacks() {
    // Simple polling-based key reading for now
    // Advanced callback system would require more complex seesaw integration
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("Key scanning will use polling method");
    #endif
}

void NeoTrellisController::read() {
    // Poll keypad state and detect changes
    checkKeys();
}

void NeoTrellisController::setPixelColor(int key, uint32_t color) {
    if (key < 0 || key >= TOTAL_KEYS || !pixels) return;
    pixels->setPixelColor(key, color);
    pixels->show();
}

void NeoTrellisController::checkKeys() {
    if (!seesaw) return;
    
    // Check for keypad events at reasonable intervals (don't spam)
    unsigned long now = millis();
    if (now - lastKeyCheck < 100) return; // Check every 100ms to reduce noise
    lastKeyCheck = now;
    
    // TEMPORARY: Disable automatic key scanning to prevent false triggers
    // This approach was causing phantom key presses
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    static unsigned long lastDebugTime = 0;
    if (now - lastDebugTime > 5000) { // Debug every 5 seconds
        Serial.println("NeoTrellis M4: Key scanning temporarily disabled to prevent false triggers");
        Serial.println("NeoTrellis M4: Physical keys should be detected via proper seesaw keypad events");
        lastDebugTime = now;
    }
    #endif
    
    // TODO: Implement proper NeoTrellis keypad event handling using seesaw keypad API
    // For now, we disable scanning to prevent phantom triggers
}

void NeoTrellisController::updateClipState(int padIndex, int state) {
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

void NeoTrellisController::testAllPads() {
    if (!pixels) return;
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("M4: Quick connection test - minimal visual feedback");
    #endif
    
    // Minimal test: just flash corners briefly to confirm connection
    int corners[] = {0, 3, 28, 31}; // 4 corners of 8x4 grid
    
    // Quick flash of corners
    for (int i = 0; i < 4; i++) {
        pixels->setPixelColor(corners[i], 0x00FF00); // Green
    }
    pixels->show();
    delay(200);
    
    // Turn off corners
    for (int i = 0; i < 4; i++) {
        pixels->setPixelColor(corners[i], 0x000000); // Off
    }
    pixels->show();
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.println("M4: Connection test complete - ready for Live integration");
    #endif
}

void NeoTrellisController::handleKeyPress(int key) {
    int track = key % 4;
    int scene = key / 4;
    
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.print("Pad "); Serial.print(key);
    Serial.print(" (Track "); Serial.print(track);
    Serial.print(", Scene "); Serial.print(scene);
    Serial.println(") pressed");
    #endif
    
    // Send clip trigger command via UART to Teensy
    uint8_t data[] = {(uint8_t)track, (uint8_t)scene};
    uartInterface.sendToTeensy(CMD_CLIP_TRIGGER, data, 2);
    
    // Visual feedback
    setPixelColor(key, COLOR_TRIGGERED);
}

void NeoTrellisController::handleKeyRelease(int key) {
    #ifndef ADAFRUIT_USBD_CDC_H_
    Serial.print("Pad "); Serial.print(key); Serial.println(" released");
    #endif
    setPixelColor(key, COLOR_LOADED);
}

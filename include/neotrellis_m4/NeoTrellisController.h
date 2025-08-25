#pragma once

// Forward declarations to avoid requiring the full library headers here
class Adafruit_NeoPixel;
class Adafruit_seesaw;
#include "shared/Config.h"

class NeoTrellisController {
public:
    NeoTrellisController();
    ~NeoTrellisController();
    void begin();
    void read();
    void setPixelColor(int key, uint32_t color);
    void updateClipState(int padIndex, int state);
    void testAllPads();
    
private:
    Adafruit_NeoPixel* pixels;
    Adafruit_seesaw* seesaw;
    uint32_t lastKeyState;
    unsigned long lastKeyCheck;
    
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    void setupKeyCallbacks();
    void checkKeys();
};

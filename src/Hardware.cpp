#include "Hardware.h"
#include "Trellis.h"
// #include "Encoders.h"
// #include "Faders.h"
// #include "Neopixels.h"
// #include "Piezo.h"
// #include "Theremin.h"
// #include "CapButtons.h"
#include "MidiHandler.h"

// Create instances of all our hardware classes
Trellis trellis;
// Encoders encoders;
// Faders faders;
// Neopixels neopixels;
// Piezo piezo;
// Theremin theremin;
// CapButtons capButtons;
MidiHandler midiHandler;

void setupHardware() {
    midiHandler.begin();
    trellis.begin();
    // encoders.begin();
    // faders.begin();
    // neopixels.begin();
    // piezo.begin();
    // theremin.begin();
    // capButtons.begin();
    
}

void loopHardware() {
    midiHandler.processMidiMessages();
    trellis.read();
    // encoders.read();
    // faders.read();
    // piezo.read();
    // theremin.read();
    // capButtons.read();
    // neopixels.update();
}

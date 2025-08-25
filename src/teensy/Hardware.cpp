#include "../../include/teensy/Hardware.h"
#include "../../lib/Trellis/Trellis.h"
#include "../../lib/teensy/UartHandler/UartHandler.h"
// #include "Encoders.h"
// #include "Faders.h"
// #include "Neopixels.h"
// #include "Piezo.h"
// #include "Theremin.h"
// #include "CapButtons.h"
#include "../../lib/MidiHandler/MidiHandler.h"

// Create instances of all our hardware classes
Trellis trellis;
UartHandler uartHandler;
// Encoders encoders;
// Faders faders;
// Neopixels neopixels;
// Piezo piezo;
// Theremin theremin;
// CapButtons capButtons;
MidiHandler midiHandler;

void setupHardware() {
    uartHandler.begin();
    trellis.begin();
    // encoders.begin();
    // faders.begin();
    // neopixels.begin();
    // piezo.begin();
    // theremin.begin();
    // capButtons.begin();
    midiHandler.begin();
}

void loopHardware() {
    uartHandler.read();
    trellis.processMIDI();  // Process USB MIDI from Ableton and forward to NeoTrellis M4
    midiHandler.processMidiMessages();
    trellis.read();
    // encoders.read();
    // faders.read();
    // piezo.read();
    // theremin.read();
    // capButtons.read();
    // neopixels.update();
}

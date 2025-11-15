#include "../../include/teensy/Hardware.h"
#include "LiveController/LiveController.h"
#include "../../lib/teensy/UartHandler/UartHandler.h"
#include "../../lib/teensy/UIBridge.h"
#include "../../lib/teensy/NeoTrellisLink/NeoTrellisLink.h"
#include "../../lib/teensy/GUIInterface/GUIInterface.h"
// #include "Encoders.h"
// #include "Faders.h"
// #include "Neopixels.h"
// #include "Piezo.h"
// #include "Theremin.h"
// #include "CapButtons.h"
#include "../../lib/MidiHandler/MidiHandler.h"

// Create instances of all our hardware classes
LiveController liveController;
UartHandler uartHandler;
UIBridge uiBridge;
NeoTrellisLink neoTrellisLink;
GUIInterface guiInterface;
// Encoders encoders;
// Faders faders;
// Neopixels neopixels;
// Piezo piezo;
// Theremin theremin;
// CapButtons capButtons;
MidiHandler midiHandler;

void setupHardware() {
    uartHandler.begin();
    uiBridge.begin();
    liveController.begin();
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
    neoTrellisLink.update();
    uiBridge.update();
    midiHandler.processMidiMessages();
    liveController.read();
    // encoders.read();
    // faders.read();
    // piezo.read();
    // theremin.read();
    // capButtons.read();
    // neopixels.update();
}

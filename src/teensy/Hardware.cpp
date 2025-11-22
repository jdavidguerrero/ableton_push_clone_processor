#include "../../include/teensy/Hardware.h"
#include "LiveController/LiveController.h"
#include "../../lib/teensy/UartHandler/UartHandler.h"
#include "../../lib/teensy/UIBridge.h"
#include "../../lib/teensy/NeoTrellisLink/NeoTrellisLink.h"
#include "../../lib/teensy/GUIInterface/GUIInterface.h"
#include "../../lib/ButtonManager/ButtonManager.h"
#include "../../include/MidiCommands.h"
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
ButtonManager buttonManager;

namespace {
constexpr uint8_t SHIFT_UI_PANEL = 0x0F;

void handleEncoderButtonPress(uint8_t encoderIndex) {
    // Map encoder 0-3 to scene 0-3
    uint8_t scene = encoderIndex;
    if (scene < GRID_SCENES) {
        liveController.sendSysExToAbleton(CMD_SCENE_FIRE, &scene, 1);
    }
}

void handleNavigationPress(uint8_t direction) {
    switch (direction) {
        case 0: uiBridge.shiftSessionRing(0, -1); break; // Up
        case 1: uiBridge.shiftSessionRing(0, 1);  break; // Down
        case 2: uiBridge.shiftSessionRing(-1, 0); break; // Left
        case 3: uiBridge.shiftSessionRing(1, 0);  break; // Right
    }
}

void handleTransportPress(ButtonID id) {
    switch (id) {
        case ButtonID::PLAY:    liveController.sendSysExToAbleton(CMD_TRANSPORT_PLAY, nullptr, 0); break;
        case ButtonID::STOP: {
            uint8_t off = 0;
            liveController.sendSysExToAbleton(CMD_TRANSPORT_PLAY, &off, 1);
            break;
        }
        case ButtonID::RECORD:  liveController.sendSysExToAbleton(CMD_TRANSPORT_RECORD, nullptr, 0); break;
        case ButtonID::LOOP:    liveController.sendSysExToAbleton(CMD_TRANSPORT_LOOP, nullptr, 0); break;
        default: break;
    }
}

void handleBankChange(int8_t direction) {
    // Move ring horizontally in groups of 4 tracks
    int deltaTracks = direction * GRID_TRACKS;
    uiBridge.shiftSessionRing(deltaTracks, 0);
}

void handleShiftChange(bool pressed) {
    guiInterface.sendUiState(SHIFT_UI_PANEL, pressed);
}
} // namespace

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
    buttonManager.onEncoderButtonPress = handleEncoderButtonPress;
    buttonManager.onNavigationPress = handleNavigationPress;
    buttonManager.onTransportPress = handleTransportPress;
    buttonManager.onBankChange = handleBankChange;
    buttonManager.onShiftChange = handleShiftChange;
    buttonManager.begin();
}

void loopHardware() {
    uartHandler.read();
    neoTrellisLink.update();
    uiBridge.update();
    midiHandler.processMidiMessages();
    liveController.read();
    buttonManager.update();
    // encoders.read();
    // faders.read();
    // piezo.read();
    // theremin.read();
    // capButtons.read();
    // neopixels.update();
}

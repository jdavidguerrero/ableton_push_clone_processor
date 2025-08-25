#include "CapButtons.h"
#include "MidiCommands.h"
#include <Arduino.h>
#include <MIDI.h>

extern midi::MidiInterface<midi::SerialMIDI<usb_midi_class>> MIDI;

void CapButtons::begin() {
    int pins[] = CAPACITIVE_PINS;
    for (int i = 0; i < NUM_CAPACITIVE_BUTTONS; i++) {
        pinMode(pins[i], INPUT);
        oldStates[i] = false;
    }
    lastReadTime = 0;
}

void CapButtons::read() {
    // Only read capacitive buttons every 20ms to avoid noise
    if (millis() - lastReadTime < 20) return;
    lastReadTime = millis();
    
    int pins[] = CAPACITIVE_PINS;
    for (int i = 0; i < NUM_CAPACITIVE_BUTTONS; i++) {
        bool currentState = touchRead(pins[i]) < 1000; // Threshold for touch detection
        
        if (currentState && !oldStates[i]) {
            handleButtonPress(i);
        } else if (!currentState && oldStates[i]) {
            handleButtonRelease(i);
        }
        
        oldStates[i] = currentState;
    }
}

void CapButtons::handleButtonPress(int button) {
    byte command = CMD_PLAY; // Default to play
    
    // Map buttons to transport and navigation commands
    switch (button) {
        case 0: command = CMD_PLAY; break;
        case 1: command = CMD_STOP; break;
        case 2: command = CMD_RECORD; break;
        case 3: command = CMD_LOOP; break;
        case 4: command = CMD_NAV_TRACK_PREV; break;
        case 5: command = CMD_NAV_TRACK_NEXT; break;
        case 6: command = CMD_NAV_SCENE_PREV; break;
        case 7: command = CMD_NAV_SCENE_NEXT; break;
    }
    
    byte data[] = {0xF0, command, 0xF7};
    MIDI.sendSysEx(sizeof(data), data, true);
}

void CapButtons::handleButtonRelease(int button) {
    // Nothing to do for button release for now
}
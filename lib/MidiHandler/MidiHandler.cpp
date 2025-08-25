#include "MidiHandler.h"
#include "MidiCommands.h"
#include "Hardware.h"
#include "Trellis.h"
#include <Arduino.h>

// Use Teensy's native USB MIDI
extern class Trellis trellis;

void MidiHandler::begin() {
    // No MIDI library setup needed for Teensy USB MIDI
    Serial.println("MidiHandler initialized for NeoTrellis only");
}

void MidiHandler::processMidiMessages() {
    // Process USB MIDI directly in Trellis class
    // No need to duplicate MIDI processing here
}

void MidiHandler::handleSysEx(uint8_t* data, unsigned int length) {
    if (length < 3) return; // Invalid SysEx message
    
    // Skip F0 (start) and check command
    byte command = data[1];
    
    switch (command) {
        case CMD_LED_CLIP_STATE:
            handleLedClipState(data, length);
            break;
        // Remove other cases for now - focus on NeoTrellis only
    }
}

void MidiHandler::handleLedClipState(uint8_t* data, unsigned int length) {
    if (length < 5) return; // Need at least F0, CMD, index, state, F7
    
    byte clipIndex = data[2];
    byte state = data[3];
    
    // Map clip states to colors using Config.h constants
    uint32_t color = COLOR_EMPTY; // Default dim
    switch (state) {
        case CLIP_STATE_EMPTY:    color = COLOR_EMPTY; break;
        case CLIP_STATE_LOADED:   color = COLOR_LOADED; break;
        case CLIP_STATE_PLAYING:  color = COLOR_PLAYING; break;
        case CLIP_STATE_RECORDING: color = COLOR_RECORDING; break;
        case CLIP_STATE_SELECTED: color = COLOR_SELECTED; break;
        case CLIP_STATE_TRIGGERED: color = COLOR_TRIGGERED; break;
    }
    
    if (clipIndex < 32) { // 8x4 grid
        trellis.setPixelColor(clipIndex, color);
        // setPixelColor already handles display update internally
    }
}
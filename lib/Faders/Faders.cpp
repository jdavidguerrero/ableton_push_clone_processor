#include "Faders.h"
#include "MidiCommands.h"
#include <Arduino.h>
#include <MIDI.h>

extern midi::MidiInterface<midi::SerialMIDI<usb_midi_class>> MIDI;

void Faders::begin() {
    int pins[] = FADER_PINS;
    for (int i = 0; i < NUM_FADERS; i++) {
        pinMode(pins[i], INPUT);
        oldValues[i] = -1;
    }
}

void Faders::read() {
    int pins[] = FADER_PINS;
    for (int i = 0; i < NUM_FADERS; i++) {
        int value = analogRead(pins[i]) / 8; // 10-bit to 7-bit
        if (abs(value - oldValues[i]) > 2) { // Add some tolerance
            byte data[] = {0xF0, CMD_MIXER_VOLUME, (byte)i, (byte)value, 0xF7};
            MIDI.sendSysEx(sizeof(data), data, true);
            oldValues[i] = value;
        }
    }
}

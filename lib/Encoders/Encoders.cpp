#include "Encoders.h"
#include "MidiCommands.h"
#include <MIDI.h>

extern midi::MidiInterface<midi::SerialMIDI<usb_midi_class>> MIDI;

void Encoders::begin() {
    int pins[] = ENCODER_PINS;
    for (int i = 0; i < NUM_ENCODERS; i++) {
        encoders[i] = new Encoder(pins[i * 2], pins[i * 2 + 1]);
        oldPositions[i] = -999;
    }
}

void Encoders::read() {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        long newPosition = encoders[i]->read();
        if (newPosition != oldPositions[i]) {
            int value = (newPosition - oldPositions[i]) > 0 ? 1 : 127;
            byte data[] = {0xF0, CMD_DEVICE_PARAMETER, (byte)i, (byte)value, 0xF7};
            MIDI.sendSysEx(sizeof(data), data, true);
            oldPositions[i] = newPosition;
        }
    }
}

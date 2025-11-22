#include "Encoders.h"
#include "MidiCommands.h"
#include <MIDI.h>
#include <Arduino.h>

extern midi::MidiInterface<midi::SerialMIDI<usb_midi_class>> MIDI;

Encoders::Encoders()
    : activeEncoders(NUM_ENCODERS_ACTIVE)
    , currentView(0)
    , shiftHeld(false)
    , onEncoderChange(nullptr)
{
    for (int i = 0; i < NUM_ENCODERS_MAX; i++) {
        encoders[i] = nullptr;
        oldPositions[i] = -999;
    }
}

void Encoders::begin() {
    int pins[] = ENCODER_PINS_ACTIVE;

    // Inicializar solo los encoders activos
    for (int i = 0; i < activeEncoders; i++) {
        encoders[i] = new Encoder(pins[i * 2], pins[i * 2 + 1]);
        oldPositions[i] = 0;
        Serial.printf("Encoder %d initialized (pins %d, %d)\n",
                     i + 1, pins[i * 2], pins[i * 2 + 1]);
    }

    Serial.printf("Encoders initialized: %d active of %d max\n",
                 activeEncoders, NUM_ENCODERS_MAX);
}

void Encoders::setActiveCount(uint8_t count) {
    if (count > NUM_ENCODERS_MAX) {
        count = NUM_ENCODERS_MAX;
    }

    // TODO: Si se aumenta el count, inicializar los nuevos encoders
    // Por ahora solo permitimos reducir
    if (count < activeEncoders) {
        for (int i = count; i < activeEncoders; i++) {
            if (encoders[i] != nullptr) {
                delete encoders[i];
                encoders[i] = nullptr;
            }
        }
    }

    activeEncoders = count;
    Serial.printf("Active encoders changed to: %d\n", activeEncoders);
}

void Encoders::read() {
    for (int i = 0; i < activeEncoders; i++) {
        if (encoders[i] == nullptr) continue;

        long newPosition = encoders[i]->read();
        if (newPosition != oldPositions[i]) {
            int delta = newPosition - oldPositions[i];

            // Aplicar control fino con Shift (incrementos 10x más pequeños)
            if (shiftHeld && abs(delta) > 1) {
                delta = delta / 10;
                if (delta == 0) delta = (newPosition > oldPositions[i]) ? 1 : -1;
            }

            sendEncoderChange(i, delta);
            oldPositions[i] = newPosition;
        }
    }
}

void Encoders::sendEncoderChange(uint8_t encoderIndex, int8_t delta) {
    // Llamar callback si está configurado
    if (onEncoderChange) {
        onEncoderChange(encoderIndex, delta);
        return;
    }

    // Comportamiento por defecto: enviar como MIDI relative
    // Valores MIDI: 1-63 = incrementos positivos, 65-127 = incrementos negativos
    // 64 = no change, 1 = +1, 127 = -1
    int value = (delta > 0) ? 1 : 127;

    // Por ahora enviar como CMD_DEVICE_PARAMETER (default)
    // En el futuro esto se mapeará dinámicamente según currentView
    byte data[] = {0xF0, CMD_DEVICE_PARAMETER, (byte)encoderIndex, (byte)value, 0xF7};
    MIDI.sendSysEx(sizeof(data), data, true);
}

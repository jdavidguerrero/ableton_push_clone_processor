#include "Faders.h"
#include "MidiCommands.h"
#include <Arduino.h>
#include <MIDI.h>

extern midi::MidiInterface<midi::SerialMIDI<usb_midi_class>> MIDI;

Faders::Faders()
    : currentBankOffset(0)
    , paramMode(FaderParamMode::VOLUME_PAN)
    , onVolumeChange(nullptr)
    , onPickupStateChange(nullptr)
{
    for (int i = 0; i < NUM_FADERS; i++) {
        oldValues[i] = -1;

        // Inicializar pickup state
        pickupStates[i].physicalValue = 0;
        pickupStates[i].targetValue = 0;
        pickupStates[i].hasPickedUp = false;
        pickupStates[i].assignedTrackIndex = i;  // Bank 0 por defecto
    }
}

void Faders::begin() {
    int pins[] = FADER_PINS;
    for (int i = 0; i < NUM_FADERS; i++) {
        pinMode(pins[i], INPUT);
        oldValues[i] = -1;
        Serial.printf("Fader %d initialized (pin A%d)\n", i + 1, pins[i] - A0);
    }

    Serial.printf("Faders initialized: %d faders in bank %d (tracks %d-%d)\n",
                 NUM_FADERS, currentBankOffset,
                 currentBankOffset, currentBankOffset + NUM_FADERS - 1);
}

void Faders::setTrackBank(int bankOffset) {
    if (bankOffset == currentBankOffset) return;

    Serial.printf("Fader bank change: %d → %d (tracks %d-%d)\n",
                 currentBankOffset, bankOffset,
                 bankOffset, bankOffset + NUM_FADERS - 1);

    currentBankOffset = bankOffset;

    // RESET pickup state para todos los faders
    for (int i = 0; i < NUM_FADERS; i++) {
        pickupStates[i].assignedTrackIndex = bankOffset + i;
        pickupStates[i].hasPickedUp = false;

        // Notificar GUI que necesita pickup
        if (onPickupStateChange) {
            onPickupStateChange(i, true);  // needsPickup = true
        }

        Serial.printf("  Fader %d → Track %d (needs pickup)\n",
                     i, pickupStates[i].assignedTrackIndex);
    }
}

void Faders::onTrackVolumeUpdate(int trackIndex, int volume) {
    // Actualizar target value si este track está en banco actual
    for (int i = 0; i < NUM_FADERS; i++) {
        if (pickupStates[i].assignedTrackIndex == trackIndex) {
            pickupStates[i].targetValue = volume;

            // Re-check pickup si cambió el target
            // (auto-pickup si el fader físico ya está cerca)
            if (!pickupStates[i].hasPickedUp) {
                int diff = abs(pickupStates[i].physicalValue - volume);
                if (diff <= FADER_PICKUP_THRESHOLD) {
                    pickupStates[i].hasPickedUp = true;

                    if (onPickupStateChange) {
                        onPickupStateChange(i, false);  // needsPickup = false
                    }

                    Serial.printf("AUTO-PICKUP: Fader %d caught track %d (diff=%d)\n",
                                 i, trackIndex, diff);
                }
            }

            break;
        }
    }
}

void Faders::read() {
    int pins[] = FADER_PINS;

    for (int i = 0; i < NUM_FADERS; i++) {
        // Leer ADC 12-bit y convertir a 7-bit MIDI (0-127)
        int rawValue = analogRead(pins[i]);
        int value = rawValue >> 5;  // Divide por 32 (4096/128 ≈ 32)

        // Tolerancia para evitar ruido
        if (abs(value - oldValues[i]) <= 2) {
            continue;  // No hay cambio significativo
        }

        pickupStates[i].physicalValue = value;

        // === PICKUP MODE LOGIC ===
        if (!pickupStates[i].hasPickedUp) {
            int diff = abs(value - pickupStates[i].targetValue);

            if (diff <= FADER_PICKUP_THRESHOLD) {
                // ✅ PICKUP ACHIEVED!
                pickupStates[i].hasPickedUp = true;

                if (onPickupStateChange) {
                    onPickupStateChange(i, false);  // needsPickup = false
                }

                Serial.printf("PICKUP: Fader %d (track %d) diff=%d | phys=%d target=%d\n",
                             i, pickupStates[i].assignedTrackIndex, diff,
                             value, pickupStates[i].targetValue);
            } else {
                // ❌ AÚN NO - Solo actualizar oldValues, NO enviar MIDI
                oldValues[i] = value;

                // Opcional: log para debug
                if (diff > 10) {  // Solo log si la diferencia es grande
                    Serial.printf("Fader %d waiting pickup: phys=%d target=%d (diff=%d)\n",
                                 i, value, pickupStates[i].targetValue, diff);
                }

                continue;  // CRITICAL: Skip MIDI send
            }
        }

        // Ya enganchó o está en tracking normal → Enviar MIDI
        sendVolumeCommand(pickupStates[i].assignedTrackIndex, value);
        oldValues[i] = value;
    }
}

void Faders::sendVolumeCommand(int trackIndex, int volume) {
    // Callback si está configurado
    if (onVolumeChange) {
        onVolumeChange(trackIndex, volume);
        return;
    }

    // Comportamiento por defecto: enviar SysEx
    byte data[] = {0xF0, CMD_MIXER_VOLUME, (byte)trackIndex, (byte)volume, 0xF7};
    MIDI.sendSysEx(sizeof(data), data, true);
}

void Faders::setParamMode(FaderParamMode mode) {
    if (paramMode == mode) return;

    Serial.printf("Fader param mode change: %d → %d\n",
                 static_cast<int>(paramMode), static_cast<int>(mode));

    paramMode = mode;

    // RESET pickup para todos los faders (cambió qué parámetro controlan)
    for (int i = 0; i < NUM_FADERS; i++) {
        pickupStates[i].hasPickedUp = false;

        if (onPickupStateChange) {
            onPickupStateChange(i, true);  // needsPickup = true
        }
    }
}

void Faders::onTrackParamUpdate(int trackIndex, uint8_t paramType, int value) {
    // TODO: Implementar pickup check para parámetros (Pan, Sends, etc.)
    // Similar a onTrackVolumeUpdate pero para otros parámetros
    Serial.printf("Track %d param %d updated: %d\n", trackIndex, paramType, value);
}

void Faders::sendParamCommand(int trackIndex, uint8_t paramType, int value) {
    // TODO: Enviar comando MIDI según paramMode
    // CMD_MIXER_PAN, CMD_MIXER_SEND, etc.
    Serial.printf("Sending param command: track=%d type=%d value=%d\n",
                 trackIndex, paramType, value);
}

#include "ButtonManager.h"
#include <Arduino.h>

ButtonManager::ButtonManager()
    : shiftPressed(false)
    , mappingCount(0)
    , onEncoderButtonPress(nullptr)
    , onEncoderButtonRelease(nullptr)
    , onNavigationPress(nullptr)
    , onTransportPress(nullptr)
    , onBankChange(nullptr)
{
    // Inicializar estados
    for (int i = 0; i < 32; i++) {
        buttonStates[i] = false;
        lastButtonStates[i] = false;
        lastDebounceTime[i] = 0;
    }
}

void ButtonManager::begin() {
    // Inicializar MCP #1 (Encoder buttons)
    if (!mcp[0].begin_I2C(MCP_ENCODER_BUTTONS_ADDR)) {
        Serial.println("ERROR: MCP23017 #1 (0x20) not found!");
        return;
    }

    // Inicializar MCP #2 (Extra buttons)
    if (!mcp[1].begin_I2C(MCP_EXTRA_BUTTONS_ADDR)) {
        Serial.println("ERROR: MCP23017 #2 (0x21) not found!");
        return;
    }

    // Configurar todos los pines como INPUT con PULLUP
    for (int i = 0; i < 16; i++) {
        mcp[0].pinMode(i, INPUT_PULLUP);
        mcp[1].pinMode(i, INPUT_PULLUP);
    }

    initializeMappings();

    Serial.println("ButtonManager initialized");
    Serial.printf("  MCP #1 (0x20): %d encoder buttons\n", NUM_ENCODER_BUTTONS);
    Serial.printf("  MCP #2 (0x21): %d extra buttons\n", NUM_EXTRA_BUTTONS);
    Serial.printf("  Total mappings: %d\n", mappingCount);
}

void ButtonManager::initializeMappings() {
    mappingCount = 0;

    // MCP #1 (0x20): Encoder buttons on GPA0-GPA3
    mappings[mappingCount++] = {0, 0, ButtonID::ENC_1, true};
    mappings[mappingCount++] = {0, 1, ButtonID::ENC_2, true};
    mappings[mappingCount++] = {0, 2, ButtonID::ENC_3, true};
    mappings[mappingCount++] = {0, 3, ButtonID::ENC_4, true};

    // MCP #2 (0x21): Function buttons on GPA0-GPA7
    mappings[mappingCount++] = {1, 0, ButtonID::PLAY, true};
    mappings[mappingCount++] = {1, 1, ButtonID::STOP, true};
    mappings[mappingCount++] = {1, 2, ButtonID::RECORD, true};
    mappings[mappingCount++] = {1, 3, ButtonID::LOOP, true};
    mappings[mappingCount++] = {1, 4, ButtonID::BANK_LEFT, true};
    mappings[mappingCount++] = {1, 5, ButtonID::BANK_RIGHT, true};
    mappings[mappingCount++] = {1, 6, ButtonID::SHIFT, true};
    mappings[mappingCount++] = {1, 7, ButtonID::METRONOME, true};

    // Futuro: Encoders 5-8 en MCP #1, GPA4-GPA7 (actualmente disabled)
    mappings[mappingCount++] = {0, 4, ButtonID::ENC_5, false};
    mappings[mappingCount++] = {0, 5, ButtonID::ENC_6, false};
    mappings[mappingCount++] = {0, 6, ButtonID::ENC_7, false};
    mappings[mappingCount++] = {0, 7, ButtonID::ENC_8, false};
}

void ButtonManager::update() {
    readMCP(0);  // Leer MCP #1
    readMCP(1);  // Leer MCP #2
}

void ButtonManager::readMCP(uint8_t mcpIndex) {
    for (uint8_t i = 0; i < mappingCount; i++) {
        ButtonMapping& mapping = mappings[i];

        // Solo procesar si pertenece a este MCP y está habilitado
        if (mapping.mcpIndex != mcpIndex || !mapping.enabled) {
            continue;
        }

        // Leer estado (LOW = pressed porque usamos PULLUP)
        bool currentState = !mcp[mcpIndex].digitalRead(mapping.gpioPin);

        uint8_t buttonIndex = i;  // Usar índice de mapping como índice de estado

        // Debouncing
        if (currentState != lastButtonStates[buttonIndex]) {
            lastDebounceTime[buttonIndex] = millis();
        }

        if ((millis() - lastDebounceTime[buttonIndex]) > BUTTON_DEBOUNCE_MS) {
            // El estado es estable
            if (currentState != buttonStates[buttonIndex]) {
                buttonStates[buttonIndex] = currentState;
                handleButtonChange(mapping.buttonID, currentState);
            }
        }

        lastButtonStates[buttonIndex] = currentState;
    }
}

void ButtonManager::handleButtonChange(ButtonID id, bool pressed) {
    // SHIFT button - delegate to callback, which will call setShiftState() if needed
    if (id == ButtonID::SHIFT) {
        Serial.printf("Shift button: %s\n", pressed ? "PRESSED" : "RELEASED");
        if (onShiftChange) {
            onShiftChange(pressed);
        }
        return;
    }

    // Solo procesar presses (no releases por ahora, excepto encoders)
    if (!pressed) {
        // Manejar release de encoders
        if (id >= ButtonID::ENC_1 && id <= ButtonID::ENC_4) {
            uint8_t encoderIndex = static_cast<uint8_t>(id);
            if (onEncoderButtonRelease) {
                onEncoderButtonRelease(encoderIndex);
            }
        }
        return;
    }

    // === PRESS HANDLING ===

    // Encoder buttons con Shift = Navegación
    if (shiftPressed && id >= ButtonID::ENC_1 && id <= ButtonID::ENC_4) {
        uint8_t direction;
        switch (id) {
            case ButtonID::ENC_1: direction = 0; break;  // UP
            case ButtonID::ENC_2: direction = 1; break;  // DOWN
            case ButtonID::ENC_3: direction = 2; break;  // LEFT
            case ButtonID::ENC_4: direction = 3; break;  // RIGHT
            default: return;
        }

        Serial.printf("Navigation: %s\n",
                     direction == 0 ? "UP" :
                     direction == 1 ? "DOWN" :
                     direction == 2 ? "LEFT" : "RIGHT");

        if (onNavigationPress) {
            onNavigationPress(direction);
        }
        return;
    }

    // Encoder buttons SIN Shift = Scene launch / Device select
    if (!shiftPressed && id >= ButtonID::ENC_1 && id <= ButtonID::ENC_4) {
        uint8_t encoderIndex = static_cast<uint8_t>(id);
        Serial.printf("Encoder %d button pressed\n", encoderIndex + 1);

        if (onEncoderButtonPress) {
            onEncoderButtonPress(encoderIndex);
        }
        return;
    }

    // Transport buttons
    if (id == ButtonID::PLAY || id == ButtonID::STOP ||
        id == ButtonID::RECORD || id == ButtonID::LOOP ||
        id == ButtonID::METRONOME) {

        const char* name =
            id == ButtonID::PLAY ? "PLAY" :
            id == ButtonID::STOP ? "STOP" :
            id == ButtonID::RECORD ? "RECORD" :
            id == ButtonID::LOOP ? "LOOP" : "METRONOME";

        Serial.printf("Transport: %s\n", name);

        if (onTransportPress) {
            onTransportPress(id);
        }
        return;
    }

    // Bank navigation buttons
    if (id == ButtonID::BANK_LEFT || id == ButtonID::BANK_RIGHT) {
        int8_t direction = (id == ButtonID::BANK_LEFT) ? -1 : 1;
        Serial.printf("Bank navigation: %s\n", direction < 0 ? "LEFT" : "RIGHT");

        if (onBankChange) {
            onBankChange(direction);
        }
        return;
    }
}

bool ButtonManager::isPressed(ButtonID id) {
    uint8_t idx = getMappingIndex(id);
    if (idx < mappingCount) {
        return buttonStates[idx];
    }
    return false;
}

uint8_t ButtonManager::getMappingIndex(ButtonID id) {
    for (uint8_t i = 0; i < mappingCount; i++) {
        if (mappings[i].buttonID == id) {
            return i;
        }
    }
    return 255;  // Not found
}

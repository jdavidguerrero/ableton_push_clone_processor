#pragma once

#include <Adafruit_MCP23X17.h>
#include "shared/Config.h"

// Enumeración de botones lógicos
enum class ButtonID : uint8_t {
    // Encoder push buttons (MCP #1 @ 0x20, GPA0-GPA3)
    ENC_1 = 0,
    ENC_2,
    ENC_3,
    ENC_4,

    // Extra function buttons (MCP #2 @ 0x21, GPA0-GPA7)
    PLAY = 10,
    STOP,
    RECORD,
    LOOP,
    BANK_LEFT,     // Navigate track/param banks left
    BANK_RIGHT,    // Navigate track/param banks right
    SHIFT,
    METRONOME,

    // Reservado para expansión futura (4 encoders adicionales)
    ENC_5 = 4,  // MCP #1, GPA4 (futuro)
    ENC_6,      // MCP #1, GPA5
    ENC_7,      // MCP #1, GPA6
    ENC_8,      // MCP #1, GPA7

    // Botones extra futuros (MCP #2, GPB0-GPB7)
    DELETE = 20,
    DUPLICATE,
    QUANTIZE,
    UNDO,
    REDO,
    // ... más botones futuros
};

struct ButtonMapping {
    uint8_t mcpIndex;    // 0 o 1 (qué MCP23017)
    uint8_t gpioPin;     // 0-15 (pin dentro del MCP)
    ButtonID buttonID;   // ID lógico del botón
    bool enabled;        // Si está mapeado actualmente
};

class ButtonManager {
public:
    ButtonManager();

    void begin();
    void update();

    // Estado de botones
    bool isPressed(ButtonID id);
    bool isShiftHeld() const { return shiftPressed; }

    // Callbacks (se deben configurar externamente)
    void (*onEncoderButtonPress)(uint8_t encoderIndex);
    void (*onEncoderButtonRelease)(uint8_t encoderIndex);
    void (*onNavigationPress)(uint8_t direction);  // 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
    void (*onTransportPress)(ButtonID id);
    void (*onBankChange)(int8_t direction);  // -1=left, +1=right
    void (*onShiftChange)(bool pressed);

private:
    Adafruit_MCP23X17 mcp[2];  // Dos MCPs

    bool buttonStates[32];                // Estado actual de todos los botones
    bool lastButtonStates[32];            // Estado anterior para detección de cambios
    unsigned long lastDebounceTime[32];   // Tiempo del último cambio para debouncing

    bool shiftPressed;                    // Estado del botón shift

    ButtonMapping mappings[24];           // Mapeo de botones activos
    uint8_t mappingCount;

    void initializeMappings();
    void handleButtonChange(ButtonID id, bool pressed);
    uint8_t getMappingIndex(ButtonID id);
    void readMCP(uint8_t mcpIndex);
};

#pragma once

#include "shared/Config.h"
#include <Arduino.h> // For byte type

class Trellis {
public:
    Trellis(); // Constructor
    ~Trellis(); // Destructor
    void begin();
    void read();
    void setPixelColor(int key, uint32_t color);
    void testAllPads(); // Test function to verify all pads work
    void processMIDI(); // Process incoming MIDI for LED feedback
    void updateClipState(int padIndex, int state); // Update pad LED based on Ableton state
    
    // MIDI utility functions
    void sendClipTrigger(byte track, byte scene);
    void sendTransportCommand(byte command);
    void sendSysExToAbleton(uint8_t command, uint8_t* data, int dataLength);
    void sendSysExToM4(uint8_t command, uint8_t* data, int dataLength);
    void initializeM4Communication(); // Initialize M4 UART communication
    void waitForLiveHandshake(); // Wait for Live to initiate handshake
    void sendUartConfirmationAnimation(); // Visual confirmation of UART communication
    void setM4Connected(bool connected); // Set UART connection state from handler
    
    // System state management
    bool isM4Connected() { return m4Connected; }
    bool isLiveConnected() { return liveConnected; }
    
private:
    // System state variables
    bool m4Connected = false;
    bool liveConnected = false;
    
    // SysEx buffer for handling fragmented messages
    static const int SYSEX_BUFFER_SIZE = 64;
    uint8_t sysexBuffer[SYSEX_BUFFER_SIZE];
    int sysexBufferIndex = 0;
    bool sysexInProgress = false;
    
    // All I2C-related member variables and functions have been removed
    // to match the UART-based architecture.
    void handleKeyPress(int board, int key);
    void handleKeyRelease(int board, int key);
    int getBoardFromKey(int globalKey);
    int getLocalKeyFromGlobal(int globalKey);
    void processSysEx(byte* data, int length);
    void processHandshakeMessage(uint8_t* data, int length);
};

#pragma once

// === HARDWARE CONFIGURATION ===
// Grid Layout: 8 tracks (columns) × 4 scenes (rows) = 32 pads
#define GRID_TRACKS 8   // 8 tracks (columns) - horizontal
#define GRID_SCENES 4   // 4 scenes (rows) - vertical  
#define TOTAL_KEYS (GRID_TRACKS * GRID_SCENES) // 32 keys total

// Physical NeoTrellis mapping (8x4 orientation)
#define Y_DIM GRID_SCENES // 4 rows (scenes)
#define X_DIM GRID_TRACKS // 8 columns (tracks)
#define I2C_START_ADDR 0x2E // Starting I2C address for NeoTrellis boards

// === MIDI CONFIGURATION ===
#define SYSEX_START 0xF0
#define SYSEX_END 0xF7
#define MANUFACTURER_ID 0x7F // Universal Non-Commercial (matches Live Remote Script)
#define DEVICE_ID 0x00       // Device channel (matches Live Remote Script)

// === LED BRIGHTNESS ===
#define LED_BRIGHTNESS 100 // 0-255

// === COLORS (RGB values) ===
#define COLOR_EMPTY 0x202020     // Dim white
#define COLOR_LOADED 0xFF4000    // Orange
#define COLOR_PLAYING 0x00FF00   // Green
#define COLOR_RECORDING 0xFF0000 // Red
#define COLOR_SELECTED 0x0080FF  // Blue
#define COLOR_TRIGGERED 0xFFFF00 // Yellow

// === UART COMMUNICATION ===
// Tuned for reliability: 1 Mbps over jumper pads (SDA/SCL)
#define UART_BAUD_RATE 1000000  // 1 Mbps - Reliable low-latency link
#define UART_BUFFER_SIZE 256
// Heartbeat ping from Teensy to M4
#define UART_PING_INTERVAL_MS 30000     // Send PING every 30 seconds
#define UART_LINK_TIMEOUT_MS 90000      // Consider link down after 90 seconds without RX

// === NEOTRELLIS M4 UART PINS (Using I2C jumper pins - ONLY available pins) ===
// NeoTrellis M4 only exposes I2C pins through jumper pads
#define UART_RX_PIN 21  // SDA pin (Pin 21) - only available pin for RX
#define UART_TX_PIN 22  // SCL pin (Pin 22) - only available pin for TX

// === ABLETON INTEGRATION ===
#define ABLETON_MIDI_CHANNEL 1
#define ABLETON_SYSEX_TIMEOUT 1000

// === DEBUG FLAGS ===
// #define DEBUG_LIVE_LOG  // Enable Live command logging (disabled to reduce spam)

// === FADERS CONFIGURATION (Teensy only) ===
#define NUM_FADERS 4
#define FADER_PINS {A0, A1, A2, A3}  // 4 ALPS B50K faders
#define TRACKS_PER_BANK 4              // Tracks visible per bank (matches fader count)
#define FADER_PICKUP_THRESHOLD 3       // ±3 MIDI units for pickup mode (2.4% tolerance)

// === ENCODERS CONFIGURATION (Teensy only) ===
#define NUM_ENCODERS_ACTIVE 4          // Currently connected encoders
#define NUM_ENCODERS_MAX 8             // Maximum encoders supported (future expansion)
// Format: {Enc1_A, Enc1_B, Enc2_A, Enc2_B, Enc3_A, Enc3_B, Enc4_A, Enc4_B}
// CORRECTED: Pin 6-9 (not 6-7)
#define ENCODER_PINS_ACTIVE {2, 3, 4, 5, 6, 9, 10, 11}
// Future expansion pins for encoders 5-8 (when added)
#define ENCODER_PINS_EXPANSION {12, 28, 24, 25, 26, 27, 33, 34}

// === MCP23017 I/O EXPANDER CONFIGURATION (Teensy only) ===
#define MCP_ENCODER_BUTTONS_ADDR 0x20  // MCP23017 #1: 4 encoder buttons
#define MCP_EXTRA_BUTTONS_ADDR 0x21    // MCP23017 #2: 8 extra buttons
#define NUM_ENCODER_BUTTONS 4          // Buttons on first MCP23017
#define NUM_EXTRA_BUTTONS 8            // Buttons on second MCP23017
#define TOTAL_MCP_BUTTONS (NUM_ENCODER_BUTTONS + NUM_EXTRA_BUTTONS)  // 12 total

// === BUTTON DEBOUNCE ===
#define BUTTON_DEBOUNCE_MS 50

// === ADC CONFIGURATION ===
#define ADC_RESOLUTION 12              // Teensy 4.1 supports 12-bit ADC
#define FADER_TOLERANCE 8              // Minimum change to trigger update (out of 4096)

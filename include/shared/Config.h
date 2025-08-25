#pragma once

// === HARDWARE CONFIGURATION ===
#define Y_DIM 8 // 8 rows
#define X_DIM 4 // 4 columns
#define TOTAL_KEYS (Y_DIM * X_DIM) // 32 keys total
#define I2C_START_ADDR 0x2E // Starting I2C address for NeoTrellis boards

// === MIDI CONFIGURATION ===
#define SYSEX_START 0xF0
#define SYSEX_END 0xF7
#define MANUFACTURER_ID 0x7D // Educational/Development use
#define DEVICE_ID 0x00

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
#define DEBUG_LIVE_LOG  // Enable Live command logging

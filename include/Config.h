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
#pragma once
#include "shared/Config.h"

// Ableton Live MIDI API Commands (SysEx) - Based on API documentation
// Manufacturer ID, Device ID, and SysEx delimiters are defined in shared/Config.h

// === HANDSHAKE / CONTROL (0x00-0x0F) ===
#define CMD_HANDSHAKE 0x01
#define CMD_HANDSHAKE_REPLY 0x02
#define CMD_PING      0x03

// === TRANSPORT COMMANDS (0x50-0x5F) ===
#define CMD_PLAY 0x50
#define CMD_STOP 0x51
#define CMD_RECORD 0x52
#define CMD_LOOP 0x53
#define CMD_METRONOME 0x54
#define CMD_TAP_TEMPO 0x55
#define CMD_NUDGE_UP 0x56
#define CMD_NUDGE_DOWN 0x57

// === CLIP COMMANDS (0x10-0x1F) ===
#define CMD_CLIP_TRIGGER 0x10
#define CMD_CLIP_STOP 0x11
#define CMD_CLIP_STOP_ALL 0x12
#define CMD_CLIP_DELETE 0x13
#define CMD_CLIP_DUPLICATE 0x14
#define CMD_CLIP_COLOR 0x15

// === CLIP STATES (for LED feedback) ===
#define CLIP_STATE_EMPTY 0x00
#define CLIP_STATE_LOADED 0x01
#define CLIP_STATE_PLAYING 0x02
#define CLIP_STATE_RECORDING 0x03
#define CLIP_STATE_SELECTED 0x04
#define CLIP_STATE_TRIGGERED 0x05

// === TRACK COMMANDS (0x20-0x2F) ===
#define CMD_TRACK_VOLUME 0x20
#define CMD_TRACK_PAN 0x21
#define CMD_TRACK_MUTE 0x22
#define CMD_TRACK_SOLO 0x23
#define CMD_TRACK_ARM 0x24
#define CMD_TRACK_SEND_A 0x25
#define CMD_TRACK_SEND_B 0x26
#define CMD_TRACK_SELECT 0x27

// === DEVICE COMMANDS (0x40-0x4F) ===
#define CMD_DEVICE_PARAMETER 0x40
#define CMD_DEVICE_ON_OFF 0x41
#define CMD_DEVICE_BANK 0x42
#define CMD_DEVICE_PRESET 0x43

// === NAVIGATION COMMANDS (0xB0-0xBF) ===
#define CMD_NAV_TRACK_LEFT 0xB0
#define CMD_NAV_TRACK_RIGHT 0xB1
#define CMD_NAV_SCENE_UP 0xB2
#define CMD_NAV_SCENE_DOWN 0xB3
#define CMD_NAV_SESSION_VIEW 0xB4
#define CMD_NAV_DETAIL_VIEW 0xB5

// === LED FEEDBACK COMMANDS (From Ableton to Controller) ===
#define CMD_LED_CLIP_STATE 0x80
#define CMD_LED_TRACK_STATE 0x81
#define CMD_LED_TRANSPORT_STATE 0x82
#define CMD_LED_DEVICE_STATE 0x83
#define CMD_LED_RGB_STATE 0x84

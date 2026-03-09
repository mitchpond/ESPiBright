#pragma once
#include <Arduino.h>
#include "RFTransmitter.h"

// ── ChannelState ──────────────────────────────────────────────────────────────
// Stores the current state of all three OptiBright channels and sends the
// channel command packet.
//
// Protocol encoding for the channel command packet (TYPE 0x06):
//   B3 = levelByte(white_on, white_level)
//   B4 = levelByte(blue_on,  blue_level)
//   B5 = rgbStateByte(rgb_on, rgb_color)
//   B6 = rgb_cycle  (0x01=static, 0x02=3s, 0x04=4s, 0x08=5s; 0x00 when off)
//   TYPE byte = (dominantLevel() << 4) | 0x06
//     low nibble 0x06 = command type "set all channels now"
//     high nibble     = RGB level when RGB on; else max(white,blue) level
//
// RGB color presets (B5 low nibble):
//   1=blue, 2=green, 3=white, 4=red, 5=orange,
//   6=purple, 7=pink, 8=yellow, 9=rainbow

class ChannelState {
public:
    // White channel
    bool    whiteOn    = true;
    uint8_t whiteLevel = 0x0A;   // 1–10

    // Blue channel
    bool    blueOn     = true;
    uint8_t blueLevel  = 0x0A;

    // RGB channel
    bool    rgbOn      = true;
    uint8_t rgbColor   = 0x06;   // default: purple
    uint8_t rgbCycle   = 0x01;   // 0x01=static
    uint8_t rgbLevel   = 0x0A;   // 1–10 (drives B7 high nibble when RGB dominant)

    explicit ChannelState(RFTransmitter& rf) : rf_(rf) {}

    // Build and transmit the channel command
    void send();

    // Convenience: encode current RGB state as a single byte (used by ScheduleState)
    uint8_t rgbStateByte() const;

    // Returns the level for the high nibble of the TYPE byte (channel command TYPE 0x06).
    // Priority: rgb_level if RGB on; white_level if white on; blue_level if blue on; 0xA if all off.
    // Combined with cmd type: typeByte = (dominantLevel() << 4) | 0x06
    uint8_t dominantLevel() const;

private:
    RFTransmitter& rf_;
};

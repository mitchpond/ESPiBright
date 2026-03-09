#pragma once
#include <Arduino.h>
#include "config.h"

// ── Protocol ──────────────────────────────────────────────────────────────────
// Stateless packet building for the OptiBright RF protocol.
// All packets are 8 bytes: 7-byte payload + 1-byte CRC.
// Bytes 0-1 are always 0xD0 0x23 (device address).
//
// Packet field layout:
//   [0]    = 0xD0
//   [1]    = 0x23
//   [2] B3 = channel/state data
//   [3] B4 = channel/state data
//   [4] B5 = RGB state / schedule data
//   [5] B6 = cycle speed (channel cmd) / 0x00 (schedule)
//   [6]    = TYPE byte
//              For channel commands (TYPE 0x06):
//                high nibble = dominant level (rgb_level if RGB on; else max(W,B))
//                low  nibble = 0x6 (command type: "set all channels now")
//              For all other types: full byte is the type identifier
//   [7]    = CRC (Maxim/Dallas 8-bit, poly 0x31)
//
// Level encoding (B3, B4 level nibble, TYPE high nibble): 0x1–0xa = 10%–100%

namespace Protocol {

    // Precompute the 256-entry lookup table for the Maxim/Dallas CRC-8 (poly 0x31).
    // Call once at startup; pass the resulting table to checksum() and buildPacket().
    inline void buildCrcTable(uint8_t* table, uint8_t poly = CRC_POLY) {
        for (int i = 0; i < 256; i++) {
            uint8_t crc = i;
            for (int j = 0; j < 8; j++)
                crc = (crc & 0x80) ? ((crc << 1) ^ poly) : (crc << 1);
            table[i] = crc;
        }
    }

    // Compute CRC-8 over a 7-byte payload using a precomputed table.
    inline uint8_t checksum(const uint8_t* table, const uint8_t* p7) {
        uint8_t crc = 0;
        for (int i = 0; i < 7; i++) crc = table[crc ^ p7[i]];
        return table[crc ^ 0x00];
    }

    // Append a CRC byte to a 7-byte payload, writing the complete 8-byte packet to out8.
    inline void buildPacket(const uint8_t* table, const uint8_t* p7, uint8_t* out8) {
        memcpy(out8, p7, 7);
        out8[7] = checksum(table, p7);
    }

    // Build a channel command packet (TYPE 0x06).
    // typeByte = (dominant_level << 4) | 0x06
    inline void buildChannelPacket(const uint8_t* table,
                                   uint8_t b3, uint8_t b4, uint8_t b5,
                                   uint8_t b6, uint8_t typeByte, uint8_t* out8) {
        uint8_t p7[7] = {PROTO_ADDR0, PROTO_ADDR1, b3, b4, b5, b6, typeByte};
        buildPacket(table, p7, out8);
    }

    // Convenience: build a schedule slot packet
    inline void buildSchedPacket(const uint8_t* table,
                                 uint8_t hh, uint8_t mm, uint8_t b5,
                                 uint8_t type, uint8_t* out8) {
        uint8_t p7[7] = {PROTO_ADDR0, PROTO_ADDR1, hh, mm, b5, 0x00, type};
        buildPacket(table, p7, out8);
    }

    // Encode a channel's on/off state and level (1–10) into a single byte.
    // Bit 7 = on flag; low nibble = level (1–10 → 0x1–0xA).
    inline uint8_t levelByte(bool on, uint8_t level) {
        return (on ? 0x80 : 0x00) | (level & 0x0F);
    }

    // Encode the RGB channel's on/off state and color preset into a single byte.
    // Bit 7 = on flag; low nibble = color index (see ChannelState.h for presets).
    inline uint8_t rgbStateByte(bool on, uint8_t color) {
        return (on ? 0x80 : 0x00) | (color & 0x0F);
    }

} // namespace Protocol

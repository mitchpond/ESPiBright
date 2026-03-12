#pragma once
#include <Arduino.h>
#include "RFTransmitter.h"

// ── ScheduleState ─────────────────────────────────────────────────────────────
// Stores the six schedule slots and builds/transmits the schedule packet burst.
//
// CONFIRMED FINAL sequence (×3): 03 → 04 → 05 → 07 → 08 → 09 → 02
//
//   02 = Blue  OFF   B5=0x00  (last — type number is NOT "time")
//   03 = White OFF   B5=0x00
//   04 = White ON    B5=0x00
//   05 = Blue  ON    B5=0x00
//   07 = State set   B3=rgbOn.state  (the target RGB color for the schedule)
//   08 = RGB   ON    B5=rgb_state FROZEN at schedule-save time
//   09 = RGB   OFF   B5=rgb_state FROZEN (stored for restore at next ON)
//
// Disable a slot by setting active=false (sends HH=MM=B5=0x00).
// Type 07 B3 == rgbOn.state: tells the device which color to use when the
// schedule fires. Types 08/09 B5 carry the same value independently.

struct SchedSlot {
    bool    active = true;
    uint8_t hh = 0, mm = 0;
};

struct SchedRgbSlot {
    bool    active = true;
    uint8_t hh = 0, mm = 0;
    uint8_t state = 0x86;   // frozen rgb_state byte (on|color)
};

class ScheduleState {
public:
    SchedSlot    whiteOff = {true, 23,  0};
    SchedSlot    whiteOn  = {true, 11,  0};
    SchedSlot    blueOn   = {true, 11,  0};
    SchedSlot    blueOff  = {true, 23, 30};   // type 02
    SchedRgbSlot rgbOn    = {true, 11,  0, 0x86};
    SchedRgbSlot rgbOff   = {true, 23,  0, 0x86};

    explicit ScheduleState(RFTransmitter& rf) : rf_(rf) {}

    // Build and transmit the full 7-packet schedule burst (types 02–05, 07–09).
    void send();

    // Minutes until the next scheduled event from a given time; -1 if none active
    int minutesUntilNext(uint8_t hh, uint8_t mm, const char** labelOut = nullptr) const;

private:
    RFTransmitter& rf_;

    void buildSlotPkt_(uint8_t* out, const SchedSlot& s, uint8_t type) const;
    void buildRgbSlotPkt_(uint8_t* out, const SchedRgbSlot& s, uint8_t type) const;
};

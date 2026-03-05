#pragma once
#include <Arduino.h>
#include "RFTransmitter.h"
#include "ChannelState.h"

// ── ScheduleState ─────────────────────────────────────────────────────────────
// Stores the six schedule slots and builds/transmits the schedule packet burst.
//
// CONFIRMED FINAL sequence (×3): 03 → 04 → 05 → 07 → 08 → 09 → 02
//
//   02 = Blue  OFF   B5=0x00  (last — type number is NOT "time")
//   03 = White OFF   B5=0x00
//   04 = White ON    B5=0x00
//   05 = Blue  ON    B5=0x00
//   07 = State broadcast  B3=LIVE rgbStateByte  (independent of slot B5)
//   08 = RGB   ON    B5=rgb_state FROZEN at schedule-save time
//   09 = RGB   OFF   B5=rgb_state FROZEN (stored for restore at next ON)
//
// Disable a slot by setting active=false (sends HH=MM=B5=0x00).
// Type 07 always reflects the LIVE channel state at send time.
// Types 08/09 carry their own frozen state, independent of the live channel.

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

    ScheduleState(RFTransmitter& rf, ChannelState& ch)
        : rf_(rf), ch_(ch) {}

    void send();

    // Minutes until the next scheduled event from a given time; -1 if none active
    int minutesUntilNext(uint8_t hh, uint8_t mm, const char** labelOut = nullptr) const;

private:
    RFTransmitter& rf_;
    ChannelState&  ch_;

    void buildSlotPkt_(uint8_t* out, const SchedSlot& s, uint8_t type) const;
    void buildRgbSlotPkt_(uint8_t* out, const SchedRgbSlot& s, uint8_t type) const;
};

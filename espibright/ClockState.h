#pragma once
#include <Arduino.h>
#include "RFTransmitter.h"

// ── ClockState ────────────────────────────────────────────────────────────────
// Tracks the time value sent to the OptiBright and handles time packet
// transmission. Time packets use TYPE 0x01 (HMS).

class ClockState {
public:
    uint8_t hh = 19, mm = 20, ss = 29;

    explicit ClockState(RFTransmitter& rf) : rf_(rf) {}

    void set(uint8_t h, uint8_t m, uint8_t s);
    void send(const char* label = "TIME SEND");

private:
    RFTransmitter& rf_;
};

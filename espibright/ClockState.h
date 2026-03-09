#pragma once
#include <Arduino.h>
#include "RFTransmitter.h"

// ── ClockState ────────────────────────────────────────────────────────────────
// Tracks the time value sent to the OptiBright and handles time packet
// transmission. Time packets use TYPE 0x01 (HMS).

class ClockState {
public:
    uint8_t hh = 19, mm = 20, ss = 29;
    bool    ntpSynced = false;   // true once NTP has set the clock at least once

    explicit ClockState(RFTransmitter& rf) : rf_(rf) {}

    // Set the time and write it through to the hardware RTC so tick() stays in sync.
    void set(uint8_t h, uint8_t m, uint8_t s);

    // Transmit a TYPE 0x01 HMS time packet burst to the fixture.
    void send(const char* label = "TIME SEND");

    // Sync from NTP. Blocks up to timeoutMs waiting for a valid fix.
    // Returns true if successful. tz_offset_sec adjusts for local time.
    bool syncNtp(long tz_offset_sec = 0, unsigned long timeoutMs = 6000);

    // Read the current time from the hardware RTC and update hh/mm/ss.
    // Call every loop() iteration; updates are sub-ms cost when the RTC is ready.
    void tick();

private:
    RFTransmitter& rf_;
};

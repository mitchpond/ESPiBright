#include "ClockState.h"

void ClockState::set(uint8_t h, uint8_t m, uint8_t s) {
    hh = h; mm = m; ss = s;
    Serial.printf("[TIME] set %02d:%02d:%02d\n", hh, mm, ss);
}

void ClockState::send(const char* label) {
    rf_.sendTimePackets(hh, mm, ss, label);
}

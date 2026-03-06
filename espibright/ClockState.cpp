#include "ClockState.h"
#include <time.h>

void ClockState::set(uint8_t h, uint8_t m, uint8_t s) {
    hh = h; mm = m; ss = s;
    Serial.printf("[TIME] set %02d:%02d:%02d\n", hh, mm, ss);
}

void ClockState::send(const char* label) {
    rf_.sendTimePackets(hh, mm, ss, label);
}

bool ClockState::syncNtp(long tz_offset_sec, unsigned long timeoutMs) {
    configTime(tz_offset_sec, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] syncing...");
    unsigned long start = millis();
    struct tm t;
    while (!getLocalTime(&t, 200)) {
        if (millis() - start > timeoutMs) {
            Serial.println("[NTP] timed out");
            return false;
        }
    }
    set(t.tm_hour, t.tm_min, t.tm_sec);
    ntpSynced = true;
    Serial.printf("[NTP] synced %02d:%02d:%02d\n", hh, mm, ss);
    return true;
}

void ClockState::tick() {
    unsigned long now = millis();
    if (lastTickMs_ == 0) { lastTickMs_ = now; return; }
    if (now - lastTickMs_ < 1000) return;
    lastTickMs_ = now;
    if (++ss >= 60) { ss = 0; if (++mm >= 60) { mm = 0; if (++hh >= 24) hh = 0; } }
}

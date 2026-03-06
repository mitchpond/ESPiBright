#include "ClockState.h"
#include <time.h>
#include <sys/time.h>

void ClockState::set(uint8_t h, uint8_t m, uint8_t s) {
    hh = h; mm = m; ss = s;
    // Write through to the hardware RTC so tick() reads back what we just set
    struct timeval tv;
    struct tm t = {};
    t.tm_hour = h; t.tm_min = m; t.tm_sec = s;
    t.tm_mday = 1; t.tm_mon = 0; t.tm_year = 70; // epoch date placeholder
    tv.tv_sec  = mktime(&t);
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
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
    struct tm t;
    if (getLocalTime(&t, 0)) {
        hh = t.tm_hour;
        mm = t.tm_min;
        ss = t.tm_sec;
    }
}

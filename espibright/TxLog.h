#pragma once
#include <Arduino.h>

#define LOG_ENTRIES  40
#define LOG_PKT_MAX   8
#define LOG_HEX_LEN  17

struct LogPkt {
    char hex[LOG_HEX_LEN];
    char note[12];
};

struct LogEntry {
    unsigned long ms;
    char label[48];
    LogPkt pkts[LOG_PKT_MAX];
    uint8_t npkts;
};

// ── TxLog ─────────────────────────────────────────────────────────────────────
// Simple ring buffer recording outgoing RF transmissions.
// Usage: begin() → addPkt() × N → commit()

class TxLog {
public:
    void begin(const char* label);
    void addPkt(const uint8_t* p8, const char* note);
    void commit();

    // Total transmissions ever logged (monotonically increasing, used as sequence number)
    int  totalCount() const { return count_; }

    // Direct slot access for JSON serialisation (used by WebAPI).
    // offset=0 is the most recent entry, offset=1 the one before, etc.
    const LogEntry* slotAt(int offset) const;
    int  available() const { return min(count_, LOG_ENTRIES); }

private:
    LogEntry entries_[LOG_ENTRIES];
    int      writeIdx_ = 0;
    int      count_    = 0;
    LogEntry* cur_     = nullptr;
};

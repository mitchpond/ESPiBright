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
    int   count     = 0;

    void begin(const char* label);
    void addPkt(const uint8_t* p8, const char* note);
    void commit();

    // Iterate entries newer than global index `since`, newest-first
    // Returns number of entries written to `out` (up to `maxOut`)
    int  getEntriesSince(int since, LogEntry* out, int maxOut) const;
    int  totalCount() const { return count; }

    // Direct slot access for JSON serialisation (used by WebAPI)
    const LogEntry* slotAt(int offset) const;   // offset from most recent (0=newest)
    int  available() const { return min(count, LOG_ENTRIES); }

private:
    LogEntry entries_[LOG_ENTRIES];
    int      writeIdx_ = 0;
    LogEntry* cur_     = nullptr;
};

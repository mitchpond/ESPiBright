#pragma once
#include <Arduino.h>
#include <driver/rmt_tx.h>
#include <functional>
#include "config.h"
#include "Protocol.h"
#include "TxLog.h"

// ── RFTransmitter ─────────────────────────────────────────────────────────────
// Owns the RMT peripheral, CRC table, and TX log.
// Higher-level state classes call sendPkt() which handles burst + logging.

class RFTransmitter {
public:
    // Set this before any transmissions to get a flash callback on every TX
    std::function<void()> onTransmit;

    explicit RFTransmitter(TxLog& log) : log_(log) {}

    void begin();

    // Raw single transmission (no log, no burst)
    void transmitOnce(const uint8_t* p8);

    // Transmit p ×3 with 1ms gap
    void tx3(const uint8_t* p8);

    // Full burst: CMD×3 [+ HMS×3 if withTime && timeEnabled], log, fire onTransmit
    void sendPkt(const uint8_t* p8, bool withTime, const char* label);

    // Build and send a time HMS packet
    void sendTimePackets(uint8_t hh, uint8_t mm, uint8_t ss, const char* label);

    // Packet builder (wraps Protocol::buildPacket with our CRC table)
    void buildPacket(const uint8_t* p7, uint8_t* out8) const;

    // Access CRC table for external packet builders
    const uint8_t* crcTable() const { return crcTable_; }

    bool timeEnabled = true;   // global +TIME toggle
    int  repeatCount = TX_REPEAT; // burst repeat count (set via API or config)

    // Last TX info (read by WebAPI / Display)
    char          lastLabel[48] = "none";
    char          lastHex[20]   = "--";
    unsigned long lastMs        = 0;

    // Public log access (needed by ScheduleState which builds its own burst)
    TxLog& log() { return log_; }

    // Record a TX without transmitting (for callers that manage their own burst)
    void recordTx(const uint8_t* p8, const char* label);

private:
    TxLog&               log_;
    rmt_channel_handle_t chan_    = nullptr;
    rmt_encoder_handle_t encoder_ = nullptr;
    uint8_t              crcTable_[256];
    uint8_t              pktTimeHms_[8];

    void recordTx_(const uint8_t* p8, const char* label);
};

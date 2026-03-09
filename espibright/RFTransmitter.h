#pragma once
#include <Arduino.h>
#include <driver/rmt_tx.h>
#include <functional>
#include "config.h"
#include "Protocol.h"
#include "TxLog.h"

// ── RFTransmitter ─────────────────────────────────────────────────────────────
// Owns the RMT peripheral, CRC table, and TX log.
//
// Transmission model — buffer-and-flush:
//   1. clearBuf()           — reset the packet buffer
//   2. addToBuf(p8, note)   — append a packet (up to BUF_CAP)
//   3. flush(label)         — repeat the whole buffer repeatCount times,
//                             with a 1 ms gap between full-batch repeats,
//                             then log and fire onTransmit
//
// sendPkt() and sendTimePackets() use this internally; callers that need to
// send a heterogeneous burst (e.g. ScheduleState) may call the buffer API
// directly.

class RFTransmitter {
public:
    std::function<void()> onTransmit;   // optional callback fired after every flush

    explicit RFTransmitter(TxLog& log) : log_(log) {}

    void begin();

    // Raw single transmission — no repeat, no log
    void transmitOnce(const uint8_t* p8);

    // ── Buffer API ────────────────────────────────────────────────────────────
    void clearBuf();
    void addToBuf (const uint8_t* p8, const char* note); // repeated repeatCount times
    void addToTail(const uint8_t* p8, const char* note); // sent ONCE after burst
    // Transmit buf_ × repeatCount, then tail_ × 1; log all; fire onTransmit
    void flush(const char* label);

    // ── Convenience wrappers ─────────────────────────────────────────────────
    // CMD × repeatCount, then HMS × 1 (tail) if withTime && timeEnabled
    void sendPkt(const uint8_t* p8, bool withTime, const char* label);

    // Build and send a time HMS packet burst
    void sendTimePackets(uint8_t hh, uint8_t mm, uint8_t ss, const char* label);

    // Packet builder (wraps Protocol::buildPacket with our CRC table)
    void buildPacket(const uint8_t* p7, uint8_t* out8) const;

    // Access CRC table for external packet builders
    const uint8_t* crcTable() const { return crcTable_; }

    bool timeEnabled = true;           // global +TIME toggle
    int  repeatCount  = TX_REPEAT;     // batch repeat count (1–20, set via API)
    int  packetGapMs  = TX_PACKET_GAP_MS; // ms gap between burst repeats (0–1000)

    // Last TX info (read by WebAPI / Display)
    char          lastLabel[48] = "none";
    char          lastHex[20]   = "--";
    unsigned long lastMs        = 0;

private:
    static constexpr int BUF_CAP = 16;

    struct BufPkt {
        uint8_t pkt[8];
        char    note[12];
    };

    TxLog&               log_;
    rmt_channel_handle_t chan_    = nullptr;
    rmt_encoder_handle_t encoder_ = nullptr;
    uint8_t              crcTable_[256];
    uint8_t              pktTimeHms_[8];

    BufPkt  buf_[BUF_CAP];
    int     bufN_  = 0;
    BufPkt  tail_[BUF_CAP];
    int     tailN_ = 0;

    void recordTx_(const uint8_t* p8, const char* label);
};

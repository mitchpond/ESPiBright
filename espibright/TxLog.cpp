#include "TxLog.h"

void TxLog::begin(const char* label) {
    cur_ = &entries_[writeIdx_];
    cur_->ms = millis();
    strncpy(cur_->label, label, sizeof(cur_->label) - 1);
    cur_->label[sizeof(cur_->label) - 1] = '\0';
    cur_->npkts = 0;
}

void TxLog::addPkt(const uint8_t* p8, const char* note) {
    if (!cur_ || cur_->npkts >= LOG_PKT_MAX) return;
    LogPkt* lp = &cur_->pkts[cur_->npkts++];
    snprintf(lp->hex, sizeof(lp->hex),
             "%02x%02x%02x%02x%02x%02x%02x%02x",
             p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7]);
    strncpy(lp->note, note, sizeof(lp->note) - 1);
    lp->note[sizeof(lp->note) - 1] = '\0';
}

void TxLog::commit() {
    writeIdx_ = (writeIdx_ + 1) % LOG_ENTRIES;
    count++;
    cur_ = nullptr;
}

const LogEntry* TxLog::slotAt(int offset) const {
    int total = min(count, LOG_ENTRIES);
    if (offset < 0 || offset >= total) return nullptr;
    int slot = ((writeIdx_ - 1 - offset) + LOG_ENTRIES * 2) % LOG_ENTRIES;
    return &entries_[slot];
}

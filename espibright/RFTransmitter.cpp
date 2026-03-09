#include "RFTransmitter.h"

void RFTransmitter::begin() {
    Protocol::buildCrcTable(crcTable_);

    rmt_tx_channel_config_t cc = {
        .gpio_num          = TX_PIN,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = 1000000,
        .mem_block_symbols = 128,
        .trans_queue_depth = 4
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&cc, &chan_));

    rmt_copy_encoder_config_t ec = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&ec, &encoder_));
    ESP_ERROR_CHECK(rmt_enable(chan_));
}

void RFTransmitter::transmitOnce(const uint8_t* p8) {
    static rmt_symbol_word_t sym[MAX_SYMBOLS];
    int idx = 0;
    sym[idx++] = {.duration0=1, .level0=0, .duration1=RESET_GAP_US, .level1=0};
    for (int b = 0; b < 8; b++)
        for (int bit = 7; bit >= 0; bit--) {
            uint16_t gap = ((p8[b] >> bit) & 1) ? LONG_GAP_US : SHORT_GAP_US;
            sym[idx++] = {.duration0=PULSE_US, .level0=1, .duration1=gap, .level1=0};
        }
    sym[idx++] = {.duration0=PULSE_US, .level0=1, .duration1=SHORT_GAP_US, .level1=0};
    rmt_transmit_config_t tc = {.loop_count=0};
    ESP_ERROR_CHECK(rmt_transmit(chan_, encoder_, sym, idx * sizeof(rmt_symbol_word_t), &tc));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan_, 200));
}

void RFTransmitter::clearBuf() {
    bufN_  = 0;
    tailN_ = 0;
}

void RFTransmitter::addToBuf(const uint8_t* p8, const char* note) {
    if (bufN_ >= BUF_CAP) return;
    BufPkt& bp = buf_[bufN_++];
    memcpy(bp.pkt, p8, 8);
    strncpy(bp.note, note, sizeof(bp.note) - 1);
    bp.note[sizeof(bp.note) - 1] = '\0';
}

void RFTransmitter::addToTail(const uint8_t* p8, const char* note) {
    if (tailN_ >= BUF_CAP) return;
    BufPkt& bp = tail_[tailN_++];
    memcpy(bp.pkt, p8, 8);
    strncpy(bp.note, note, sizeof(bp.note) - 1);
    bp.note[sizeof(bp.note) - 1] = '\0';
}

void RFTransmitter::flush(const char* label) {
    if (bufN_ == 0 && tailN_ == 0) return;

    // Repeated section
    for (int rep = 0; rep < repeatCount; rep++) {
        for (int i = 0; i < bufN_; i++)
            transmitOnce(buf_[i].pkt);
        if (rep < repeatCount - 1) delay(1);
    }
    // Tail section — once, after all repeats
    for (int i = 0; i < tailN_; i++)
        transmitOnce(tail_[i].pkt);

    log_.begin(label);
    for (int i = 0; i < bufN_;  i++) log_.addPkt(buf_[i].pkt,  buf_[i].note);
    for (int i = 0; i < tailN_; i++) log_.addPkt(tail_[i].pkt, tail_[i].note);
    log_.commit();

    recordTx_(bufN_ > 0 ? buf_[0].pkt : tail_[0].pkt, label);
}

void RFTransmitter::buildPacket(const uint8_t* p7, uint8_t* out8) const {
    Protocol::buildPacket(crcTable_, p7, out8);
}

void RFTransmitter::sendPkt(const uint8_t* p8, bool withTime, const char* label) {
    clearBuf();
    addToBuf(p8, "CMD");
    if (withTime && timeEnabled) addToTail(pktTimeHms_, "HMS");
    flush(label);
}

void RFTransmitter::sendTimePackets(uint8_t hh, uint8_t mm, uint8_t ss, const char* label) {
    uint8_t p7[7] = {0xD0, 0x23, hh, mm, ss, 0x00, 0x01};
    buildPacket(p7, pktTimeHms_);
    clearBuf();
    addToBuf(pktTimeHms_, "HMS");
    flush(label);
}

void RFTransmitter::recordTx_(const uint8_t* p8, const char* label) {
    strncpy(lastLabel, label, sizeof(lastLabel) - 1);
    lastLabel[sizeof(lastLabel) - 1] = '\0';
    snprintf(lastHex, sizeof(lastHex),
             "%02x%02x%02x%02x%02x%02x%02x%02x",
             p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7]);
    lastMs = millis();
    Serial.printf("[TX] %-24s %s\n", lastLabel, lastHex);
    if (onTransmit) onTransmit();
}

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

void RFTransmitter::tx3(const uint8_t* p8) {
    for (int i = 0; i < 3; i++) {
        transmitOnce(p8);
        if (i < 2) delay(1);
    }
}

void RFTransmitter::buildPacket(const uint8_t* p7, uint8_t* out8) const {
    Protocol::buildPacket(crcTable_, p7, out8);
}

void RFTransmitter::sendPkt(const uint8_t* p8, bool withTime, const char* label) {
    log_.begin(label);
    log_.addPkt(p8, "CMD x3");
    tx3(p8);

    if (withTime && timeEnabled) {
        for (int i = 0; i < 3; i++) {
            transmitOnce(pktTimeHms_);
            if (i < 2) delay(1);
        }
        log_.addPkt(pktTimeHms_, "HMS x3");
    }

    log_.commit();
    recordTx_(p8, label);
}

void RFTransmitter::sendTimePackets(uint8_t hh, uint8_t mm, uint8_t ss, const char* label) {
    uint8_t p7[7] = {0xD0, 0x23, hh, mm, ss, 0x00, 0x01};
    buildPacket(p7, pktTimeHms_);

    log_.begin(label);
    for (int i = 0; i < 3; i++) {
        transmitOnce(pktTimeHms_);
        if (i < 2) delay(1);
    }
    log_.addPkt(pktTimeHms_, "HMS x3");
    log_.commit();
    recordTx_(pktTimeHms_, label);
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

void RFTransmitter::recordTx(const uint8_t* p8, const char* label) {
    recordTx_(p8, label);
}

#include "ChannelState.h"

uint8_t ChannelState::rgbStateByte() const {
    return Protocol::rgbStateByte(rgbOn, rgbColor);
}

uint8_t ChannelState::dominantLevel() const {
    if (rgbOn)    return rgbLevel;
    if (whiteOn)  return whiteLevel;
    if (blueOn)   return blueLevel;
    return 0x0A;
}

void ChannelState::send() {
    uint8_t b3 = Protocol::levelByte(whiteOn, whiteLevel);
    uint8_t b4 = Protocol::levelByte(blueOn,  blueLevel);
    uint8_t b5 = rgbStateByte();
    uint8_t b6 = rgbOn ? rgbCycle : 0x00;
    uint8_t b7 = (dominantLevel() << 4) | 0x06;  // TYPE byte: high=level, low=0x06 (cmd type)

    uint8_t pkt[8];
    Protocol::buildChannelPacket(rf_.crcTable(), rf_.deviceAddr(), b3, b4, b5, b6, b7, pkt);
    rf_.sendPkt(pkt, true, "CHANNEL CMD");
}

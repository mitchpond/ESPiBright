#include "ScheduleState.h"

void ScheduleState::buildSlotPkt_(uint8_t* out, const SchedSlot& s, uint8_t type) const {
    Protocol::buildSchedPacket(rf_.crcTable(),
        s.active ? s.hh : 0x00,
        s.active ? s.mm : 0x00,
        0x00, type, out);
}

void ScheduleState::buildRgbSlotPkt_(uint8_t* out, const SchedRgbSlot& s, uint8_t type) const {
    Protocol::buildSchedPacket(rf_.crcTable(),
        s.active ? s.hh    : 0x00,
        s.active ? s.mm    : 0x00,
        s.active ? s.state : 0x00,
        type, out);
}

void ScheduleState::send() {
    uint8_t pkts[7][8];
    const char* notes[7] = {
        "BlOFF(02)", "WhOFF(03)", "WhON(04)", "BlON(05)",
        "STATE(07)", "RGBON(08)", "RGBOFF(09)"
    };

    buildSlotPkt_   (pkts[0], blueOff, 0x02);
    buildSlotPkt_   (pkts[1], whiteOff, 0x03);
    buildSlotPkt_   (pkts[2], whiteOn,  0x04);
    buildSlotPkt_   (pkts[3], blueOn,   0x05);

    // Type 07: the target RGB state for the schedule (same as rgbOn.state)
    {
        uint8_t schedState = rgbOn.active ? rgbOn.state : 0x00;
        uint8_t p7[7] = {PROTO_ADDR0, PROTO_ADDR1, schedState, 0x00, 0x00, 0x00, 0x07};
        rf_.buildPacket(p7, pkts[4]);
    }

    buildRgbSlotPkt_(pkts[5], rgbOn,   0x08);
    buildRgbSlotPkt_(pkts[6], rgbOff,  0x09);

    rf_.clearBuf();
    for (int i = 0; i < 7; i++)
        rf_.addToBuf(pkts[i], notes[i]);
    rf_.flush("SCHEDULE");
}

int ScheduleState::minutesUntilNext(uint8_t hh, uint8_t mm, const char** labelOut) const {
    int nowMins = hh * 60 + mm;
    int best = 99999;
    const char* bestLbl = nullptr;

    auto check = [&](bool active, uint8_t sh, uint8_t sm, const char* lbl) {
        if (!active) return;
        int t = sh * 60 + sm;
        int delta = t - nowMins;
        if (delta <= 0) delta += 1440;
        if (delta < best) { best = delta; bestLbl = lbl; }
    };

    check(whiteOn.active,  whiteOn.hh,  whiteOn.mm,  "W.ON");
    check(whiteOff.active, whiteOff.hh, whiteOff.mm, "W.OFF");
    check(blueOn.active,   blueOn.hh,   blueOn.mm,   "B.ON");
    check(blueOff.active,  blueOff.hh,  blueOff.mm,  "B.OFF");
    check(rgbOn.active,    rgbOn.hh,    rgbOn.mm,     "RGB.ON");
    check(rgbOff.active,   rgbOff.hh,   rgbOff.mm,   "RGB.OFF");

    if (labelOut) *labelOut = bestLbl;
    return bestLbl ? best : -1;
}

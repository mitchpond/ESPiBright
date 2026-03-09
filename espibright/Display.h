#pragma once
#include <M5Unified.h>
#include "config.h"
#include "ChannelState.h"
#include "ScheduleState.h"
#include "ClockState.h"
#include "RFTransmitter.h"

enum UIPage { PAGE_LIVE = 0, PAGE_SCHEDULE = 1, PAGE_SYSTEM = 2, PAGE_COUNT = 3 };

// ── Display ───────────────────────────────────────────────────────────────────
// Owns all screen drawing. Does not transmit anything directly — calls back
// into ChannelState, ScheduleState, ClockState via references.
// Button B on PAGE_LIVE  → ch.send()
// Button B on PAGE_SCHED → sched.send()
// Button C on PAGE_LIVE  → clock.send()
// Button C on PAGE_SYS   → ESP.restart()

class Display {
public:
    String hostname  = "---";   // set by main after WiFi connects
    String ipAddress = "---";   // set by main after WiFi connects

    Display(ChannelState& ch, ScheduleState& sched,
            ClockState& clock, RFTransmitter& rf)
        : ch_(ch), sched_(sched), clock_(clock), rf_(rf) {}

    void begin();
    void update();          // call every loop()

    void drawBootSplash();
    void drawConnecting();

    void markDirty() { dirty_ = true; }
    void flashTx()   { txLit_ = true; txUntil_ = millis() + 280; }

private:
    ChannelState&  ch_;
    ScheduleState& sched_;
    ClockState&    clock_;
    RFTransmitter& rf_;

    UIPage   page_       = PAGE_LIVE;
    bool     dirty_      = true;
    bool     landscape_  = true;
    uint8_t  rotation_   = 3;
    uint32_t tickMs_     = 0;
    uint32_t imuMs_      = 0;
    bool     txLit_      = false;
    uint32_t txUntil_    = 0;

    // Drawing helpers
    void redraw_();
    void refreshHeader_();
    void drawHeader_(const char* title);
    void drawFooter_(const char* a, const char* b, const char* c);
    void drawLevelBar_(int x, int y, uint8_t lvl, bool on, uint16_t litCol);
    void drawRssiBars_(int x, int y, int rssi);
    void drawSchedSlot_(int x, int y, int w, bool active,
                        uint8_t hh, uint8_t mm, uint16_t dotCol);

    void drawPageLive_();
    void drawPageSchedule_();
    void drawPageSystem_();

    // IMU + buttons
    void checkOrientation_();
    void handleButtons_();

    // Color helpers
    static const char*  rgbColorName(uint8_t c);
    static uint16_t     rgbPresetTft(uint8_t c);
    static uint16_t     rainbowNow();
    static uint16_t     rgbColorDisplay(uint8_t c, bool on);
    static const char*  speedLabel(uint8_t cyc);
};

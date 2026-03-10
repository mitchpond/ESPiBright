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
// BtnA click on PAGE_LIVE     → ch.send()
// BtnA click on PAGE_SCHEDULE → sched.send()
// BtnA click on PAGE_SYSTEM   → clock.send("SYNC")
// BtnA hold  on PAGE_SYSTEM   → ESP.restart()
// BtnB click (any page)       → cycle to next page

class Display {
public:
    // Set by main after WiFi connects / settings load
    String hostname  = "---";
    String ipAddress = "---";
    String wifiSsid  = WIFI_SSID;

    Display(ChannelState& ch, ScheduleState& sched,
            ClockState& clock, RFTransmitter& rf)
        : ch_(ch), sched_(sched), clock_(clock), rf_(rf) {}

    void begin();
    void update();          // call every loop()

    void drawBootSplash();
    void drawConnecting();

    void markDirty() { dirty_ = true; }
    void flashTx()   { txLit_ = true; txUntil_ = millis() + 280; }

    // Brightness applied when the display is awake (0–255).
    // Setting this after begin() defers the hardware write to the next update() call.
    void    setWakeBrightness(uint8_t b);
    uint8_t wakeBrightness() const { return wakeBrightness_; }

    // How long (ms) of inactivity before the screen blanks.
    void     setSleepTimeout(uint32_t ms) { sleepTimeoutMs_ = ms; }
    uint32_t sleepTimeout()         const { return sleepTimeoutMs_; }

private:
    ChannelState&  ch_;
    ScheduleState& sched_;
    ClockState&    clock_;
    RFTransmitter& rf_;

    uint32_t sleepTimeoutMs_ = SLEEP_TIMEOUT_MS;
    uint8_t  wakeBrightness_ = WAKE_BRIGHTNESS;

    UIPage   page_             = PAGE_LIVE;
    bool     dirty_            = true;
    bool     pendingBrightness_ = false;
    bool     landscape_     = true;
    uint8_t  rotation_      = 3;
    uint32_t tickMs_        = 0;
    uint32_t imuMs_         = 0;
    bool     txLit_         = false;
    uint32_t txUntil_       = 0;
    bool     sleeping_      = false;
    uint32_t lastActivityMs_ = 0;

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

    // IMU + buttons + sleep
    void checkOrientation_();
    void handleButtons_();
    void wake_();

    // Color helpers
    static const char*  rgbColorName(uint8_t c);
    static uint16_t     rgbPresetTft(uint8_t c);
    static uint16_t     rainbowNow();
    static uint16_t     rgbColorDisplay(uint8_t c, bool on);
    static const char*  speedLabel(uint8_t cyc);
};

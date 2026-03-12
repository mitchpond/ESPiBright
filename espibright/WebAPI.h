#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "RFTransmitter.h"
#include "ChannelState.h"
#include "ScheduleState.h"
#include "ClockState.h"
#include "Display.h"
#include "Storage.h"
#include "TxLog.h"

// ── WebAPI ────────────────────────────────────────────────────────────────────
// Owns the WebServer and all HTTP route handlers.
// Call begin() after WiFi is up to register routes and start the server.
// Call handle() from loop().

class WebAPI {
public:
    WebAPI(RFTransmitter& rf, ChannelState& ch,
           ScheduleState& sched, ClockState& clock,
           TxLog& log, Storage& store, Display& display)
        : rf_(rf), ch_(ch), sched_(sched), clock_(clock),
          log_(log), store_(store), display_(display), server_(80) {}

    void begin();
    void handle() { server_.handleClient(); }

private:
    RFTransmitter& rf_;
    ChannelState&  ch_;
    ScheduleState& sched_;
    ClockState&    clock_;
    TxLog&         log_;
    Storage&       store_;
    Display&       display_;
    WebServer      server_;

    String pktHex_(const uint8_t* p) const;
    void   sendCors_();
    bool   parseBody_(JsonDocument& doc);

    // Route handlers
    void handleRoot_();
    void handleApiStatus_();
    void handleApiChannels_();
    void handleApiSchedule_();
    void handleApiLog_();
    void handleSendRaw_();
    void handleSendChannels_();
    void handleTimeSet_();
    void handleTimeSend_();
    void handleTimeNtp_();
    void handleScheduleSet_();
    void handleScheduleSend_();
    void handleTimeGlobal_();
    void handleRepeatSet_();
    void handlePacketGapSet_();
    void handleBurstGapSet_();
    void handleSettingsDevGet_();
    void handleSettingsDevPost_();
    void handleReboot_();
    void handleOptions_();

    // Fixture management
    void handleFixturesList_();
    void handleFixturesAdd_();
    void handleFixturesRemove_();
    void handleFixturesUpdate_();
};

#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "RFTransmitter.h"
#include "ChannelState.h"
#include "ScheduleState.h"
#include "ClockState.h"
#include "Storage.h"
#include "TxLog.h"

// ── Known packet table ────────────────────────────────────────────────────────
struct KnownPacket {
    const char* label;
    const char* group;
    uint8_t     payload[7];
    uint8_t     pkt[8];
    bool        sendTime;
};

// ── WebAPI ────────────────────────────────────────────────────────────────────
// Owns the WebServer and all HTTP route handlers.
// Call begin() after WiFi is up to register routes and start the server.
// Call handle() from loop().

class WebAPI {
public:
    WebAPI(RFTransmitter& rf, ChannelState& ch,
           ScheduleState& sched, ClockState& clock,
           TxLog& log, Storage& store)
        : rf_(rf), ch_(ch), sched_(sched), clock_(clock),
          log_(log), store_(store), server_(80) {}

    void begin();
    void handle() { server_.handleClient(); }

private:
    RFTransmitter& rf_;
    ChannelState&  ch_;
    ScheduleState& sched_;
    ClockState&    clock_;
    TxLog&         log_;
    Storage&       store_;
    WebServer      server_;

    KnownPacket knownPkts_[14];
    int         numKnownPkts_ = 0;

    void buildKnownPackets_();
    String pktHex_(const uint8_t* p) const;
    String pktJson_(int i) const;
    void   sendCors_();
    bool   parseBody_(JsonDocument& doc);

    // Route handlers
    void handleRoot_();
    void handleApiPackets_();
    void handleApiStatus_();
    void handleApiChannels_();
    void handleApiSchedule_();
    void handleApiLog_();
    void handleSendIndex_();
    void handleSendRaw_();
    void handleSendChannels_();
    void handleTimeSet_();
    void handleTimeSend_();
    void handleScheduleSet_();
    void handleScheduleSend_();
    void handleTimeGlobal_();
    void handleRepeatSet_();
    void handleOptions_();
};

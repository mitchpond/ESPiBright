#include "WebAPI.h"
#include <ESPmDNS.h>
#include "config.h"
#include "html.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

void WebAPI::buildKnownPackets_() {
    const uint8_t* t = rf_.crcTable();
    struct Init { const char* label; const char* group; uint8_t p[7]; bool st; };
    static const Init inits[] = {
        {"ALL ON",         "Power",    {0xd0,0x23,0x8a,0x8a,0x86,0x01,0xa6}, true },
        {"ALL OFF",        "Power",    {0xd0,0x23,0x0a,0x0a,0x06,0x00,0xa6}, true },
        {"DIM 10%",        "Dim",      {0xd0,0x23,0x81,0x81,0x06,0x01,0x16}, true },
        {"DIM 50%",        "Dim",      {0xd0,0x23,0x85,0x85,0x06,0x01,0x56}, true },
        {"DIM 75%",        "Dim",      {0xd0,0x23,0x87,0x87,0x06,0x01,0x76}, true },
        {"DIM 100%",       "Dim",      {0xd0,0x23,0x8a,0x8a,0x86,0x01,0xa6}, true },
        {"WHITE ONLY 100", "Channel",  {0xd0,0x23,0x8a,0x0a,0x06,0x01,0xa6}, true },
        {"WHITE ONLY 50",  "Channel",  {0xd0,0x23,0x85,0x05,0x06,0x01,0x56}, true },
        {"BLUE ONLY 100",  "Channel",  {0xd0,0x23,0x0a,0x8a,0x06,0x01,0xa6}, true },
        {"BLUE ONLY 50",   "Channel",  {0xd0,0x23,0x05,0x85,0x06,0x01,0x56}, true },
        {"SCHED TYPE 03",  "Schedule", {0xd0,0x23,0x17,0x00,0x00,0x00,0x03}, false},
        {"SCHED TYPE 08",  "Schedule", {0xd0,0x23,0x0b,0x00,0x86,0x00,0x08}, false},
        {"TIME HMS",       "Time",     {0xd0,0x23,0x13,0x14,0x1d,0x00,0x01}, false},
    };
    numKnownPkts_ = sizeof(inits) / sizeof(inits[0]);
    for (int i = 0; i < numKnownPkts_; i++) {
        knownPkts_[i].label    = inits[i].label;
        knownPkts_[i].group    = inits[i].group;
        knownPkts_[i].sendTime = inits[i].st;
        memcpy(knownPkts_[i].payload, inits[i].p, 7);
        Protocol::buildPacket(t, inits[i].p, knownPkts_[i].pkt);
    }
}

String WebAPI::pktHex_(const uint8_t* p) const {
    char b[17];
    snprintf(b, sizeof(b), "%02x%02x%02x%02x%02x%02x%02x%02x",
             p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    return String(b);
}

void WebAPI::sendCors_() {
    server_.sendHeader("Access-Control-Allow-Origin",  "*");
    server_.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server_.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

bool WebAPI::parseBody_(JsonDocument& doc) {
    return server_.hasArg("plain") && !deserializeJson(doc, server_.arg("plain"));
}

// Build one JSON object for a known packet entry
String WebAPI::pktJson_(int i) const {
    return String("{\"index\":") + i
         + ",\"label\":\"" + knownPkts_[i].label + "\""
         + ",\"group\":\"" + knownPkts_[i].group + "\""
         + ",\"hex\":\""   + pktHex_(knownPkts_[i].pkt) + "\""
         + ",\"send_time\":" + (knownPkts_[i].sendTime ? "true" : "false")
         + "}";
}

// ── Registration ──────────────────────────────────────────────────────────────

void WebAPI::begin() {
    buildKnownPackets_();

    server_.on("/",                        HTTP_GET,  [this](){ handleRoot_(); });
    server_.on("/api/packets",             HTTP_GET,  [this](){ handleApiPackets_(); });
    server_.on("/api/status",              HTTP_GET,  [this](){ handleApiStatus_(); });
    server_.on("/api/channels",            HTTP_GET,  [this](){ handleApiChannels_(); });
    server_.on("/api/schedule",            HTTP_GET,  [this](){ handleApiSchedule_(); });
    server_.on("/api/log",                 HTTP_GET,  [this](){ handleApiLog_(); });
    server_.on("/api/send/index",          HTTP_POST, [this](){ handleSendIndex_(); });
    server_.on("/api/send/raw",            HTTP_POST, [this](){ handleSendRaw_(); });
    server_.on("/api/send/channels",       HTTP_POST, [this](){ handleSendChannels_(); });
    server_.on("/api/time/set",            HTTP_POST, [this](){ handleTimeSet_(); });
    server_.on("/api/time/send",           HTTP_POST, [this](){ handleTimeSend_(); });
    server_.on("/api/time/ntp",            HTTP_POST, [this](){ handleTimeNtp_(); });
    server_.on("/api/schedule/set",        HTTP_POST, [this](){ handleScheduleSet_(); });
    server_.on("/api/schedule/send",       HTTP_POST, [this](){ handleScheduleSend_(); });
    server_.on("/api/settings/time_global",HTTP_POST, [this](){ handleTimeGlobal_(); });
    server_.on("/api/settings/repeat",     HTTP_POST, [this](){ handleRepeatSet_(); });
    server_.onNotFound([this](){
        server_.send(404, "application/json", "{\"error\":\"not found\"}");
    });

    const char* corsPaths[] = {
        "/api/send/index", "/api/send/raw", "/api/send/channels",
        "/api/time/set", "/api/time/send", "/api/time/ntp",
        "/api/schedule/set", "/api/schedule/send",
        "/api/settings/time_global", "/api/settings/repeat"
    };
    for (auto r : corsPaths)
        server_.on(r, HTTP_OPTIONS, [this](){ handleOptions_(); });

    server_.begin();
}

// ── GET handlers ──────────────────────────────────────────────────────────────

void WebAPI::handleRoot_() {
    sendCors_();
    String inj = "window.__PACKETS__=[";
    for (int i = 0; i < numKnownPkts_; i++) {
        if (i > 0) inj += ",";
        inj += pktJson_(i);
    }
    inj += "];";
    String html = FPSTR(HTML);
    html.replace("const PACKETS = window.__PACKETS__ || [];",
                 inj + "\nconst PACKETS=window.__PACKETS__;");
    server_.send(200, "text/html", html);
}

void WebAPI::handleApiPackets_() {
    sendCors_();
    String j = "[";
    for (int i = 0; i < numKnownPkts_; i++) {
        if (i > 0) j += ",";
        j += pktJson_(i);
    }
    j += "]";
    server_.send(200, "application/json", j);
}

void WebAPI::handleApiStatus_() {
    sendCors_();
    String j = "{\"label\":\""  + String(rf_.lastLabel) + "\""
             + ",\"hex\":\""    + String(rf_.lastHex)   + "\""
             + ",\"ms_ago\":"   + (rf_.lastMs ? String(millis() - rf_.lastMs) : String("null"))
             + ",\"send_time_global\":" + (rf_.timeEnabled ? "true" : "false")
             + ",\"repeat_count\":"    + rf_.repeatCount
             + ",\"time\":{\"hh\":" + clock_.hh
             + ",\"mm\":"           + clock_.mm
             + ",\"ss\":"           + clock_.ss + "}}";
    server_.send(200, "application/json", j);
}

void WebAPI::handleApiChannels_() {
    sendCors_();
    String j = String("{\"white_on\":") + (ch_.whiteOn ? "true" : "false")
             + ",\"white_level\":" + ch_.whiteLevel
             + ",\"blue_on\":"     + (ch_.blueOn ? "true" : "false")
             + ",\"blue_level\":"  + ch_.blueLevel
             + ",\"rgb_on\":"      + (ch_.rgbOn ? "true" : "false")
             + ",\"rgb_color\":"   + ch_.rgbColor
             + ",\"rgb_cycle\":"   + ch_.rgbCycle
             + ",\"rgb_level\":"   + ch_.rgbLevel
             + "}";
    server_.send(200, "application/json", j);
}

static String schedSlotJson(const char* key, const SchedSlot& s) {
    return String("\"") + key + "\":{\"active\":" + (s.active ? "true" : "false")
         + ",\"hh\":" + s.hh + ",\"mm\":" + s.mm + "}";
}
static String schedRgbSlotJson(const char* key, const SchedRgbSlot& s) {
    return String("\"") + key + "\":{\"active\":" + (s.active ? "true" : "false")
         + ",\"hh\":" + s.hh + ",\"mm\":" + s.mm + ",\"state\":" + s.state + "}";
}

void WebAPI::handleApiSchedule_() {
    sendCors_();
    String j = String("{")
             + schedSlotJson("white_on",  sched_.whiteOn)  + ","
             + schedSlotJson("white_off", sched_.whiteOff) + ","
             + schedSlotJson("blue_on",   sched_.blueOn)   + ","
             + schedSlotJson("blue_off",  sched_.blueOff)  + ","
             + schedRgbSlotJson("rgb_on",  sched_.rgbOn)   + ","
             + schedRgbSlotJson("rgb_off", sched_.rgbOff)
             + "}";
    server_.send(200, "application/json", j);
}

void WebAPI::handleApiLog_() {
    sendCors_();
    int since = server_.hasArg("since") ? server_.arg("since").toInt() : 0;
    int total = log_.available();

    String j = "{\"count\":" + String(log_.count) + ",\"entries\":[";
    bool first = true;

    for (int offset = 0; offset < total; offset++) {
        int globalIdx = log_.count - 1 - offset;
        if (globalIdx <= since) break;
        const LogEntry* e = log_.slotAt(offset);
        if (!e) continue;

        if (!first) j += ",";
        first = false;

        j += "{\"seq\":"     + String(globalIdx)
           + ",\"ms\":"      + String(e->ms)
           + ",\"label\":\"" + String(e->label) + "\""
           + ",\"pkts\":[";
        for (int p = 0; p < e->npkts; p++) {
            if (p > 0) j += ",";
            j += "{\"hex\":\""  + String(e->pkts[p].hex)  + "\""
               + ",\"note\":\"" + String(e->pkts[p].note) + "\"}";
        }
        j += "]}";
    }
    j += "]}";
    server_.send(200, "application/json", j);
}

// ── POST handlers ─────────────────────────────────────────────────────────────

static const char JSON_OK[]       = "{\"ok\":true}";
static const char JSON_BAD_BODY[] = "{\"ok\":false,\"error\":\"bad body\"}";

void WebAPI::handleSendIndex_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int idx = doc["index"] | -1;
    if (idx < 0 || idx >= numKnownPkts_) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"index out of range\"}");
        return;
    }
    rf_.sendPkt(knownPkts_[idx].pkt, knownPkts_[idx].sendTime, knownPkts_[idx].label);
    String r = String("{\"ok\":true,\"label\":\"") + knownPkts_[idx].label
             + "\",\"hex\":\"" + pktHex_(knownPkts_[idx].pkt) + "\"}";
    server_.send(200, "application/json", r);
}

void WebAPI::handleSendRaw_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    const char* ph = doc["payload"] | "";
    bool wt = doc["send_time"] | false;
    if (strlen(ph) != 14) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"payload must be 14 hex chars\"}");
        return;
    }
    uint8_t p7[7];
    for (int i = 0; i < 7; i++) {
        char b[3] = {ph[i*2], ph[i*2+1], 0};
        p7[i] = (uint8_t)strtol(b, nullptr, 16);
    }
    uint8_t pkt[8];
    rf_.buildPacket(p7, pkt);
    rf_.sendPkt(pkt, wt, "RAW");
    String r = String("{\"ok\":true,\"hex\":\"") + pktHex_(pkt) + "\"}";
    server_.send(200, "application/json", r);
}

void WebAPI::handleSendChannels_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    ch_.whiteOn    = doc["white_on"]    | ch_.whiteOn;
    ch_.whiteLevel = doc["white_level"] | ch_.whiteLevel;
    ch_.blueOn     = doc["blue_on"]     | ch_.blueOn;
    ch_.blueLevel  = doc["blue_level"]  | ch_.blueLevel;
    ch_.rgbOn      = doc["rgb_on"]      | ch_.rgbOn;
    ch_.rgbColor   = doc["rgb_color"]   | ch_.rgbColor;
    ch_.rgbCycle   = doc["rgb_cycle"]   | ch_.rgbCycle;
    ch_.rgbLevel   = doc["rgb_level"]   | ch_.rgbLevel;
    ch_.send();
    store_.saveChannels(ch_);
    String r = String("{\"ok\":true,\"label\":\"") + rf_.lastLabel
             + "\",\"hex\":\"" + rf_.lastHex + "\"}";
    server_.send(200, "application/json", r);
}

void WebAPI::handleTimeSet_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    clock_.set(doc["hh"] | clock_.hh, doc["mm"] | clock_.mm, doc["ss"] | clock_.ss);
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleTimeSend_() {
    sendCors_();
    clock_.send("TIME SEND");
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleTimeNtp_() {
    sendCors_();
    bool ok = clock_.syncNtp(TZ_OFFSET_SEC);
    if (ok) {
        String r = String("{\"ok\":true,\"hh\":") + clock_.hh
                 + ",\"mm\":" + clock_.mm
                 + ",\"ss\":" + clock_.ss + "}";
        server_.send(200, "application/json", r);
    } else {
        server_.send(200, "application/json", "{\"ok\":false,\"error\":\"NTP sync timed out\"}");
    }
}

void WebAPI::handleScheduleSet_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }

    auto readSlot = [&](const char* key, SchedSlot& s) {
        if (!doc[key].is<JsonObject>()) return;
        JsonObject o = doc[key];
        s.active = o["active"] | s.active;
        s.hh     = o["hh"]     | s.hh;
        s.mm     = o["mm"]     | s.mm;
    };
    auto readRgb = [&](const char* key, SchedRgbSlot& s) {
        if (!doc[key].is<JsonObject>()) return;
        JsonObject o = doc[key];
        s.active = o["active"] | s.active;
        s.hh     = o["hh"]     | s.hh;
        s.mm     = o["mm"]     | s.mm;
        s.state  = o["state"]  | s.state;
    };

    readSlot("white_on",  sched_.whiteOn);
    readSlot("white_off", sched_.whiteOff);
    readSlot("blue_on",   sched_.blueOn);
    readSlot("blue_off",  sched_.blueOff);
    readRgb ("rgb_on",    sched_.rgbOn);
    readRgb ("rgb_off",   sched_.rgbOff);
    store_.saveSchedule(sched_);
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleScheduleSend_() {
    sendCors_();
    clock_.send("TIME SEND");
    sched_.send();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleTimeGlobal_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    rf_.timeEnabled = doc["enabled"] | rf_.timeEnabled;
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleRepeatSet_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int n = doc["count"] | -1;
    if (n < 1 || n > 20) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"count must be 1-20\"}");
        return;
    }
    rf_.repeatCount = n;
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleOptions_() {
    sendCors_();
    server_.send(204);
}

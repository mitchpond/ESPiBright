#include "WebAPI.h"
#include <ESPmDNS.h>
#include <M5Unified.h>
#include <esp_random.h>
#include "config.h"
#include "html.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

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

// ── Registration ──────────────────────────────────────────────────────────────

void WebAPI::begin() {
    server_.on("/",                        HTTP_GET,  [this](){ handleRoot_(); });
    server_.on("/api/status",              HTTP_GET,  [this](){ handleApiStatus_(); });
    server_.on("/api/channels",            HTTP_GET,  [this](){ handleApiChannels_(); });
    server_.on("/api/schedule",            HTTP_GET,  [this](){ handleApiSchedule_(); });
    server_.on("/api/log",                 HTTP_GET,  [this](){ handleApiLog_(); });
    server_.on("/api/fixtures",            HTTP_GET,  [this](){ handleFixturesList_(); });
    server_.on("/api/send/raw",            HTTP_POST, [this](){ handleSendRaw_(); });
    server_.on("/api/send/channels",       HTTP_POST, [this](){ handleSendChannels_(); });
    server_.on("/api/time/set",            HTTP_POST, [this](){ handleTimeSet_(); });
    server_.on("/api/time/send",           HTTP_POST, [this](){ handleTimeSend_(); });
    server_.on("/api/time/ntp",            HTTP_POST, [this](){ handleTimeNtp_(); });
    server_.on("/api/schedule/set",        HTTP_POST, [this](){ handleScheduleSet_(); });
    server_.on("/api/schedule/send",       HTTP_POST, [this](){ handleScheduleSend_(); });
    server_.on("/api/fixtures/add",        HTTP_POST, [this](){ handleFixturesAdd_(); });
    server_.on("/api/fixtures/remove",     HTTP_POST, [this](){ handleFixturesRemove_(); });
    server_.on("/api/fixtures/update",     HTTP_POST, [this](){ handleFixturesUpdate_(); });
    server_.on("/api/settings/time_global",HTTP_POST, [this](){ handleTimeGlobal_(); });
    server_.on("/api/settings/repeat",     HTTP_POST, [this](){ handleRepeatSet_(); });
    server_.on("/api/settings/packet_gap", HTTP_POST, [this](){ handlePacketGapSet_(); });
    server_.on("/api/settings/burst_gap",  HTTP_POST, [this](){ handleBurstGapSet_(); });
    server_.on("/api/settings/device",     HTTP_GET,  [this](){ handleSettingsDevGet_(); });
    server_.on("/api/settings/device",     HTTP_POST, [this](){ handleSettingsDevPost_(); });
    server_.on("/api/reboot",              HTTP_POST, [this](){ handleReboot_(); });
    server_.onNotFound([this](){
        server_.send(404, "application/json", "{\"error\":\"not found\"}");
    });

    const char* corsPaths[] = {
        "/api/send/raw", "/api/send/channels",
        "/api/time/set", "/api/time/send", "/api/time/ntp",
        "/api/schedule/set", "/api/schedule/send",
        "/api/fixtures/add", "/api/fixtures/remove", "/api/fixtures/update",
        "/api/settings/time_global", "/api/settings/repeat",
        "/api/settings/packet_gap", "/api/settings/burst_gap",
        "/api/settings/device", "/api/reboot"
    };
    for (auto r : corsPaths)
        server_.on(r, HTTP_OPTIONS, [this](){ handleOptions_(); });

    server_.begin();
}

// ── GET handlers ──────────────────────────────────────────────────────────────

void WebAPI::handleRoot_() {
    sendCors_();
    server_.setContentLength(sizeof(HTML) - 1);
    server_.send(200, "text/html", "");
    server_.sendContent_P(HTML, sizeof(HTML) - 1);
}

void WebAPI::handleApiStatus_() {
    sendCors_();
    int bat = M5.Power.getBatteryLevel();
    String j = "{\"label\":\""  + String(rf_.lastLabel()) + "\""
             + ",\"hex\":\""    + String(rf_.lastHex())   + "\""
             + ",\"ms_ago\":"   + (rf_.lastMs() ? String(millis() - rf_.lastMs()) : String("null"))
             + ",\"send_time_global\":" + (rf_.timeEnabled() ? "true" : "false")
             + ",\"repeat_count\":"    + rf_.repeatCount()
             + ",\"packet_gap_us\":"   + rf_.packetGapUs()
             + ",\"burst_gap_ms\":"    + rf_.burstGapMs()
             + ",\"battery_pct\":"     + bat
             + ",\"build\":\""         + FW_BUILD + "\""
             + ",\"time\":{\"hh\":" + clock_.hh
             + ",\"mm\":"           + clock_.mm
             + ",\"ss\":"           + clock_.ss + "}}";
    server_.send(200, "application/json", j);
}

void WebAPI::handleApiChannels_() {
    sendCors_();
    int fix = server_.hasArg("fixture") ? server_.arg("fixture").toInt() : 0;
    if (fix < 0 || fix >= store_.settings.fixtureCount) fix = 0;
    ChannelState ch(rf_);
    store_.loadFixtureChannels(fix, ch);
    String j = String("{\"white_on\":") + (ch.whiteOn ? "true" : "false")
             + ",\"white_level\":" + ch.whiteLevel
             + ",\"blue_on\":"     + (ch.blueOn ? "true" : "false")
             + ",\"blue_level\":"  + ch.blueLevel
             + ",\"rgb_on\":"      + (ch.rgbOn ? "true" : "false")
             + ",\"rgb_color\":"   + ch.rgbColor
             + ",\"rgb_cycle\":"   + ch.rgbCycle
             + ",\"rgb_level\":"   + ch.rgbLevel
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
    int fix = server_.hasArg("fixture") ? server_.arg("fixture").toInt() : 0;
    if (fix < 0 || fix >= store_.settings.fixtureCount) fix = 0;
    ScheduleState sched(rf_);
    store_.loadFixtureSchedule(fix, sched);
    String j = String("{")
             + schedSlotJson("white_on",  sched.whiteOn)  + ","
             + schedSlotJson("white_off", sched.whiteOff) + ","
             + schedSlotJson("blue_on",   sched.blueOn)   + ","
             + schedSlotJson("blue_off",  sched.blueOff)  + ","
             + schedRgbSlotJson("rgb_on",  sched.rgbOn)   + ","
             + schedRgbSlotJson("rgb_off", sched.rgbOff)
             + "}";
    server_.send(200, "application/json", j);
}

void WebAPI::handleApiLog_() {
    sendCors_();
    int since = server_.hasArg("since") ? server_.arg("since").toInt() : 0;
    int total = log_.available();

    int logTotal = log_.totalCount();
    String j = "{\"count\":" + String(logTotal) + ",\"entries\":[";
    bool first = true;

    for (int offset = 0; offset < total; offset++) {
        int globalIdx = logTotal - 1 - offset;
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

// ── Shared response constants ─────────────────────────────────────────────────
static const char JSON_OK[]       = "{\"ok\":true}";
static const char JSON_BAD_BODY[] = "{\"ok\":false,\"error\":\"bad body\"}";

// ── Fixture handlers ──────────────────────────────────────────────────────────

void WebAPI::handleFixturesList_() {
    sendCors_();
    String j = "[";
    for (int i = 0; i < store_.settings.fixtureCount; i++) {
        if (i > 0) j += ",";
        j += String("{\"index\":") + i
           + ",\"name\":\""  + store_.fixtures[i].name + "\""
           + ",\"addr\":"    + store_.fixtures[i].addr
           + "}";
    }
    j += "]";
    server_.send(200, "application/json", j);
}


void WebAPI::handleFixturesAdd_() {
    sendCors_();
    if (store_.settings.fixtureCount >= Storage::MAX_FIXTURES) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"max fixtures reached\"}");
        return;
    }
    int newIdx = store_.settings.fixtureCount;
    snprintf(store_.fixtures[newIdx].name, sizeof(store_.fixtures[newIdx].name),
             "Fixture %d", newIdx + 1);
    uint8_t addr = (uint8_t)(esp_random() & 0xFF);
    if (!addr) addr = 0xA0;
    store_.fixtures[newIdx].addr = addr;
    store_.settings.fixtureCount++;
    store_.saveFixtureSettings(newIdx, store_.fixtures[newIdx]);
    store_.saveSettings();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleFixturesRemove_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int idx = doc["index"] | -1;
    int cnt = store_.settings.fixtureCount;
    if (idx < 0 || idx >= cnt || cnt <= 1) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid\"}");
        return;
    }
    // Shift fixtures array down
    for (int i = idx; i < cnt - 1; i++)
        store_.fixtures[i] = store_.fixtures[i + 1];
    store_.settings.fixtureCount--;
    store_.saveFixtures();
    store_.saveSettings();
    // Reload display state from fixture 0 (safe fallback)
    store_.loadFixtureData(0, ch_, sched_);
    rf_.setDeviceAddr(store_.fixtures[0].addr);
    display_.markDirty();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleFixturesUpdate_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int idx = doc["index"] | -1;
    if (idx < 0 || idx >= store_.settings.fixtureCount) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid index\"}");
        return;
    }
    if (!doc["name"].isNull()) {
        const char* n = doc["name"].as<const char*>();
        strncpy(store_.fixtures[idx].name, n, sizeof(store_.fixtures[idx].name) - 1);
        store_.fixtures[idx].name[sizeof(store_.fixtures[idx].name) - 1] = '\0';
    }
    if (!doc["addr"].isNull()) {
        int a = doc["addr"].as<int>();
        if (a >= 1 && a <= 255)
            store_.fixtures[idx].addr = (uint8_t)a;
    }
    store_.saveFixtureSettings(idx, store_.fixtures[idx]);
    server_.send(200, "application/json", JSON_OK);
}

// ── POST handlers ─────────────────────────────────────────────────────────────

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
    int fix = doc["fixture"] | 0;
    if (fix < 0 || fix >= store_.settings.fixtureCount) fix = 0;
    // Load fixture's persisted state, apply request overrides, send, save back
    store_.loadFixtureData(fix, ch_, sched_);
    rf_.setDeviceAddr(store_.fixtures[fix].addr);
    ch_.whiteOn    = doc["white_on"]    | ch_.whiteOn;
    ch_.whiteLevel = doc["white_level"] | ch_.whiteLevel;
    ch_.blueOn     = doc["blue_on"]     | ch_.blueOn;
    ch_.blueLevel  = doc["blue_level"]  | ch_.blueLevel;
    ch_.rgbOn      = doc["rgb_on"]      | ch_.rgbOn;
    ch_.rgbColor   = doc["rgb_color"]   | ch_.rgbColor;
    ch_.rgbCycle   = doc["rgb_cycle"]   | ch_.rgbCycle;
    ch_.rgbLevel   = doc["rgb_level"]   | ch_.rgbLevel;
    ch_.send();
    store_.saveFixtureChannels(fix, ch_);
    display_.markDirty();
    String r = String("{\"ok\":true,\"label\":\"") + rf_.lastLabel()
             + "\",\"hex\":\"" + rf_.lastHex() + "\"}";
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
    bool ok = clock_.syncNtp(store_.settings.tzOffsetSec);
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

    int fix = doc["fixture"] | 0;
    if (fix < 0 || fix >= store_.settings.fixtureCount) fix = 0;
    // Load into a temp; don't disturb the display's sched_
    ScheduleState tmp(rf_);
    store_.loadFixtureSchedule(fix, tmp);
    auto& s = tmp;
    readSlot("white_on",  s.whiteOn);
    readSlot("white_off", s.whiteOff);
    readSlot("blue_on",   s.blueOn);
    readSlot("blue_off",  s.blueOff);
    readRgb ("rgb_on",    s.rgbOn);
    readRgb ("rgb_off",   s.rgbOff);
    store_.saveFixtureSchedule(fix, tmp);
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleScheduleSend_() {
    sendCors_();
    JsonDocument doc;
    int fix = (parseBody_(doc)) ? (doc["fixture"] | 0) : 0;
    if (fix < 0 || fix >= store_.settings.fixtureCount) fix = 0;
    store_.loadFixtureData(fix, ch_, sched_);
    rf_.setDeviceAddr(store_.fixtures[fix].addr);
    display_.markDirty();
    clock_.send("TIME SEND");
    sched_.send();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleTimeGlobal_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    rf_.setTimeEnabled(doc["enabled"] | rf_.timeEnabled());
    store_.settings.timeEnabled = rf_.timeEnabled();
    store_.saveSettings();
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
    rf_.setRepeatCount(n);
    store_.settings.repeatCount = rf_.repeatCount();
    store_.saveSettings();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handlePacketGapSet_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int g = doc["gap_us"] | -1;
    if (g < 0 || g > 9999) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"gap_us must be 0-9999\"}");
        return;
    }
    rf_.setPacketGapUs(g);
    store_.settings.packetGapUs = rf_.packetGapUs();
    store_.saveSettings();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleBurstGapSet_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }
    int g = doc["gap_ms"] | -1;
    if (g < 0 || g > 1000) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"gap_ms must be 0-1000\"}");
        return;
    }
    rf_.setBurstGapMs(g);
    store_.settings.burstGapMs = rf_.burstGapMs();
    store_.saveSettings();
    server_.send(200, "application/json", JSON_OK);
}

void WebAPI::handleSettingsDevGet_() {
    sendCors_();
    String j = String("{\"repeat_count\":")      + rf_.repeatCount()
             + ",\"packet_gap_us\":"             + rf_.packetGapUs()
             + ",\"burst_gap_ms\":"              + rf_.burstGapMs()
             + ",\"time_enabled\":"              + (rf_.timeEnabled() ? "true" : "false")
             + ",\"sleep_timeout_sec\":"         + (display_.sleepTimeout() / 1000)
             + ",\"brightness\":"               + display_.wakeBrightness()
             + ",\"hostname\":\""               + String(store_.settings.hostname) + "\""
             + ",\"wifi_ssid\":\""              + String(store_.settings.wifiSsid) + "\""
             + ",\"wifi_pass\":\"***\""
             + ",\"tz_offset_sec\":"             + store_.settings.tzOffsetSec
             + "}";
    server_.send(200, "application/json", j);
}

void WebAPI::handleSettingsDevPost_() {
    sendCors_();
    JsonDocument doc;
    if (!parseBody_(doc)) { server_.send(400, "application/json", JSON_BAD_BODY); return; }

    bool rebootRequired = false;

    if (!doc["repeat_count"].isNull()) {
        int n = doc["repeat_count"].as<int>();
        if (n >= 1 && n <= 20) { rf_.setRepeatCount(n); store_.settings.repeatCount = rf_.repeatCount(); }
    }
    if (!doc["packet_gap_us"].isNull()) {
        int g = doc["packet_gap_us"].as<int>();
        if (g >= 0 && g <= 9999) { rf_.setPacketGapUs(g); store_.settings.packetGapUs = rf_.packetGapUs(); }
    }
    if (!doc["burst_gap_ms"].isNull()) {
        int g = doc["burst_gap_ms"].as<int>();
        if (g >= 0 && g <= 1000) { rf_.setBurstGapMs(g); store_.settings.burstGapMs = rf_.burstGapMs(); }
    }
    if (!doc["time_enabled"].isNull()) {
        rf_.setTimeEnabled(doc["time_enabled"].as<bool>());
        store_.settings.timeEnabled = rf_.timeEnabled();
    }
    if (!doc["sleep_timeout_sec"].isNull()) {
        int t = doc["sleep_timeout_sec"].as<int>();
        if (t >= 5 && t <= 3600) {
            display_.setSleepTimeout((uint32_t)t * 1000);
            store_.settings.sleepTimeoutSec = (uint16_t)t;
        }
    }
    if (!doc["brightness"].isNull()) {
        int b = doc["brightness"].as<int>();
        if (b >= 0 && b <= 255) {
            display_.setWakeBrightness((uint8_t)b);
            store_.settings.brightness = (uint8_t)b;
        }
    }
    if (!doc["hostname"].isNull()) {
        const char* h = doc["hostname"].as<const char*>();
        strncpy(store_.settings.hostname, h, sizeof(store_.settings.hostname) - 1);
        rebootRequired = true;
    }
    if (!doc["wifi_ssid"].isNull()) {
        const char* s = doc["wifi_ssid"].as<const char*>();
        strncpy(store_.settings.wifiSsid, s, sizeof(store_.settings.wifiSsid) - 1);
        rebootRequired = true;
    }
    if (!doc["wifi_pass"].isNull()) {
        const char* pw = doc["wifi_pass"].as<const char*>();
        strncpy(store_.settings.wifiPass, pw, sizeof(store_.settings.wifiPass) - 1);
        rebootRequired = true;
    }
    if (!doc["tz_offset_sec"].isNull()) {
        store_.settings.tzOffsetSec = doc["tz_offset_sec"].as<int32_t>();
        rebootRequired = true;
    }

    store_.saveSettings();
    String r = String("{\"ok\":true,\"reboot_required\":")
             + (rebootRequired ? "true" : "false") + "}";
    server_.send(200, "application/json", r);
}

void WebAPI::handleReboot_() {
    sendCors_();
    server_.send(200, "application/json", JSON_OK);
    delay(200);
    ESP.restart();
}

void WebAPI::handleOptions_() {
    sendCors_();
    server_.send(204);
}

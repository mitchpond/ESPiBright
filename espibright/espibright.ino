// ESPiBright — WiFi bridge for OptiBright RF aquarium lights
// Target hardware: M5Stack Nano C6 (or similar ESP32 with M5Unified)
//
// Future radio ideas:
//   BLE    — WiFi commissioning (credentials without reflashing)
//   Zigbee — Matter/Thread integration for home automation meshes
//   LoRa   — multi-device deployments (greenhouse, distributed tanks)

#include <M5Unified.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "config.h"
#include "TxLog.h"
#include "Protocol.h"
#include "RFTransmitter.h"
#include "ChannelState.h"
#include "ClockState.h"
#include "ScheduleState.h"
#include "Storage.h"
#include "Display.h"
#include "WebAPI.h"

// ── Global instances ──────────────────────────────────────────────────────────
TxLog         txLog;
RFTransmitter rf(txLog);
ChannelState  channels(rf);
ClockState    clock_(rf);
ScheduleState schedule(rf);
Storage       store;
Display       display(channels, schedule, clock_, rf);
WebAPI        web(rf, channels, schedule, clock_, txLog, store, display);

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    // Load device settings first — they affect everything below
    store.loadSettings();
    DevSettings& s = store.settings;

    // Apply display settings before splash
    display.setSleepTimeout((uint32_t)s.sleepTimeoutSec * 1000);
    display.setWakeBrightness(s.brightness);
    display.wifiSsid = s.wifiSsid;

    display.begin();
    display.drawBootSplash();
    display.drawConnecting();

    // Apply TX settings
    rf.setRepeatCount(s.repeatCount);
    rf.setPacketGapMs(s.packetGapMs);
    rf.setTimeEnabled(s.timeEnabled);

    // Restore persisted channel/schedule state before anything transmits
    store.loadAll(channels, schedule);

    rf.begin();

    // Wire TX flash callback: every transmission lights the header dot
    rf.onTransmit = [](){ display.flashTx(); display.markDirty(); };

    // Wire time provider so every +TIME tail uses the live clock, not a stale cache
    rf.getTime = [](uint8_t& h, uint8_t& m, uint8_t& s){
        h = clock_.hh; m = clock_.mm; s = clock_.ss;
    };

    // Connect WiFi using persisted credentials
    WiFi.setHostname(s.hostname);
    WiFi.begin(s.wifiSsid, s.wifiPass);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500);
        tries++;
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        display.hostname  = s.hostname;
        display.ipAddress = WiFi.localIP().toString();
        MDNS.begin(s.hostname);
        Serial.println("Host: " + display.hostname);
        Serial.println("IP:   " + display.ipAddress);
        clock_.syncNtp(s.tzOffsetSec);
    } else {
        display.hostname  = "NO WIFI";
        display.ipAddress = "---";
        Serial.println("WiFi failed");
    }

    web.begin();
    display.markDirty();
    Serial.println("Ready.");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    M5.update();
    clock_.tick();
    web.handle();
    display.update();
    delay(2);
}

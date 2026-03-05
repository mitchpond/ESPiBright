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
#include "WebAPI.h"
#include "Display.h"

// ── Global instances ──────────────────────────────────────────────────────────
TxLog        txLog;
RFTransmitter rf(txLog);
ChannelState  channels(rf);
ClockState    clock_(rf);
ScheduleState schedule(rf, channels);
Storage       store;
Display       display(channels, schedule, clock_, rf);
WebAPI        web(rf, channels, schedule, clock_, txLog, store);

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    display.begin();
    display.drawBootSplash();
    display.drawConnecting();

    // Restore persisted state before anything transmits
    store.loadAll(channels, schedule);

    rf.begin();

    // Wire TX flash callback: every transmission lights the header dot
    rf.onTransmit = [](){ display.flashTx(); display.markDirty(); };

    // Connect WiFi
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500);
        tries++;
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        // Show the bare hostname — no domain suffix assumed.
        // The IP is shown separately on the System page.
        display.ipAddress = HOSTNAME;
        MDNS.begin(HOSTNAME);
        Serial.println("Host: " + display.ipAddress);
        Serial.println("IP:   " + WiFi.localIP().toString());
    } else {
        display.ipAddress = "NO WIFI";
        Serial.println("WiFi failed");
    }

    web.begin();
    display.markDirty();
    Serial.println("Ready.");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    M5.update();
    web.handle();
    display.update();
    delay(2);
}

#pragma once
#include <Preferences.h>
#include <esp_random.h>
#include "config.h"
#include "ChannelState.h"
#include "ScheduleState.h"

// ── DevSettings ───────────────────────────────────────────────────────────────
// Device-level configuration persisted in NVS namespace "dev".
// Fields that require a reboot to take effect are marked [reboot].
// Defaults come from compile-time constants in config.h.

struct DevSettings {
    char    hostname[33]      = HOSTNAME;
    char    wifiSsid[65]      = WIFI_SSID;
    char    wifiPass[65]      = WIFI_PASS;
    int     repeatCount       = TX_REPEAT;
    int     packetGapUs       = TX_PACKET_GAP_US;
    int     burstGapMs        = TX_BURST_GAP_MS;
    bool    timeEnabled       = true;
    uint16_t sleepTimeoutSec  = SLEEP_TIMEOUT_MS / 1000;
    uint8_t brightness        = WAKE_BRIGHTNESS;
    int32_t tzOffsetSec       = TZ_OFFSET_SEC;
    // Per-device RF address byte (byte[0] of every packet).
    // Randomly generated on first boot and persisted in NVS.
    // The fixture learns this address from the first remote it hears after power-up.
    uint8_t deviceAddr        = PROTO_ADDR0;
};

// ── Storage ───────────────────────────────────────────────────────────────────
// Saves/loads channel, schedule, and device settings to ESP32 NVS.
// Clock time is not persisted — it's meaningless after a reboot.
//
// Usage:
//   Storage store;
//   store.loadSettings();            // call first in setup(); populates store.settings
//   store.loadAll(channels, sched);  // restore channel/schedule state
//   store.saveChannels(channels);    // call after any channel change
//   store.saveSchedule(sched);       // call after any schedule change
//   store.saveSettings();            // call after any settings change

class Storage {
public:
    /// Live device settings. Populated by loadSettings(); mutated by WebAPI handlers.
    DevSettings settings;

    void loadAll(ChannelState& ch, ScheduleState& sched) {
        loadChannels(ch);
        loadSchedule(sched);
    }

    void loadSettings() {
        DevSettings& s = settings;
        Preferences p;
        // Open read-write so we can generate+persist the device address on first boot.
        p.begin("dev", false);
        auto str = [&](const char* k, char* dst, size_t n) {
            String v = p.getString(k, "");
            if (v.length()) { strncpy(dst, v.c_str(), n - 1); dst[n - 1] = '\0'; }
        };
        str("host",  s.hostname, sizeof(s.hostname));
        str("ssid",  s.wifiSsid, sizeof(s.wifiSsid));
        str("pass",  s.wifiPass, sizeof(s.wifiPass));
        s.repeatCount    = p.getInt   ("rpt",   s.repeatCount);
        s.packetGapUs    = p.getInt   ("pkus",  s.packetGapUs);
        s.burstGapMs     = p.getInt   ("pgap",  s.burstGapMs);
        s.timeEnabled    = p.getBool  ("te",    s.timeEnabled);
        s.sleepTimeoutSec= p.getUShort("stSec", s.sleepTimeoutSec);
        s.brightness     = p.getUChar ("bright",s.brightness);
        s.tzOffsetSec    = p.getInt   ("tz",    s.tzOffsetSec);
        // Device address: generate random on first boot, restore thereafter.
        if (p.isKey("addr")) {
            s.deviceAddr = p.getUChar("addr", PROTO_ADDR0);
        } else {
            s.deviceAddr = (uint8_t)(esp_random() & 0xFF);
            if (!s.deviceAddr) s.deviceAddr = PROTO_ADDR0;  // avoid 0x00
            p.putUChar("addr", s.deviceAddr);
        }
        p.end();
    }

    void saveSettings() {
        const DevSettings& s = settings;
        Preferences p;
        p.begin("dev", false);
        p.putString("host",  s.hostname);
        p.putString("ssid",  s.wifiSsid);
        p.putString("pass",  s.wifiPass);
        p.putInt   ("rpt",   s.repeatCount);
        p.putInt   ("pkus",  s.packetGapUs);
        p.putInt   ("pgap",  s.burstGapMs);
        p.putBool  ("te",    s.timeEnabled);
        p.putUShort("stSec", s.sleepTimeoutSec);
        p.putUChar ("bright",s.brightness);
        p.putInt   ("tz",    s.tzOffsetSec);
        p.putUChar ("addr",  s.deviceAddr);
        p.end();
    }

    void saveChannels(const ChannelState& ch) {
        Preferences p;
        p.begin("ch", false);
        p.putBool("w_on",  ch.whiteOn);
        p.putUChar("w_lv", ch.whiteLevel);
        p.putBool("b_on",  ch.blueOn);
        p.putUChar("b_lv", ch.blueLevel);
        p.putBool("r_on",  ch.rgbOn);
        p.putUChar("r_co", ch.rgbColor);
        p.putUChar("r_cy", ch.rgbCycle);
        p.putUChar("r_lv", ch.rgbLevel);
        p.end();
    }

    void saveSchedule(const ScheduleState& s) {
        Preferences p;
        p.begin("sched", false);
        saveSlot(p, "wo", s.whiteOn);
        saveSlot(p, "wf", s.whiteOff);
        saveSlot(p, "bo", s.blueOn);
        saveSlot(p, "bf", s.blueOff);
        saveRgbSlot(p, "ro", s.rgbOn);
        saveRgbSlot(p, "rf", s.rgbOff);
        p.end();
    }

private:
    void loadChannels(ChannelState& ch) {
        Preferences p;
        if (!p.begin("ch", true)) return;
        ch.whiteOn    = p.getBool ("w_on", ch.whiteOn);
        ch.whiteLevel = p.getUChar("w_lv", ch.whiteLevel);
        ch.blueOn     = p.getBool ("b_on", ch.blueOn);
        ch.blueLevel  = p.getUChar("b_lv", ch.blueLevel);
        ch.rgbOn      = p.getBool ("r_on", ch.rgbOn);
        ch.rgbColor   = p.getUChar("r_co", ch.rgbColor);
        ch.rgbCycle   = p.getUChar("r_cy", ch.rgbCycle);
        ch.rgbLevel   = p.getUChar("r_lv", ch.rgbLevel);
        p.end();
    }

    void loadSchedule(ScheduleState& s) {
        Preferences p;
        if (!p.begin("sched", true)) return;
        loadSlot(p, "wo", s.whiteOn);
        loadSlot(p, "wf", s.whiteOff);
        loadSlot(p, "bo", s.blueOn);
        loadSlot(p, "bf", s.blueOff);
        loadRgbSlot(p, "ro", s.rgbOn);
        loadRgbSlot(p, "rf", s.rgbOff);
        p.end();
    }

    // ── slot helpers ──────────────────────────────────────────────────────────
    void saveSlot(Preferences& p, const char* pfx, const SchedSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); p.putBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); p.putUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); p.putUChar(k, s.mm);
    }
    void loadSlot(Preferences& p, const char* pfx, SchedSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); s.active = p.getBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); s.hh     = p.getUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); s.mm     = p.getUChar(k, s.mm);
    }
    void saveRgbSlot(Preferences& p, const char* pfx, const SchedRgbSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); p.putBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); p.putUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); p.putUChar(k, s.mm);
        snprintf(k, sizeof(k), "%s_s", pfx); p.putUChar(k, s.state);
    }
    void loadRgbSlot(Preferences& p, const char* pfx, SchedRgbSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); s.active = p.getBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); s.hh     = p.getUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); s.mm     = p.getUChar(k, s.mm);
        snprintf(k, sizeof(k), "%s_s", pfx); s.state  = p.getUChar(k, s.state);
    }
};

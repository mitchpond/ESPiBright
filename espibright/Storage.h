#pragma once
#include <Preferences.h>
#include <esp_random.h>
#include "config.h"
#include "ChannelState.h"
#include "ScheduleState.h"

// ── FixtureSettings ───────────────────────────────────────────────────────────
// Per-fixture configuration: name and RF address byte[0].
// Persisted in NVS namespace "fx0"–"fx3".

struct FixtureSettings {
    char    name[17] = "Fixture 1";   // user-visible label
    uint8_t addr     = PROTO_ADDR0;   // RF address byte[0]
};

// ── DevSettings ───────────────────────────────────────────────────────────────
// Device-level (global) configuration persisted in NVS namespace "dev".
// Fields that require a reboot to take effect are marked [reboot].

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
    uint8_t fixtureCount      = 1;    // 1–MAX_FIXTURES
};

// ── Storage ───────────────────────────────────────────────────────────────────
// Saves/loads fixture, channel, schedule, and device settings to ESP32 NVS.
// Clock time is not persisted — it's meaningless after a reboot.
//
// NVS layout:
//   "dev"       → global device settings
//   "fx{i}"     → fixture i name + address  (i = 0–MAX_FIXTURES-1)
//   "fx{i}c"    → fixture i channel state
//   "fx{i}s"    → fixture i schedule state
//
// First-boot migration: if "fx0" has no address, it is copied from the legacy
// "dev">"addr" key. Channel/schedule fall back to the old "ch"/"sched" namespaces
// if the per-fixture namespaces are empty.

class Storage {
public:
    static constexpr int MAX_FIXTURES = 4;

    DevSettings     settings;
    FixtureSettings fixtures[MAX_FIXTURES];

    // ── Boot sequence calls ────────────────────────────────────────────────────
    // 1. loadSettings()   — global device settings (must come first)
    // 2. loadFixtures()   — all fixture name/addr settings
    // 3. loadAll(ch, sched) — active fixture's channel + schedule state

    void loadSettings() {
        DevSettings& s = settings;
        Preferences p;
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
        s.fixtureCount   = p.getUChar ("fxcnt", s.fixtureCount);
        if (s.fixtureCount < 1 || s.fixtureCount > MAX_FIXTURES) s.fixtureCount = 1;
        p.end();
    }

    void loadFixtures() {
        for (int i = 0; i < MAX_FIXTURES; i++) {
            // Reset to defaults
            snprintf(fixtures[i].name, sizeof(fixtures[i].name), "Fixture %d", i + 1);
            fixtures[i].addr = PROTO_ADDR0;

            if (i >= settings.fixtureCount) continue;  // don't care about unused slots

            char ns[6];
            snprintf(ns, sizeof(ns), "fx%d", i);
            Preferences p;
            if (!p.begin(ns, true)) {
                p.end();
                if (i == 0) migrateFixture0_();
                continue;
            }
            if (p.isKey("addr")) {
                String n = p.getString("name", "");
                if (n.length()) {
                    strncpy(fixtures[i].name, n.c_str(), sizeof(fixtures[i].name) - 1);
                    fixtures[i].name[sizeof(fixtures[i].name) - 1] = '\0';
                }
                fixtures[i].addr = p.getUChar("addr", PROTO_ADDR0);
                p.end();
            } else {
                p.end();
                if (i == 0) migrateFixture0_();
            }
        }
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
        p.putUChar ("fxcnt", s.fixtureCount);
        p.end();
    }

    void saveFixtureSettings(int idx, const FixtureSettings& fs) {
        char ns[6];
        snprintf(ns, sizeof(ns), "fx%d", idx);
        Preferences p;
        p.begin(ns, false);
        p.putString("name", fs.name);
        p.putUChar("addr",  fs.addr);
        p.end();
    }

    void saveFixtures() {
        for (int i = 0; i < settings.fixtureCount; i++)
            saveFixtureSettings(i, fixtures[i]);
    }

    // ── Channel / schedule ────────────────────────────────────────────────────

    // Load fixture 0's data at boot (explicit index for clarity)
    void loadAll(ChannelState& ch, ScheduleState& sched) {
        loadFixtureData(0, ch, sched);
    }

    void saveFixtureChannels(int idx, const ChannelState& ch) {
        char ns[8];
        snprintf(ns, sizeof(ns), "fx%dc", idx);
        Preferences p;
        p.begin(ns, false);
        p.putBool ("w_on", ch.whiteOn);
        p.putUChar("w_lv", ch.whiteLevel);
        p.putBool ("b_on", ch.blueOn);
        p.putUChar("b_lv", ch.blueLevel);
        p.putBool ("r_on", ch.rgbOn);
        p.putUChar("r_co", ch.rgbColor);
        p.putUChar("r_cy", ch.rgbCycle);
        p.putUChar("r_lv", ch.rgbLevel);
        p.end();
    }

    void saveFixtureSchedule(int idx, const ScheduleState& s) {
        char ns[8];
        snprintf(ns, sizeof(ns), "fx%ds", idx);
        Preferences p;
        p.begin(ns, false);
        saveSlot_(p, "wo", s.whiteOn);
        saveSlot_(p, "wf", s.whiteOff);
        saveSlot_(p, "bo", s.blueOn);
        saveSlot_(p, "bf", s.blueOff);
        saveRgbSlot_(p, "ro", s.rgbOn);
        saveRgbSlot_(p, "rf", s.rgbOff);
        p.end();
    }

    void loadFixtureData(int idx, ChannelState& ch, ScheduleState& sched) {
        loadFixtureChannels_(idx, ch);
        loadFixtureSchedule_(idx, sched);
    }

    // Load only one half (useful for stateless GET handlers)
    void loadFixtureChannels(int idx, ChannelState& ch) { loadFixtureChannels_(idx, ch); }
    void loadFixtureSchedule(int idx, ScheduleState& s)  { loadFixtureSchedule_(idx, s); }

private:
    // ── Migration ─────────────────────────────────────────────────────────────
    // Called on first boot or when "fx0" namespace is missing.
    // Copies address from the legacy "dev">"addr" key (old single-fixture code).
    void migrateFixture0_() {
        Preferences old;
        old.begin("dev", true);
        uint8_t legacyAddr = old.getUChar("addr", 0);
        old.end();
        if (!legacyAddr) {
            legacyAddr = (uint8_t)(esp_random() & 0xFF);
            if (!legacyAddr) legacyAddr = PROTO_ADDR0;
        }
        fixtures[0].addr = legacyAddr;
        saveFixtureSettings(0, fixtures[0]);
    }

    // ── Per-fixture load helpers ───────────────────────────────────────────────
    // Falls back to legacy namespaces for fixture 0 on first run.

    void loadFixtureChannels_(int idx, ChannelState& ch) {
        char ns[8];
        snprintf(ns, sizeof(ns), "fx%dc", idx);
        Preferences p;
        if (p.begin(ns, true) && p.isKey("w_on")) {
            loadChannelsFrom_(p, ch);
            p.end();
        } else {
            p.end();
            if (idx == 0) {  // migrate from legacy "ch" namespace
                Preferences o;
                if (o.begin("ch", true)) { loadChannelsFrom_(o, ch); o.end(); }
            }
        }
    }

    void loadFixtureSchedule_(int idx, ScheduleState& s) {
        char ns[8];
        snprintf(ns, sizeof(ns), "fx%ds", idx);
        Preferences p;
        if (p.begin(ns, true) && p.isKey("wo_a")) {
            loadScheduleFrom_(p, s);
            p.end();
        } else {
            p.end();
            if (idx == 0) {  // migrate from legacy "sched" namespace
                Preferences o;
                if (o.begin("sched", true)) { loadScheduleFrom_(o, s); o.end(); }
            }
        }
    }

    void loadChannelsFrom_(Preferences& p, ChannelState& ch) {
        ch.whiteOn    = p.getBool ("w_on", ch.whiteOn);
        ch.whiteLevel = p.getUChar("w_lv", ch.whiteLevel);
        ch.blueOn     = p.getBool ("b_on", ch.blueOn);
        ch.blueLevel  = p.getUChar("b_lv", ch.blueLevel);
        ch.rgbOn      = p.getBool ("r_on", ch.rgbOn);
        ch.rgbColor   = p.getUChar("r_co", ch.rgbColor);
        ch.rgbCycle   = p.getUChar("r_cy", ch.rgbCycle);
        ch.rgbLevel   = p.getUChar("r_lv", ch.rgbLevel);
    }

    void loadScheduleFrom_(Preferences& p, ScheduleState& s) {
        loadSlot_(p, "wo", s.whiteOn);
        loadSlot_(p, "wf", s.whiteOff);
        loadSlot_(p, "bo", s.blueOn);
        loadSlot_(p, "bf", s.blueOff);
        loadRgbSlot_(p, "ro", s.rgbOn);
        loadRgbSlot_(p, "rf", s.rgbOff);
    }

    // ── Slot helpers ──────────────────────────────────────────────────────────
    void saveSlot_(Preferences& p, const char* pfx, const SchedSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); p.putBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); p.putUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); p.putUChar(k, s.mm);
    }
    void loadSlot_(Preferences& p, const char* pfx, SchedSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); s.active = p.getBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); s.hh     = p.getUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); s.mm     = p.getUChar(k, s.mm);
    }
    void saveRgbSlot_(Preferences& p, const char* pfx, const SchedRgbSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); p.putBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); p.putUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); p.putUChar(k, s.mm);
        snprintf(k, sizeof(k), "%s_s", pfx); p.putUChar(k, s.state);
    }
    void loadRgbSlot_(Preferences& p, const char* pfx, SchedRgbSlot& s) {
        char k[8];
        snprintf(k, sizeof(k), "%s_a", pfx); s.active = p.getBool (k, s.active);
        snprintf(k, sizeof(k), "%s_h", pfx); s.hh     = p.getUChar(k, s.hh);
        snprintf(k, sizeof(k), "%s_m", pfx); s.mm     = p.getUChar(k, s.mm);
        snprintf(k, sizeof(k), "%s_s", pfx); s.state  = p.getUChar(k, s.state);
    }
};

#pragma once
#include <Preferences.h>
#include "ChannelState.h"
#include "ScheduleState.h"

// ── Storage ───────────────────────────────────────────────────────────────────
// Saves/loads channel and schedule state to ESP32 NVS (non-volatile storage).
// Clock time is not persisted — it's meaningless after a reboot.
//
// Usage:
//   Storage store;
//   store.loadAll(channels, schedule);   // call in setup() before begin()
//   store.saveChannels(channels);        // call after any channel change
//   store.saveSchedule(schedule);        // call after any schedule change

class Storage {
public:
    void loadAll(ChannelState& ch, ScheduleState& sched) {
        loadChannels(ch);
        loadSchedule(sched);
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
        if (!p.begin("ch", true)) return;   // true = read-only; returns false if namespace is new
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
        saveSlot(p, pfx, s);
        char k[8];
        snprintf(k, sizeof(k), "%s_s", pfx); p.putUChar(k, s.state);
    }
    void loadRgbSlot(Preferences& p, const char* pfx, SchedRgbSlot& s) {
        loadSlot(p, pfx, s);
        char k[8];
        snprintf(k, sizeof(k), "%s_s", pfx); s.state = p.getUChar(k, s.state);
    }
};

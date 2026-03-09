#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID    "dd-wrt_vap"
#define WIFI_PASS    "wireless"
#define HOSTNAME     "ESPiBright"

// ── RF TX ─────────────────────────────────────────────────────────────────────
#define TX_PIN       GPIO_NUM_7
#define PULSE_US     508
#define SHORT_GAP_US 508
#define LONG_GAP_US  1008
#define RESET_GAP_US 6000
#define MAX_SYMBOLS  (2 + 64 + 1)
// Number of times each packet burst is repeated per send.
// Increase if the fixture misses commands due to RF interference.
#define TX_REPEAT    5

// ── Display ───────────────────────────────────────────────────────────────────
#define SLEEP_TIMEOUT_MS  (3 * 60 * 1000)  // blank screen after 3 min idle
#define SLEEP_BRIGHTNESS  180               // brightness when awake
// Target hardware: Arduino Nesso N1 (ESP32-C6, 1.14" TFT, IMU, LiPo)
// Landscape (rotation 3, normal): 240w × 135h
// Portrait  (rotation 2, tilted): 135w × 240h
// Drawing code uses D.width()/D.height() at runtime, not these constants.
// These are only kept as reference / for anything that must be compile-time.
#define SCR_W_LS     240   // landscape width
#define SCR_H_LS     135   // landscape height
#define SCR_W_PT     135   // portrait width
#define SCR_H_PT     240   // portrait height
#define HDR_H         15
#define FTR_H         14
// COL_W is computed at runtime as D.width()/3

#define UI_BG        0x0000
#define UI_HDR       0x1082
#define UI_DIM       0x4208
#define UI_BRIGHT    0xFFFF
#define UI_CYAN      0x07FF
#define UI_GREEN     0x07E0
#define UI_ORANGE    0xFCA0
#define UI_RED       0xF800
#define UI_W_COL     0xFFE0   // warm white
#define UI_B_COL     0x041F   // blue

// ── NTP ───────────────────────────────────────────────────────────────────────
// UTC offset in seconds for your local timezone.
// Examples: UTC-5 (EST) = -18000, UTC+1 (CET) = 3600, UTC+10 (AEST) = 36000
#define TZ_OFFSET_SEC  (-5 * 3600)  // CDT — change to match your timezone


// ── Firmware build number ─────────────────────────────────────────────────────
// Format: YY.M.BUILD  (year-2000, month, sequential build counter)
// Increment BUILD each release; update YY.M when the calendar month changes.
#define FW_BUILD     "26.3.1000"

#define PROTO_ADDR0  0xD0
#define PROTO_ADDR1  0x23
#define CRC_POLY     0x31

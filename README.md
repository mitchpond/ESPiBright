# ESPiBright

**WiFi bridge for OptiBright RF aquarium lights**

ESPiBright turns an Arduino Nesso N1 (ESP32-C6) into a WiFi-accessible
controller for OptiBright LED aquarium fixtures. The original remote sends
short-range 433 MHz OOK packets; ESPiBright reverse-engineers and replays
that protocol over RF while exposing a clean web UI and REST API from
anywhere on your local network.

Please keep in mind that this is early in development and this is a personal pet project.
The docs may not always be 100% up-to-date and the API is not at all stable as I am still
figuring out exactly what I want it to be. If you find this, build it, and find it useful,
know that I am overjoyed that anyone else appreciates it, but please be patient as I get it
to a solid 1.0 offering.

*Gen AI Disclaimer:* I am making use of Claude Code to help with _primarily_ the UI, as this
is an area that I am woefully underskilled in. Additionally I am using it to help keep my
mess of thoughts coherent enough to continue working on this given that I only get an hour here
or a few minutes there. Any C that it ends up writing or refactoring is read and understood by
me before being committed. I can't say the same for the HTML/JS/CSS as I simply do not have the time.

---

## Features

- **Full channel control** — White, Blue, and RGB channels with independent
  on/off and brightness (10%–100%)
- **RGB colour presets** — Blue, Green, White, Red, Orange, Purple, Pink,
  Yellow, Rainbow with cycle duration (Rainbow only)
- **Schedule programming** — per-channel on/off times sent directly to the
  fixture's internal scheduler
- **Device clock sync** — set the fixture's internal clock over WiFi
- **Configurable repeat count** — burst repeat count adjustable in the UI
  and via API to compensate for RF interference (`TX_REPEAT` in `config.h`)
- **Persistent state** — channel state and schedule survive reboots via ESP32
  NVS (non-volatile storage)
- **On-device display** — 3-page UI on the built-in TFT; auto-rotates using
  the IMU
- **Web UI** — dark-theme responsive interface served from PROGMEM; no
  internet connection required. [Demo Web UI here](https://mitchpond.github.io/ESPiBright/)
- **REST API** — all functions exposed as JSON endpoints for home automation
  integration
- **TX log** — rolling log of recent transmissions viewable in the web UI
- **mDNS** — registers as `ESPiBright` via mDNS; reachable by hostname on
  networks that resolve mDNS (e.g. `http://espibright.local` or
  `http://espibright.lan` depending on your DHCP server's search domain,
  or by IP if your network doesn't resolve hostnames)

---

## Hardware

| Component | Part |
|-----------|------|
| MCU / display | Arduino Nesso N1 (ESP32-C6, 1.14" TFT, IMU, LiPo) |
| RF transmitter | 433 MHz OOK module on GPIO 2 |
| Target fixture | OptiBright LED aquarium light (433 MHz RF remote) |

The TX pin is defined in `config.h` (`TX_PIN`, default GPIO 2). The RF
module should be a simple OOK transmitter; no specific module is required.

**Compatibility:** The code targets the Arduino Nesso N1. It should be
compatible with the M5StickC Plus / Plus2 and similar M5Stack devices that
use M5Unified, though display rotation (`config.h`) may need adjustment.

---

## Buttons

The Nesso N1 has two buttons:

| Button | Action |
|--------|--------|
| **Face button** (front of device) | Context action for current page: send channels / send schedule / sync time. Hold on System page = reboot. |
| **Side button** | Cycle through display pages |

---

## Display Pages

### Live
Shows current state of all three channels with level bars and on/off status.
Face button sends the current channel state to the fixture.

### Schedule
Shows all six schedule slots (white on/off, blue on/off, RGB on/off) with
their times. Face button transmits the full schedule sequence to the fixture.

### System
Shows WiFi status, IP address, RSSI, battery level, and uptime.
Face button pushes the current device time to the fixture.

---

## Web UI

Connect to `http://espibright.local` (or `.lan`, or the IP shown on the System
page — the hostname and IP are both displayed there). The interface loads
current channel state, schedule, and device time automatically on page load.

### Sections

- **Channels** — live sliders and on/off toggles for all three channels;
  RGB colour pills; Rainbow duration selector (Rainbow only)
- **Device Time** — set and transmit the fixture clock
- **Schedule** — configure all six on/off slots; RGB color shared across
  ON and OFF slots (set once on the ON row)
- **Known Packets** — one-click buttons for common preset commands
- **Packet Crafter** — manual hex packet builder with live CRC
- **TX Log** — recent transmissions with hex dumps, auto-polls every 1.5s
- **Header controls** — `REPEAT` (burst count, 1–20) and `+TIME` (append
  time packets to channel commands) are always visible in the header

---

## REST API

All endpoints return JSON. Base URL: `http://espibright.local` (or `.lan`, or by IP)

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET`  | `/api/status` | Device status, uptime, WiFi, time, repeat count |
| `GET`  | `/api/channels` | Current channel state |
| `GET`  | `/api/schedule` | Current schedule slots |
| `GET`  | `/api/packets` | Known packet list |
| `GET`  | `/api/log` | TX log |
| `POST` | `/api/send/channels` | Set and transmit channel state |
| `POST` | `/api/send/index` | Send a known packet by index |
| `POST` | `/api/send/raw` | Send a raw 7-byte payload (CRC auto) |
| `POST` | `/api/time/set` | Set device clock |
| `POST` | `/api/time/send` | Transmit time packets |
| `POST` | `/api/schedule/set` | Set schedule slots |
| `POST` | `/api/schedule/send` | Transmit schedule |
| `POST` | `/api/settings/time_global` | Toggle auto time-with-channels |
| `POST` | `/api/settings/repeat` | Set burst repeat count (1–20) |

**Send channels example:**
```json
POST /api/send/channels
{
  "white_on": true,  "white_level": 8,
  "blue_on":  true,  "blue_level":  6,
  "rgb_on":   true,  "rgb_color":   9,  "rgb_cycle": 4,  "rgb_level": 10
}
```
Levels are 1–10 (10%–100%). `rgb_color`: 1=Blue 2=Green 3=White 4=Red
5=Orange 6=Purple 7=Pink 8=Yellow 9=Rainbow. `rgb_cycle` (Rainbow only):
1=static 2=3s 4=4s 8=5s.

---

## Project Structure

```
espibright/
├── espibright.ino      Main sketch — setup(), loop(), global instances
├── config.h            WiFi credentials, pin definitions, constants
├── Protocol.h          Stateless packet building (header-only)
├── TxLog.h/.cpp        Ring buffer for TX history
├── RFTransmitter.h/.cpp RMT hardware layer, sendPkt(), onTransmit callback
├── ChannelState.h/.cpp  Live channel state + send()
├── ClockState.h/.cpp    Time state + send()
├── ScheduleState.h/.cpp Schedule slots + send() + countdown
├── Storage.h            NVS persistence (Preferences wrapper)
├── Display.h/.cpp       3-page TFT UI, IMU orientation, button handling
├── WebAPI.h/.cpp        HTTP server, JSON routes
├── html.h               PROGMEM HTML/CSS/JS web interface
├── README.md            This file
└── PROTOCOL.md          Reverse-engineered RF protocol reference
```

---

## Configuration

Edit `config.h` before flashing:

```cpp
#define WIFI_SSID   "your-network"
#define WIFI_PASS   "your-password"
#define HOSTNAME    "ESPiBright"      // mDNS name (no suffix — appended by router)
#define TX_PIN      GPIO_NUM_2        // 433 MHz OOK transmitter data pin
#define TX_REPEAT   5                 // burst repeat count; increase for poor RF
```

---

## Building & Flashing

1. Install [Arduino IDE](https://www.arduino.cc/en/software) 2.x
2. Add the ESP32 board package (Espressif ESP32 Arduino core ≥ 3.x)
3. Install libraries:
   - **M5Unified** (M5Stack)
   - **ArduinoJson** (Benoit Blanchon, v7)
4. Open `espibright.ino` — Arduino IDE will pick up all files in the folder
5. Select board: **Arduino Nesso N1** (or compatible ESP32-C6 variant;
   M5StickC Plus2 also works but may require rotation adjustment in `config.h`)
6. Flash

---

## RF Protocol

See [`PROTOCOL.md`](PROTOCOL.md) for the full reverse-engineered packet
specification, including all confirmed packet types, encoding, and open
questions.

**Summary:**
- 433 MHz OOK, device address `0xD0 0x23`
- 8-byte packets: 7-byte payload + 1-byte CRC (Maxim/Dallas, poly 0x31)
- TYPE `0x06` — set all channels (only live control command)
- TYPE `0x01` — set device clock (HMS)
- TYPEs `0x02`–`0x09` — schedule slots


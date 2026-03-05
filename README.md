# ESPiBright

**WiFi bridge for OptiBright RF aquarium lights**

ESPiBright turns an M5StickC Plus2 (ESP32-C6) into a WiFi-accessible
controller for OptiBright LED aquarium fixtures. The original remote sends
short-range 433 MHz OOK packets; ESPiBright reverse-engineers and replays
that protocol over RF while exposing a clean web UI and REST API from
anywhere on your local network.

---

## Features

- **Full channel control** — White, Blue, and RGB channels with independent
  on/off and brightness (10%–100%)
- **RGB colour presets** — Blue, Green, White, Red, Orange, Purple, Pink,
  Yellow, Rainbow with cycle duration (Rainbow only)
- **Schedule programming** — per-channel on/off times sent directly to the
  fixture's internal scheduler
- **Device clock sync** — set the fixture's internal clock over WiFi
- **Persistent state** — channel state and schedule survive reboots via ESP32
  NVS (non-volatile storage)
- **On-device display** — 3-page UI on the built-in TFT; auto-rotates using
  the IMU
- **Web UI** — dark-theme responsive interface served from PROGMEM; no
  internet connection required
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
| MCU / display | M5StickC Plus2 (ESP32-C6, 1.14" TFT, IMU, LiPo) |
| RF transmitter | 433 MHz OOK module on GPIO 2 |
| Target fixture | OptiBright LED aquarium light (433 MHz RF remote) |

The TX pin is defined in `config.h` (`TX_PIN`, default GPIO 2). The RF
module should be a simple OOK transmitter; no specific module is required.

---

## Buttons

| Button | Action |
|--------|--------|
| **A** (green face button) | Context action for current page: send channels / send schedule / sync time. Hold on System page = reboot. |
| **B** (side button) | Cycle through pages |

---

## Display Pages

### Live
Shows current state of all three channels with level bars and on/off status.
Send button pushes the current state to the fixture.

### Schedule
Shows all six schedule slots (white on/off, blue on/off, RGB on/off) with
their times. Send button transmits the full schedule sequence to the fixture.

### System
Shows WiFi status, IP address, RSSI, firmware info, and uptime.
Sync button pushes the current device time to the fixture.

---

## Web UI

Connect to `http://espibright.local` (or `.lan`, or the IP shown on the System
page — the hostname and IP are both displayed there). The interface loads
current channel state, schedule, and device time automatically on page load.

### Sections

- **Channels** — live sliders and on/off toggles for all three channels;
  RGB colour pills; Rainbow duration selector (visible for Rainbow only)
- **Device Time** — set and transmit the fixture clock
- **Schedule** — configure all six on/off slots with RGB colour state
- **Known Packets** — one-click buttons for common preset commands
- **Packet Crafter** — manual hex packet builder with live CRC
- **TX Log** — recent transmissions with hex dumps, auto-polls every 1.5s

---

## REST API

All endpoints return JSON. Base URL: `http://espibright.local` (or `.lan`, or by IP)

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET`  | `/api/status` | Device status, uptime, WiFi, time |
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
#define HOSTNAME    "ESPiBright"      // mDNS name (.local appended)
#define TX_PIN      GPIO_NUM_2        // 433 MHz OOK transmitter data pin
```

---

## Building & Flashing

1. Install [Arduino IDE](https://www.arduino.cc/en/software) 2.x
2. Add the ESP32 board package (Espressif ESP32 Arduino core ≥ 3.x)
3. Install libraries:
   - **M5Unified** (M5Stack)
   - **ArduinoJson** (Benoit Blanchon, v7)
4. Open `espibright.ino` — Arduino IDE will pick up all files in the folder
5. Select board: **M5Stick-C-Plus2** (or your ESP32-C6 variant)
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

---

## Version History

| Version | Notes |
|---------|-------|
| 0.5 | Initial release. Modular refactor from monolithic sketch. Full web UI, REST API, NVS persistence, IMU orientation, schedule support. |

---

## Nesso lives 🐟

# OptiBright RF Protocol Reference

Reverse-engineered from RTL-SDR captures of the OptiBright controller remote.
All findings confirmed across 5+ independent captures unless noted otherwise.

---

## Physical Layer

- **Modulation:** OOK (On-Off Keying), 433 MHz
- **Observed timing (RTL-SDR captures):** ~300 µs (1-bit), ~150 µs (0-bit)
- **RMT implementation timing:** pulse=508µs, short_gap=508µs, long_gap=1008µs, reset_gap=6000µs
- **Burst:** each packet transmitted ×`TX_REPEAT` (default 5) in rapid succession
- **Channel commands** also append time (HMS) packets when the global time flag is enabled

---

## Packet Structure

```
Byte:  [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]
       0xD0  0x23   B3    B4    B5    B6   TYPE   CRC
```

- **`0xD0 0x23`**: fixed preamble / device address
- **B3–B6**: payload bytes — meaning depends on TYPE
- **TYPE**: encodes the command class. For channel commands, the low nibble is the
  command sub-type and the high nibble carries channel data (see TYPE 0x06 below).
  For all other packet types, the full byte is the type identifier.
- **CRC**: checksum over bytes [0]–[6]

---

## Level Encoding

All channel levels use the same scheme across B3, B4, and the high nibble of the
TYPE byte:

| Value | Meaning |
|-------|---------|
| `0x0` | Off, level not set |
| `0x1`–`0x9` | Level 1–9 (10%–90%) |
| `0xa` | Level 10 (100%) |

The high bit of B3/B4/B5 is the **on/off flag**:
- `0x80` | level = channel **on** at that level
- `0x00` | level = channel **off** (level remembered for next on)

**Level range is confirmed 0x1–0xa (1–10, 10%–100%).** Level 0xa = 100% is valid
and routinely seen in captures.

---

## Packet Types

### TYPE 0x06 — Set All Channels Now (sole live control packet)

The only packet type used to change live channel state. Covers everything from
full brightness to all-off — there is no separate all-off command type.
Transmitted ×3, optionally followed by ×3 HMS time packets.

**TYPE byte encoding:**
```
TYPE byte = (rgb_level << 4) | 0x06
```

The low nibble `0x06` identifies this as a "set all channels" command.
The high nibble carries the RGB channel's brightness level (0x1–0xa).
When RGB is off, the high nibble carries the dominant active level
(max of white and blue levels, or 0xa as fallback).

**Example:** White=5, Blue=8, RGB=on@10 → TYPE = `0xa6` (rgb_level=10, cmd=06)

```
B3 = white channel  — 0x80|level (on) or 0x00|level (off)
B4 = blue channel   — same encoding
B5 = RGB state      — 0x80|color (on) or 0x00|color (off)
B6 = cycle speed    — one-hot (see below); 0x00 when RGB off
TYPE = (rgb_level << 4) | 0x06
```

**RGB color presets (B5 low nibble):**

| Nibble | Color   |
|--------|---------|
| `0x1`  | Blue    |
| `0x2`  | Green   |
| `0x3`  | White   |
| `0x4`  | Red     |
| `0x5`  | Orange  |
| `0x6`  | Purple  |
| `0x7`  | Pink    |
| `0x8`  | Yellow  |
| `0x9`  | Rainbow (cycles through colors) |

**Cycle speed encoding (B6) — Rainbow only:**

| Value  | Duration |
|--------|----------|
| `0x01` | Static (no cycle) |
| `0x02` | 3 seconds per colour |
| `0x04` | 4 seconds per colour |
| `0x08` | 5 seconds per colour |

One-hot encoding. B6 = `0x00` when RGB is off. Cycle is only meaningful
when color = Rainbow (0x9); for solid colors, use `0x01`.

---

### TYPE 0x01 — Time Set (HMS)

The only packet type used to sync the device clock. Carries hours, minutes,
and seconds. Sent ×3, standalone — never mixed into the schedule burst.

```
B3 = HH  (0–23)
B4 = MM  (0–59)
B5 = SS  (0–59)
B6 = 0x00
TYPE = 0x01
```

There is no separate "Time HM" packet type. TYPE 0x01 is the sole time
mechanism; if seconds are not known, send `0x00` for B5.

---

## Schedule Packets

The schedule is sent as a fixed sequence of 7 types, repeated **×3**, with no
time packet appended. Sending schedule does **not** implicitly sync the clock —
use a separate time send.

**Implementation sequence (×repeatCount):**
```
02 → 03 → 04 → 05 → 07 → 08 → 09
```

**Note on ordering:** Early RTL-SDR captures suggested TYPE 02 came last (03→04→05→07→08→09→02).
The implementation sends it first. Both orderings have been used; the device appears to accept either.

To disable a slot, set HH = MM = `0x00`.

### Schedule Slot Encoding

```
B3 = HH  (hour, 0–23; or 0x00 to disable)
B4 = MM  (minute, 0–59; or 0x00 to disable)
B5 = state byte (0x00 for W/B slots; rgb_state byte for RGB slots)
B6 = 0x00
```

---

### TYPE 03 — White OFF

White channel turn-off event.

```
B3=HH, B4=MM, B5=0x00, B6=0x00
```

### TYPE 04 — White ON

White channel turn-on event.

```
B3=HH, B4=MM, B5=0x00, B6=0x00
```

### TYPE 05 — Blue ON

Blue channel turn-on event.

```
B3=HH, B4=MM, B5=0x00, B6=0x00
```

### TYPE 07 — Schedule RGB State Set

Sets the target RGB color that the device will use when the schedule fires.
Does **not** represent a timed event — carries no HH/MM.

```
B3 = target rgb_state byte (same value as TYPE 08 B5 / rgbOn.state)
     high nibble: 0x8 = on, 0x0 = off
     low nibble:  color preset (0x1–0x9)
     e.g. 0x86 = purple, 0x81 = blue
B4, B5, B6 = 0x00
TYPE = 0x07
```

B3 matches the RGB ON slot's stored color (`rgbOn.state`). TYPE 07 tells the
device which color to arm for the upcoming schedule; TYPE 08/09 B5 carry the
same value as part of the timed ON/OFF events.

### TYPE 08 — RGB ON

RGB channel turn-on event, with frozen color state.

```
B3=HH, B4=MM
B5 = rgb_state frozen at last schedule save (high nibble: on/off, low nibble: color)
B6 = 0x00
```

### TYPE 09 — RGB OFF

RGB channel turn-off event. B5 carries the frozen color so the device can
restore it at the next scheduled ON event.

```
B3=HH, B4=MM
B5 = rgb_state frozen at last schedule save (same encoding as TYPE 08)
B6 = 0x00
```

### TYPE 02 — Blue OFF (schedule)

Blue channel turn-off event. Sent first in the current implementation.

```
B3=HH, B4=MM, B5=0x00, B6=0x00
```

---

## Schedule Summary Table

| Type | Channel | Action          | B3   | B5              | Position |
|------|---------|-----------------|------|-----------------|----------|
| 02   | Blue    | OFF             | HH   | `0x00`          | 1 (first)|
| 03   | White   | OFF             | HH   | `0x00`          | 1        |
| 04   | White   | ON              | HH   | `0x00`          | 2        |
| 05   | Blue    | ON              | HH   | `0x00`          | 3        |
| 07   | —       | RGB state set   | rgbOn.state    | `0x00` | 4  |
| 08   | RGB     | ON              | HH   | frozen rgb_state| 5        |
| 09   | RGB     | OFF             | HH   | frozen rgb_state| 6        |

---

## Previously Unresolved — Now Confirmed

### Level range: 1–10 (not 1–9)

The range `0x1`–`0xa` is confirmed. `0xa` = level 10 = 100%. All captures
showing `0x8a` in B3/B4 (e.g. the ALL ON packet) are White/Blue ON at 100%.

### TYPE 0x06 is the only live control packet

Confirmed: all live channel changes — brightness, colour, on/off, and all-off
— use TYPE 0x06. There is no dedicated all-off command. An all-off packet
is simply TYPE 0x06 with the on/off bits clear on all channels, retaining
the last-known levels in the low nibbles.

### TYPE byte low nibble = command type

Previously the `0x6` in the low nibble of the channel command TYPE byte was
noted as a "mystery constant". It is the command type identifier: `0x06` = 
"set all channels now". The high nibble carries the RGB brightness level.
This is why the TYPE byte for a channel command always has `0x6` in the
low nibble regardless of channel state.

### Packet `d023171f210001c54`

Previously unresolved. Now decoded as a **Time Set (HMS)** packet:

```
TYPE = 0x01 (Time Set)
B3 = 0x17 = 23  → HH = 23
B4 = 0x1f = 31  → MM = 31
B5 = 0x21 = 33  → SS = 33
B6 = 0x00
```

Appeared before a schedule burst because the app always sends a time sync
immediately before pushing a schedule. TYPE 0x02 is never used for time —
it is exclusively the Blue OFF schedule slot.

---

## Open Questions

1. **B6 cycle speed captures for 3s/4s/5s still pending.**
   Values `0x02`, `0x04`, `0x08` are inferred from app UI one-hot layout;
   direct packet captures with each speed selected would fully confirm.

2. **Independent slot behaviour.**
   Whether a schedule can omit TYPE 08 or 09 (e.g. RGB always on, no OFF
   slot) without causing device issues is untested.

---

## Capture Methodology Notes

- All captures used an RTL-SDR dongle with URH (Universal Radio Hacker)
- 433.92 MHz, OOK demodulation
- Packet boundaries identified by inter-packet silence gaps
- CRC verified by re-computing over bytes [0]–[6] and matching byte [7]

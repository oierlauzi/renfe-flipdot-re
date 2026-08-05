# I/O port map

The 8085 addresses peripherals through its separate 8-bit I/O space
(`IN`/`OUT` instructions). Peripheral *registers* are therefore **not** in the
memory map — see [memory-map.md](memory-map.md) for the memory side.

Confidence key: **[C]** confirmed from firmware + datasheet, **[L]** likely,
**[?]** guess awaiting evidence.

## Summary

| Ports | Device | Role | Conf. |
| --- | --- | --- | --- |
| `0x80-0x81` | U3 M82C51A-2 USART | External serial link (DB37) | **[C]** |
| `0xA0-0xA5` | U2 M81C55-5 I/O section | DIP switches + baud-rate timer | **[C]** |
| `0xC0-0xCF` | *unidentified* | LED matrix interface (board absent) | **[L]** |
| `0xDE` | *unidentified* | read twice only | **[?]** |
| `0xE0-0xE7` | 8155-class device | Flip-dot interface | **[L]** |
| `0xE8-0xEF` | 8155-class device | Second display bank / LED matrix | **[L]** |

## 0x80-0x81 — M82C51A-2 USART **[C]**

Standard 8251A register pair.

| Port | Access | Function |
| --- | --- | --- |
| `0x80` | R | RX data |
| `0x80` | W | TX data |
| `0x81` | R | Status register |
| `0x81` | W | Mode word / command word |

Initialisation at `0x0443` is the canonical 8251A wake-up sequence:

```
0x0443  SUB A          ; A = 0
0x0444  OUT 0x81       ; three dummy writes to sync the
0x0446  OUT 0x81       ; mode/command state machine
0x0448  OUT 0x81
0x044A  MVI A,0x40
0x044C  OUT 0x81       ; internal reset
0x044E  MVI A,0x7E
0x0450  OUT 0x81       ; MODE word
0x0452  STA 0xA005     ; ...and cache it in RAM
0x0455  MVI A,0x37
0x0457  OUT 0x81       ; COMMAND word
```

Decoded mode word `0x7E` = `0111 1110`:

| Bits | Value | Meaning |
| --- | --- | --- |
| 1:0 | `10` | baud rate factor 1/16 |
| 3:2 | `11` | 8 data bits |
| 4 | `1` | parity enable |
| 5 | `1` | even parity |
| 7:6 | `01` | 1 stop bit |

→ **8E1 async, x16 clock**.

Command words seen: `0x37` (`TxEN|DTR|RxEN|ErrReset|RTS`) and `0x17`
(same but RTS deasserted). A second init path at `0x0533` writes the mode word
from register `B` instead of a literal, i.e. **the frame format is
switch-selectable at power-on** — see [dip-switches.md](dip-switches.md).

## 0xA0-0xA5 — M81C55-5 I/O section **[C]**

The 81C55's RAM appears in memory space at `0xA000`; its I/O section is
selected by the same address decode in I/O space, giving the standard six
registers:

| Port | Access | Function | Observed use |
| --- | --- | --- | --- |
| `0xA0` | W | Command/status register | `0x00` then `0xC0` (all ports input, timer run) |
| `0xA1` | R | Port A | read-only in firmware → **input, DIP switch bank** |
| `0xA2` | R | Port B | read-only → **input, DIP switch bank**; bits 7:5 select baud rate |
| `0xA3` | R | Port C (6 bit) | read-only → **input**; bit 5 selects an alternate init path at `0x0430` |
| `0xA4` | W | Timer count LSB | `0x14` |
| `0xA5` | W | Timer count MSB | `0x40` |

Timer preload `0x4014`: mode bits `01` = continuous square wave, count =
`0x014` = 20. The 81C55 timer output is the classic way to clock an 8251, so
this divider plus the x16 baud factor sets the line rate. With a 3.072 MHz
TIMER IN this gives 3072000 / 20 / 16 = **9600 baud** — consistent, but the
actual TIMER IN frequency still has to be measured on the board. **[L]**

Because the firmware never writes A/B/C, all 22 port bits are inputs. Three
banks of DIP switches are visible in
[switches1](../hardware/photos/switches1.jpg) /
[switches2](../hardware/photos/switches2.jpg) /
[switches3](../hardware/photos/switches3.jpg), which matches.

## 0xE0-0xEF — display interface **[L]**

Two register groups initialised by `FUN_ram_03F2` in a way that matches the
81C55 register layout exactly (offset 0 = command, 1 = PA, 2 = PB, 3 = PC,
4 = timer LSB, 5 = timer MSB):

```
; group 1 — base 0xE0
0x03F2  A=0x00 -> 0xE4     ; timer LSB
0x03F5  A=0x46 -> 0xE5     ; timer MSB  (mode 01 continuous, count 0x600)
0x03F9  A=0xCF -> 0xE0     ; command: PA out, PB out, PC ALT4, timer start
0x03FD  A=0x80 -> 0xE1, 0xE2
        NOP NOP NOP
0x0406  A=0x00 -> 0xE1, 0xE2, 0xE3
0x040D  A=0x00 -> 0xA0
0x040F  IN 0xE7 ; == 0xFF ?  -> skip group 2 entirely
; group 2 — base 0xE8, only if 0xE7 did not read 0xFF
0x0417  A=0x00 -> 0xEC
0x0419  A=0x46 -> 0xED
0x041D  A=0xCF -> 0xE8
0x0421  A=0x03 -> 0xEB
0x0425  A=0x00 -> 0xEA
0x0428  A=0xFF -> 0xE9
0x042C  A=0xE0 -> 0xE7
```

Runtime write counts (`0xE1`: 33, `0xE2`: 39, `0xE3`: 39) put the hot loop on
ports A/B/C of group 1 — the flip-dot row/column drive. The `IN 0xE7`
presence test is a **board-detect for the second display bank**, which is why
the unit still works with the LED matrix absent.

`0xE6/0xE7` fall outside the six 81C55 registers, so they are probably a
separate latch or buffer sharing the decode window. **[?]**

## 0xC0-0xCF — probable LED matrix interface **[L]**

`0xC0`-`0xC5` are each written exactly once (an init sequence), then only
`0xCD` (7 W / 6 R), `0xCE` (2 W) and `0xCF` (3 W) are touched. Low traffic is
what you would expect from the interface to the LED matrix panel this unit
does not have.

## Open questions

- [ ] Measure the 81C55 TIMER IN frequency to pin the baud rate down.
- [ ] Identify U9 (DIP16), U10 (DIP20), U19 (DIP16, ST) and U20 (DIP40) and map
      them onto the `0xC0`/`0xE0`/`0xE8` decode windows.
- [ ] Trace which address lines form the `0x80` / `0xA0` / `0xC0` / `0xE0`
      chip selects (U5/U11 MC14584B + the unidentified decoder).
- [ ] Find what `IN 0xDE` reads.

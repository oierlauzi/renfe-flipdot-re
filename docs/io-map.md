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
| `0xE0-0xE5` | 8155-class device | Flip-dot interface | **[L]** |
| `0xE7` | latch on the expansion board | Memory bank + capability register | **[C]** |
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
| `0xA3` | R | Port C (6 bit) | read-only → **input**; bit 5 selects an alternate init path at `0x0430` **and gates serial reception in the RST5.5 ISR** |
| `0xA4` | W | Timer count LSB | `0x14` |
| `0xA5` | W | Timer count MSB | `0x40` |

Timer preload `0x4014`: mode bits `01` = continuous square wave, count =
`0x014` = 20. The 81C55 timer output clocks the 8251A, so this divider plus the
x16 baud factor sets the line rate: `baud = TIMER_IN / (count x 16)`.

The crystal is **6.144 MHz**, so the 8085's CLK OUT is 3.072 MHz and the ladder
of dividers (20/40/80/160/320/640) yields exactly **9600, 4800, 2400, 1200, 600
and 300 baud** — all six standard, with zero error. **[C]**
See [dip-switches.md](dip-switches.md#confirmed-by-the-crystal-c).

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
ports A/B/C of group 1 — the flip-dot row/column drive.

## 0xE7 — expansion memory bank register **[C]**

`0xE7` falls outside the six 81C55 registers and is a **separate latch on the
expansion board**, serving two purposes:

* **Read** — a capability/status register. `0xFF` means *the expansion board is
  absent*. When present, individual bits carry configuration: `0x112E` tests
  bit 4.
* **Write** — selects the memory page visible through the `0xE000-0xFFFF`
  window. `0x042C` writes `0xE0`, the base page.

All banked access goes through three accessor functions, which between them
account for 46 call sites:

| Address | Operation | Callers |
| --- | --- | --- |
| `0x1B69` | read `A = (HL)` | 21 |
| `0x1B95` | write `(HL) = A` | 21 |
| `0x1BC4` | compare `A` with `(HL)` | 4 |

All three have the same shape:

```
        IN   0xE7
        CPI  0xFF
        JZ   plain_access      ; board absent -> access (HL) directly
        LDA  0xA05A            ; re-entrancy flag (set inside the ISRs)
        CPI  0x01
        JZ   save_restore      ; ...then also save/restore the previous page
        MOV  A,H
        STA  0xA059            ; remember the page
        OUT  0xE7              ; select page = high byte of the target address
        ORI  0xE0
        MOV  H,A               ; force the address into the 0xE000 window
plain_access:
        MOV  A,(HL)            ; (or MOV (HL),A / CMP (HL))
        RET
```

`ORI 0xE0` preserves the low 5 bits of `H`, so the CPU's `A12-A8` still carry
part of the offset while the latch supplies the higher bits. **[L]**

Two consequences worth being explicit about:

1. **There is no handshake.** Every one of the eleven `IN 0xE7` sites is an
   immediate conditional branch. No site loops, retries, waits or times out, and
   there is no error path. **[C]**
2. **Absence is handled by design, not by accident.** At `0x0675` the firmware
   goes further: with the board absent it reads **DIP switch port A bit 6**
   instead, substituting a switch for a value the expansion board would
   otherwise have supplied. **[C]**

## 0xC0-0xCF — unidentified, serviced by the watchdog tick **[?]**

`0xC0`-`0xC5` are each written exactly once (an init sequence in the 81C55
register order), then only `0xCD` (7 W / 6 R), `0xCE` (2 W) and `0xCF` (3 W) are
touched at runtime.

**Earlier this was labelled "LED matrix interface, board absent". That is now
doubtful.** The RST7.5 handler at `0x02A9` reads `0xCD` on every tick and runs
the watchdog, which points at a device that is *present* — the firmware has no
absence check for `0xC0` anywhere, unlike the careful probing it does for `0xE7`.

Either the device is on the motherboard and does something else entirely, or
RST7.5 simply never fires in this unit (its counter is decremented inside the
handler, so a never-firing RST7.5 is harmless). **Distinguishing these two needs
one scope probe on the 8085's RST7.5 pin.**

## Open questions

- [ ] Measure the 81C55 TIMER IN frequency (or the 8085 CLK OUT) to pin the
      whole baud ladder down. One measurement, six answers.
- [ ] Does RST7.5 actually fire? Decides whether the `0xC0` device is present.
- [ ] Identify U9 (DIP16), U10 (DIP20), U19 (DIP16, ST) and U20 (DIP40) and map
      them onto the `0xC0`/`0xE0`/`0xE8` decode windows.
- [ ] Trace which address lines form the `0x80` / `0xA0` / `0xC0` / `0xE0`
      chip selects (U5/U11 MC14584B + the unidentified decoder).
- [ ] Find what `IN 0xDE` reads.

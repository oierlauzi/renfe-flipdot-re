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
| `0xC0-0xCF` | clock board | HH:MM digits, plus a time-setting keypad on `0xCD` | **[C]** |
| `0xDE` | *unidentified input* | Active-low hardware status; sampled every RST7.5 tick | **[L]** |
| `0xE0-0xE5` | 8155-class device (U20) | Flip-dot interface — but see the ALT4 contradiction | **[?]** |
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
ports A/B/C of group 1 — the flip-dot drive.

### The ALT4 problem: this may not be an 8155 after all **[!]**

The register *layout* argues strongly for an 8155. The *command word* argues
against it, and the conflict is unresolved.

Only two command words are ever written to `0xE0`:

| Site | Value | PA | PB | PC mode | Timer |
| --- | --- | --- | --- | --- | --- |
| `0x03FD` | `0xCF` | out | out | **ALT4** | start |
| `0x06EC` | `0x0D` | out | **in** | **ALT4** | NOP |

Both select **ALT4**, which on an 8155 commits all six port C pins to handshake
duty — `PC0/PC1/PC3/PC4` outputs driven by the handshake logic, `PC2/PC5`
*inputs*. None is software-settable.

But the flip-dot driver needs software control of exactly `PC4` (shift clock) and
`PC5` (serial data), reading and writing `0xE3` on every bit. On a genuine 8155 in
ALT4 that is impossible: `PC4` is not software-driven and `PC5` is an input.

Note also that `0x06EC` configures **PB as an input**, which no part of the
flip-dot interpretation accounts for.

The most economical explanations are that U20 is a DIP40 with 8155-compatible
register offsets but a general-purpose port C, or that `0xE3` is a discrete latch
rather than the 8155's port C — which is less unlikely than it sounds, given that
`0xE7` is already known to be a separate latch inside this same address range.

**Everything derived from what the firmware writes is unaffected**; what is in
question is whether those writes reach ribbon pins as a clock and data line. See
[flipdot-interface.md](flipdot-interface.md#a7-has-a-contradiction-in-it-and-it-is-not-resolved-)
for the full argument and what settles it.

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

So `HL` is best read as a **flat 16-bit address into the expansion**, which the
accessor splits: `H`'s top bits go to the latch as a bank number, `H`'s low 5
bits and `L` address a byte inside the 8 KiB window. Every address the firmware
actually uses has `H = 0xE0`, i.e. bank 7, offset 0 — the base page the
initialisation at `0x042C` selects. Nothing in this build ever changes banks, so
the paging is latent capability rather than something a host has to think about.

### What is in the window **[C] from emulation**

Recovered by running the firmware with `--xram board` and watching the writes.
Every boundary here is written by identifiable code, not inferred from a dump:

| Range | Size | Contents | Written by |
| --- | --- | --- | --- |
| `0xE000` | 1 | frame start char, `0x60 \| address` | `0x055A` → `0xA006` |
| `0xE001-0xE040` | 64 | **message buffer** — what a host writes | `0x0C13` per character |
| `0xE041`, `0xE042` | 2 | terminator, `0x03` twice | `0x12E0`, `0x12E4` |
| `0xE043-0xE082` | 64 | **shadow copy** — what the panel currently shows | `0x136A` per character |
| `0xE083` | 1 | terminator for the shadow | — |
| `0xE084-0xE0C3` | 64 | **second message buffer** — see below | `0x0FFA`, `0x102F`, `EOT` |
| `0xE0C4`, `0xE0C5` | 2 | terminators for the second buffer | — |
| `0xE0C6-0xE105` | 64 | **second shadow copy** | — |

### The second buffer pair is a disabled feature **[C]**

`0xE084` is a **second buffer and shadow with exactly the same `0x42` stride** as
the first pair, and the firmware substitutes one for the other wholesale:

```
0x1498  LXI  H,0xE001      ; buffer 1
0x149B  LXI  D,0xE043      ; shadow 1
0x14A3  LDA  0xA057
0x14A6  CPI  0x01
0x14A8  JNZ  0x14B1
0x14AB  LXI  H,0xE084      ; ...replaced by buffer 2
0x14AE  LXI  D,0xE0C6      ; ...and shadow 2
```

The same substitution appears at `0x1542` and `0x1703`, and `FF`/`CAN` clear from
`0xE084` instead of `0xE001` under the same condition. So `0xA057` selects which of
two displays the whole render path operates on.

**But `0xA057` can never be set in this build.** Its only writer is `0x07F2`,
guarded by:

```
0x07E9  MVI  A,0x01
0x07EB  CPI  0x01          ; ...against the value just loaded
0x07ED  JZ   0x07FB        ; therefore always taken
```

`MVI A,0x01` followed by `CPI 0x01` is unconditionally equal, so the jump always
skips the setter. Confirmed by emulation across panel types and with the serial
link disabled: `0xA057` stays `0x00` and `0x07F2` never executes. This has the
shape of a **feature patched out at build time** by replacing a real test with a
constant — the ROM is a generic multi-display build, and this unit does not get the
second display.

Consequence: `EOT`'s block-write still lands in `0xE084` (it does not consult
`0xA057`), but **nothing ever renders it**. Writing there is harmless and
invisible.

The 64-byte message buffer and its 64-byte shadow are the mechanism behind the
differential repaint: `0x124E` walks the two in step and calls the panel driver
only where they differ. The terminator matters — both render passes end on
whichever byte equals `0xA007`, not on a count.

That the buffer is **64 columns** and the panel address counter also spans
**64 positions** ([flipdot-interface.md](flipdot-interface.md)) is one fact, not
two: one buffer column per panel address.

Two consequences worth being explicit about:

1. **There is no handshake.** Every one of the eleven `IN 0xE7` sites is an
   immediate conditional branch. No site loops, retries, waits or times out, and
   there is no error path. **[C]**
2. **Absence is handled by design, not by accident.** At `0x0675` the firmware
   goes further: with the board absent it reads **DIP switch port A bit 6**
   instead, substituting a switch for a value the expansion board would
   otherwise have supplied. **[C]**

## 0xC0-0xCF — a clock, with keys **[C]**

**This window is a clock board**, not an LED matrix interface, and none of
`0xC0`-`0xC5` is touched by the init path at all.

| Port | Direction | Contents |
| --- | --- | --- |
| `0xC5` | R/W | hours **tens** digit, validated `0-2` |
| `0xC4` | R/W | hours **units** digit, validated `0-9` |
| `0xC3` | R/W | minutes **tens** digit, validated `0-5` |
| `0xC2` | R/W | minutes **units** digit, validated `0-9` |
| `0xC1`, `0xC0` | W | only ever written `0`, and only by `clock_reset` |
| `0xCD` | W | command: `5` = latch, `0` = release, `0x0C` = ? |
| `0xCD` | R | bits 3 and 1 busy; bits 7:4 and 2 are key inputs |
| `0xCE` | W | control, only ever `0x0A` |
| `0xCF` | W | control, written `1` then `5` then `4` during a reset |

`clock_read_time` (`0x2A8D`) commands the board, waits for the busy bits to
clear, reads the four digits, ORs `0x30` into each and stores them as **four
ASCII characters**. Its caller passes `HL = 0xE001` — `msg_buffer` — so the time
lands in the first four display positions, and the routine then writes `0x23`
over the matching four shadow bytes to force a repaint. **The panel shows the
time as `HHMM` in characters 0-3.**

Four independent things make the reading certain: the digit ranges are exactly
`0-2 / 0-9 / 0-5 / 0-9`; `clock_inc_minute` carries at `'9'+1` and wraps the tens
at `'6'`; `clock_inc_hour` carries the same way and forces `00` when the hours
reach `24`; and the two are validated a *second* time, independently, on the way
back out in `clock_write_time`.

**Leading-zero blanking, and a self-consistency check on the above.**
`clock_read_time` replaces an hours-tens digit of `'0'` with `' '` — except for
panel families 3 and `0x0A`, which get `'?'`. `clock_inc_hour` then tests the
incremented tens digit against `0x21` and `0x40`, which are precisely `' '+1` and
`'?'+1`, and maps both to `'1'`. Two routines 350 bytes apart agree on a detail
that only makes sense if the digit really is a blanked hours tens.

**`0xCD` also carries keys.** The RST7.5 handler reads it every tick, masks
`0xF4`, and when the value changes starts a 201-tick debounce; on expiry it posts
`1` in `clock_cmd_mailbox`. `clock_service` picks that up and dispatches on the
debounced byte: `0xD0` adds a minute, `0xE0` adds an hour, `0xC0` resets, `0x70`
and `0xB0` toggle latched modes. **This is a time-setting keypad**, and it is why
RST7.5 reading `0xCD` never looked like a display access.

Still open: what `0xC1`/`0xC0` hold — seconds is the obvious guess, but the
firmware only ever zeroes them, so the code cannot say. Likewise `0xCE`/`0xCF`
are written fixed constants and never read.

## Open questions

- [ ] Measure the 81C55 TIMER IN frequency (or the 8085 CLK OUT) to pin the
      whole baud ladder down. One measurement, six answers.
- [ ] Is the clock board on this unit at all? `clock_service` returns
      immediately when `IN 0xCD` reads `0xFF`, so an open bus disables the whole
      feature — and the ribbon would have to reach a keypad and four more digits.
- [ ] What do `0xC0` and `0xC1` hold? Seconds fits, but the firmware only ever
      writes zero to them, so the ROM cannot answer it.
- [ ] Identify U9 (DIP16), U10 (DIP20) and U20 (DIP40) and map them onto the
      `0xC0`/`0xE0`/`0xE8` decode windows.
- [ ] Trace which address lines form the `0x80` / `0xA0` / `0xC0` / `0xE0`
      chip selects (U5/U11 MC14584B + the unidentified decoder).
- [ ] **Find what `IN 0xDE` reads — bit by bit.** Its role is now known even if
      its meaning is not: `0x19FC`, called from the RST7.5 tick at `0x02C0`,
      samples the port, complements it (so it is **active low**) and ORs it into
      the sticky accumulator `0xA024`. The `BS` command reports that accumulator
      and clears it, or with ROM `0x008B` non-zero reports the port directly.
      So this is the unit's **hardware status/fault input**, readable over the
      serial link. Temperature, door, lamp or supply monitoring would all fit a
      station sign. Probing the pins of whatever drives it, and grounding them one
      at a time while watching `BS`, would map the bits directly.
      → [serial-protocol.md](serial-protocol.md#bs-returns-one-status-byte-and-reading-clears-it-c)

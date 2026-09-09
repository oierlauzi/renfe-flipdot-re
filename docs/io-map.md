# I/O port map

The 8085 addresses peripherals through its separate 8-bit I/O space (`IN` and
`OUT`). Peripheral registers are therefore not in the memory map — see
[memory-map.md](memory-map.md) for the memory side.

## Summary

| Ports | Device | Role |
| --- | --- | --- |
| `0x80-0x81` | U3 M82C51A-2 USART | External serial link to the DB37 |
| `0xA0-0xA5` | U2 M81C55-5 I/O section | DIP switches and the baud-rate timer |
| `0xC0-0xCF` | Clock board | `HH:MM` digits, plus a time-setting keypad on `0xCD` |
| `0xDE` | Status input | Active-low hardware status, sampled every RST7.5 tick |
| `0xE0-0xE5` | U20 OKI M81C55-5 | Flip-dot interface |
| `0xE7` | Latch on the expansion board | Memory bank and capability register |
| `0xE8-0xEF` | Second 81C55-class device | Second display group; initialised but never painted on this unit |

## How the map is decoded

U9 is a **CD74HC138E**, a 3-to-8 line decoder. Its eight active-low outputs
account for the whole I/O map if it decodes **A6:A4** with **A7 as an enable**:

| Output | Block | Device |
| --- | --- | --- |
| `Y0` | `0x80-0x8F` | U3 USART |
| `Y1` | `0x90-0x9F` | unused |
| `Y2` | `0xA0-0xAF` | U2 M81C55-5 |
| `Y3` | `0xB0-0xBF` | unused |
| `Y4` | `0xC0-0xCF` | clock board |
| `Y5` | `0xD0-0xDF` | status input at `0xDE` |
| `Y6` | `0xE0-0xEF` | display group, `0xE7`, and the second group |
| `Y7` | `0xF0-0xFF` | unused |

Every one of the 31 ports the firmware touches lands in exactly one block, no
block holds two unrelated devices, and the firmware never touches a port below
`0x80` anywhere in the ROM — which is what tying `A7` to an enable would force.
Decoding `A7:A5` instead would put `0xC0-0xCF` and `0xD0-0xDF` on the same
output, and the clock board and the status input need separate selects.

The decoder's pins have not been traced. Its other two enables presumably carry
`IO/M` and a read/write strobe, probably via the MC14584B Schmitt triggers at U5.
The sub-division inside `Y6` happens downstream: `0xE0-0xE5` (U20), `0xE7` (a
separate latch) and `0xE8-0xED` (the second group) share one select, so something
on the expansion or daughter 2 decodes `A3:A0`.

## 0x80-0x81 — M82C51A-2 USART

A standard 8251A register pair.

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

Mode word `0x7E` = `0111 1110` decodes as 8 data bits, even parity, 1 stop, x16
baud factor. The command words used are `0x37` (`TxEN|DTR|RxEN|ErrReset|RTS`) and
`0x17`, the same with RTS deasserted.

That literal path runs only when DIP switch port C bit 5 is clear. Otherwise a
second path at `0x0533` builds the mode word from the switches — see
[dip-switches.md](dip-switches.md).

## 0xA0-0xA5 — M81C55-5 I/O section

The 81C55's RAM appears in memory space at `0xA000`; its I/O section occupies six
registers in I/O space:

| Port | Access | Function | Observed use |
| --- | --- | --- | --- |
| `0xA0` | W | Command/status register | `0x00` then `0xC0`: all ports input, timer running |
| `0xA1` | R | Port A | DIP switch bank: address, protocol select, render options |
| `0xA2` | R | Port B | DIP switch bank: baud rate and character format |
| `0xA3` | R | Port C (6 bit) | DIP switches; bit 5 selects the protocol family |
| `0xA4` | W | Timer count LSB | from the baud table |
| `0xA5` | W | Timer count MSB | from the baud table |

The 81C55 timer output clocks the 8251A, so the preload sets the line rate:
`baud = TIMER_IN / (count x 16)`. The crystal is 6.144 MHz, the 8085 halves it,
so TIMER IN is 3.072 MHz and the ladder of dividers 20/40/80/160/320/640 yields
exactly 9600, 4800, 2400, 1200, 600 and 300 baud with zero error. See
[dip-switches.md](dip-switches.md#port-b-bits-75--line-rate).

The firmware never writes ports A, B or C, so all 22 bits are inputs. Three banks
of DIP switches are visible in
[switches1](../hardware/photos/switches1.jpg),
[switches2](../hardware/photos/switches2.jpg) and
[switches3](../hardware/photos/switches3.jpg).

## 0xE0-0xEF — display interface

`init_display_ifaces` (`0x03F2`) sets up two register groups, both in the 81C55
layout — offset 0 command, 1 PA, 2 PB, 3 PC, 4 timer LSB, 5 timer MSB:

```
; group 1 — base 0xE0
0x03F2  A=0x00 -> 0xE4     ; timer LSB
0x03F5  A=0x46 -> 0xE5     ; timer MSB  (mode 01 continuous, count 0x600)
0x03F9  A=0xCF -> 0xE0     ; command: PA out, PB out, PC ALT2 (all out), timer start
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

Ports A, B and C of group 1 carry the flip-dot drive; group 2 is never painted on
this unit. See [flipdot-interface.md](flipdot-interface.md).

### Port C is in ALT2, all six pins outputs

U20 is an OKI M81C55-5, the same part as U2. Its command words decode against the
[OKI datasheet](https://docs.rs-online.com/1f1a/0900766b800264a3.pdf), page 133:

```
 7    6    5    4    3    2    1    0
TM2  TM1  IEB  IEA  PC2  PC1   PB   PA        bits 3:2 = 00 ALT1, 11 ALT2,
                    \____/                                01 ALT3, 10 ALT4
                    PC mode
```

| Site | Value | Binary | PA | PB | bits 3:2 | PC mode | Timer |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `0x03F9` → `0xE0` | `0xCF` | `1100 1111` | out | out | `11` | ALT2 | start |
| `0x06EC` → `0xE0` | `0x0D` | `0000 1101` | out | **in** | `11` | ALT2 | NOP |
| `0x041D` → `0xE8` | `0xCF` | `1100 1111` | out | out | `11` | ALT2 | start |
| `0x0400` → `0xA0` (U2) | `0xC0` | `1100 0000` | in | in | `00` | ALT1 | start |

The datasheet's Port Control Assignment Table gives ALT2 as `PC0`-`PC5` output
port, every pin. So `PC4` (shift clock) and `PC5` (serial data) are ordinary
software-driven outputs, and `PC0`/`PC1` are the attribute lines. U2's `0xC0`
selects ALT1, all inputs, consistent with its port C carrying DIP switches.

Why `0x06EC` configures PB as an input is not known.

## 0xE7 — expansion memory bank register

`0xE7` falls outside the six 81C55 registers and is a separate latch on the
expansion board, serving two purposes:

* **Read** — a capability register. `0xFF` means the expansion board is absent.
  When present, individual bits carry configuration; `0x112E` tests bit 4.
* **Write** — selects the memory page visible through the `0xE000-0xFFFF` window.
  `0x042C` writes `0xE0`, the base page.

All banked access goes through three accessors, 46 call sites between them:

| Address | Operation | Callers |
| --- | --- | --- |
| `0x1B69` | read `A = (HL)` | 21 |
| `0x1B95` | write `(HL) = A` | 21 |
| `0x1BC4` | compare `A` with `(HL)` | 4 |

All three share one shape:

```
        IN   0xE7
        CPI  0xFF
        JZ   plain_access      ; board absent -> access (HL) directly
        LDA  0xA05A            ; re-entrancy flag, set inside the ISRs
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
part of the offset while the latch supplies the higher bits. `HL` is therefore a
flat 16-bit address into the expansion, which the accessor splits: `H`'s top bits
become a bank number, and `H`'s low 5 bits with `L` address a byte inside the
8 KiB window.

Every address this build uses has `H = 0xE0`, the base page selected at `0x042C`.
Nothing ever changes banks, so the paging is latent capability rather than
something a host has to think about.

There is no handshake: all eleven `IN 0xE7` sites are immediate conditional
branches, with no loop, retry, wait, timeout or error path. Absence is handled by
design — at `0x0675` the firmware reads DIP switch port A bit 6 in place of a
value the expansion board would otherwise have supplied.

### What is in the window

Recovered by running the firmware with `--xram board` and watching the writes.
Every boundary is written by identifiable code:

| Range | Size | Contents | Written by |
| --- | --- | --- | --- |
| `0xE000` | 1 | frame start char, `0x60 \| address` | `0x055A` → `0xA006` |
| `0xE001-0xE040` | 64 | **message buffer** — what a host writes | `0x0C13` per character |
| `0xE041`, `0xE042` | 2 | terminator, `0x03` twice | `0x12E0`, `0x12E4` |
| `0xE043-0xE082` | 64 | **shadow copy** — what the panel currently shows | `0x136A` per character |
| `0xE083` | 1 | terminator for the shadow | — |
| `0xE084-0xE0C3` | 64 | second message buffer | `0x0FFA`, `0x102F`, `EOT` |
| `0xE0C4`, `0xE0C5` | 2 | terminators for the second buffer | — |
| `0xE0C6-0xE105` | 64 | second shadow copy | — |

The message buffer and its shadow are the mechanism behind the differential
repaint: `0x124E` walks the two in step and calls the panel driver only where they
differ. Both render passes end on whichever byte equals `0xA007`, not on a count.

The buffer is 64 columns and the panel address counter spans 64 positions
([flipdot-interface.md](flipdot-interface.md)) — one buffer column per panel
address.

### The second buffer pair is disabled at build time

`0xE084` is a second buffer and shadow with the same `0x42` stride as the first
pair, and the firmware substitutes one pair for the other wholesale:

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
`0xE084` under the same condition. So `0xA057` selects which of two displays the
render path operates on.

`0xA057` can never be set in this build. Its only writer is `0x07F2`, guarded by:

```
0x07E9  MVI  A,0x01
0x07EB  CPI  0x01          ; ...against the value just loaded
0x07ED  JZ   0x07FB        ; therefore always taken
```

`MVI A,0x01` followed by `CPI 0x01` is unconditionally equal, so the jump always
skips the setter. This is a build-time option expressed as a runtime test against
a constant, a pattern used throughout the ROM
([../src/README.md](../src/README.md)). The image is a generic multi-display
build and this unit does not get the second display.

`EOT`'s block-write still lands in `0xE084` because it does not consult `0xA057`,
so writing there is harmless and invisible.

## 0xC0-0xCF — clock board and keypad

| Port | Direction | Contents |
| --- | --- | --- |
| `0xC5` | R/W | hours **tens** digit, validated 0-2 |
| `0xC4` | R/W | hours **units** digit, validated 0-9 |
| `0xC3` | R/W | minutes **tens** digit, validated 0-5 |
| `0xC2` | R/W | minutes **units** digit, validated 0-9 |
| `0xC1`, `0xC0` | W | only ever written `0`, and only by `clock_reset` |
| `0xCD` | W | command: `5` latches, `0` releases, `0x0C` unknown |
| `0xCD` | R | bits 3 and 1 are busy; bits 7:4 and 2 are key inputs |
| `0xCE` | W | control, only ever `0x0A` |
| `0xCF` | W | control, written `1` then `5` then `4` during a reset |

`clock_read_time` (`0x2A8D`) commands the board, waits for the busy bits to
clear, reads the four digits, ORs `0x30` into each and stores them as four ASCII
characters. Its caller passes `HL = 0xE001`, the message buffer, and the routine
then writes `0x23` over the matching four shadow bytes to force a repaint. **The
panel shows the time as `HHMM` in characters 0-3.**

Four things fix the reading: the digit ranges are exactly 0-2 / 0-9 / 0-5 / 0-9;
`clock_inc_minute` carries at `'9'+1` and wraps the tens at `'6'`;
`clock_inc_hour` carries the same way and forces `00` when the hours reach 24;
and both are validated a second time, independently, on the way back out in
`clock_write_time`.

Leading zeros are blanked. `clock_read_time` replaces an hours-tens digit of `'0'`
with `' '`, except for panel families 3 and `0x0A`, which get `'?'`.
`clock_inc_hour` tests the incremented tens digit against `0x21` and `0x40` —
precisely `' '+1` and `'?'+1` — and maps both back to `'1'`.

`0xCD` also carries the keypad. The RST7.5 handler reads it every tick, masks
`0xF4`, and when the value changes starts a 201-tick debounce; on expiry it posts
`1` in `clock_cmd_mailbox`. `clock_service` dispatches on the debounced byte:
`0xD0` adds a minute, `0xE0` adds an hour, `0xC0` resets, and `0x70` and `0xB0`
toggle latched modes.

What `0xC0` and `0xC1` hold is not known; seconds would fit, but the firmware only
ever zeroes them. `0xCE` and `0xCF` are written fixed constants and never read.

Whether the clock board is fitted to this unit is also unknown. `clock_service`
returns immediately when `IN 0xCD` reads `0xFF`, so an open bus disables the
feature silently.

## 0xDE — hardware status input

`0x19FC`, called from the RST7.5 tick at `0x02C0`, samples the port, complements
it — so the line is active low — and ORs it into the sticky accumulator `0xA024`.
The `BS` command reports that accumulator and clears it, or reports the port
directly when ROM `0x008B` is non-zero. It is the unit's hardware status or fault
input, readable over the serial link; what each bit means is not known.
See [serial-protocol.md](serial-protocol.md#bs-status-byte).

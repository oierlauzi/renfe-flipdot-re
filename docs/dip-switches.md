# DIP switch configuration

The firmware only ever **reads** ports A, B and C of the 81C55 (`0xA1`, `0xA2`,
`0xA3`) and writes command word `0xC0` (all ports input). Those 22 input bits
are the three DIP switch banks visible in
[switches1.jpg](../hardware/photos/switches1.jpg),
[switches2.jpg](../hardware/photos/switches2.jpg) and
[switches3.jpg](../hardware/photos/switches3.jpg).

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Port B bits 7:5 — line rate **[C]**

At `0x0474` the firmware reads port B, masks `0xE0` and selects an 81C55 timer
preload for each of six values. The preload is stored at `0xA003`/`0xA004` and
later written to `0xA4`/`0xA5`.

| `PB & 0xE0` | Branch | Preload (MSB,LSB) | Divider | Baud if TIMER IN = 3.072 MHz |
| --- | --- | --- | --- | --- |
| `0x00` | `0x04D2` | `0x42`,`0x80` | 640 | 300 |
| `0x20` | `0x04C5` | `0x41`,`0x40` | 320 | 600 |
| `0x40` | `0x04B8` | `0x40`,`0xA0` | 160 | 1200 |
| `0x60` | `0x04AB` | `0x40`,`0x50` | 80 | 2400 |
| `0x80` | `0x049E` | `0x40`,`0x28` | 40 | 4800 |
| `0xA0`, `0xC0`, `0xE0` | `0x0491` (default) | `0x40`,`0x14` | 20 | 9600 |

The top two bits of the MSB are the 81C55 timer mode (`01` = continuous square
wave in every case); the remaining 14 bits are the count.

### Confirmed by the crystal **[C]**

A **6.144 MHz crystal** sits next to the CPU on the motherboard (observed
2026-08). The 8085 halves its crystal internally, so CLK OUT is **3.072 MHz**,
and that is the conventional source for the 81C55 TIMER IN.

`baud = TIMER_IN / (divider x 16)` then gives, for every divider in the table:

| Divider | Baud | |
| --- | --- | --- |
| 20 | **9600** | exact |
| 40 | **4800** | exact |
| 80 | **2400** | exact |
| 160 | **1200** | exact |
| 320 | **600** | exact |
| 640 | **300** | exact |

**All six are exact standard rates with zero error**, which is itself strong
confirmation that the clock chain is what it appears to be — an arbitrary TIMER
IN would not land on six standard rates simultaneously.

One cheap check remains: confirm TIMER IN is fed from CLK OUT rather than from
the raw crystal, which would double every rate. Continuity from **8085 pin 37
(CLK OUT)** to **81C55 pin 3 (TIMER IN)** settles it. Then 81C55 pin 6 (TIMER
OUT) should reach the 8251A's TxC and RxC, pins 9 and 25.

## Port B bits 4:2 — character format **[C]**

`0x04EA` builds the 8251A mode word in register `B`, starting from `0x02`
(baud factor x16) and OR-ing in bits taken from port B:

| Test | Bits set | Meaning |
| --- | --- | --- |
| `B` initial `0x02` | 1:0 = `10` | x16 baud factor |
| `PB` bit 4 set (`0x04EC`) | `0x08` → 3:2 = `10` | **7 data bits** |
| `PB` bit 4 clear | `0x0C` → 3:2 = `11` | **8 data bits** |
| `PB` bit 3 clear (`0x04FE`) | none | **parity disabled** |
| `PB` bits 3:2 == `0b10` (`0x0505`) | via `0x0515` | odd parity |
| otherwise | `0x30` → 5:2 = `11` | **even parity** |

So port B is now fully accounted for: bits 7:5 baud, bit 4 word length, bits 3:2
parity. The `0x7E` literal used on the hardwired path (8E1) is what these
switches produce in their common setting.

## Port C bit 5 — SERIAL ENABLE **[C]**

This bit does far more than pick an init path. **It gates reception entirely.**

Two separate places test it:

| Site | Test | Effect |
| --- | --- | --- |
| `0x0430` (init) | bit 5 clear → `0x0437` | hardwired config: timer preload `0x4014`, 8251A mode `0x7E` (**9600 8E1**) |
| `0x0430` (init) | bit 5 set → `0x0468` | switch-derived config: baud from port B bits 7:5, mode word built at `0x0533` |
| `0x0268` (RST5.5 ISR) | bit 5 clear → return immediately | **no serial data is ever received** |

The two combine into a single meaningful setting:

| Bit 5 | Meaning |
| --- | --- |
| **clear** | **Standalone / test mode.** Hardwired 9600 8E1, and the receive ISR returns without touching the UART. The unit will not respond to anything on the serial line. |
| **set** | **Normal operation.** Baud rate and character format come from the port B switches, and reception is enabled. |

Verified in the simulator: with port C bit 5 clear, no command produces any
reply; with it set, the unit answers correctly.

**Consequence:** the documented "9600 8E1" default is the configuration of a
unit that *cannot receive*. A working installation is running whatever the port B
switches select. Read those switches before assuming a baud rate.

## Port A bits 4:0 — display address **[C]**

**Confirmed twice over**, from the listing and by experiment.

From the listing, at `0x0F69` inside the ETX handler, the firmware builds its
reply address:

```
0x0F69  IN   0xA1          ; DIP switch port A
0x0F6B  ANI  0x1F          ; keep bits 4:0
0x0F6D  MOV  C,A
0x0F6E  ANI  0x10          ; bit 4 selects the encoding
0x0F70  JZ   0x0F7C
0x0F73  MOV  A,C
0x0F74  ANI  0x0F
0x0F76  ORI  0x40          ; bit 4 set   -> 0x40-0x4F  '@'..'O'
0x0F78  MOV  C,A
0x0F7C  MOV  A,C
0x0F7D  ORI  0x30          ; bit 4 clear -> 0x30-0x3F  '0'..'?'
```

So **32 addresses, encoded as printable ASCII**.

Independently, the status frame the unit transmits carries the same five bits.
Sweeping port A in the simulator moves the last byte in lockstep:

| Port A | bits 4:0 | transmitted frame |
| --- | --- | --- |
| `0x40` | `0x00` | `18 18 1E 15 60` / `18 18 1E 15 A0` |
| `0x41` | `0x01` | `18 18 1E 15 61` / `18 18 1E 15 A1` |
| `0x45` | `0x05` | `18 18 1E 15 65` / `18 18 1E 15 A5` |
| `0x50` | `0x10` | `18 18 1E 15 70` / `18 18 1E 15 B0` |
| `0x5F` | `0x1F` | `18 18 1E 15 7F` / `18 18 1E 15 BF` |

The last byte is exactly `0x60 | (port A & 0x1F)`, with a variant at
`0xA0 | (port A & 0x1F)`. Reproduce with:

```sh
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xDF \
    --xram board --send '02 48 4F 4C 41 03'
```

## Port A bit 6 — receive protocol select **[C]**

At `0x0675`, when no expansion board is present, bit 6 sets `0xA00C` and
therefore which receive handler the RST5.5 ISR dispatches to:

| Bit 6 | `0xA00C` | Handler |
| --- | --- | --- |
| clear | 0 | `0x08B7` — simple delimited text |
| set | 1 | `0x0ACB` — the full command protocol |

A fitted expansion board forces `0xA00C = 1` regardless.

## Port A bit 5 — blank substitute character **[C]**

At `0x0664`: bit 5 set stores `0x3F` `'?'` into `0xA00B`, clear stores `0x20`
(space). `0xA00B` is the byte that handler A renders as a space — see
[serial-protocol.md](serial-protocol.md). So this switch chooses whether the
"blank" placeholder shows as `?` or as nothing.

## Port A bit 7 — render option **[L]**

Tested at `0x0753` and `0x0831`, both on display paths (`0x0831` chooses whether
to start from `0xE001`). Its exact effect needs the render path traced.

## Port A summary — fully attributed **[C]**

| Bits | Function |
| --- | --- |
| 4:0 | **display address**, 32 values |
| 5 | blank substitute: set = `'?'`, clear = space |
| 6 | receive protocol: set = handler B, clear = handler A |
| 7 | a render option, exact effect open **[L]** |

An address of **0** is special: `0x10B4` and `0x1304` test `port A & 0x1F` for
zero and take an unaddressed path, and `RS` is ignored entirely at address 0.

### The address also sets handler A's start byte

At `0x055A` the firmware computes `0x60 | (port A & 0x1F)` and stores it in
`0xA006` — **handler A's start-of-frame character**. That is the same value the
status frame carries as its last byte, so the two protocols share one addressing
scheme: handler B addresses with `GS <0x30|addr>`, handler A embeds
`0x60|addr` as its frame delimiter.

## Port C bits 4:0 — display type **[C]**

At `0x055A` the startup path reads port C, masks `0x1F`, and dispatches 32 ways
on the result — a jump per value from `0x00` to `0x1F`. Most land on a common
default (`0x060F`), but `0x02`, `0x03`, `0x0D`, `0x12`, `0x13`, `0x14` and `0x15`
have their own targets, so those are the distinct configured variants.

The same `IN 0xA3 / ANI 0x1F` dispatch appears again at `0x09FF`.

This is almost certainly a **panel model/geometry selector**, letting one
firmware serve several PI-T100 variants. Which value corresponds to this
16-character panel is unknown until the switches are read — another reason to
record their as-found positions before touching anything.

## Port B bit 0 — fill character **[L]**

At `0x0822` and `0x1351` the firmware reads port B, tests bit 0, and when set
stores `0x23` into `0xA041`, the pad/substitute character. `0x23` is the
**solid-block** glyph, so bit 0 set makes the fill a solid block rather than a
space — confirmed by watching the fill activity vanish from a panel capture with
the bit cleared. See
[flipdot-interface.md](flipdot-interface.md#a-newly-attributed-switch-bit).

## Still unmapped

Port B bit 1. **[?]**

## As found **[C] for the readings, [?] for the mapping**

Recorded 2026-08, before anything was moved. **Do not change these without
writing the new positions down.**

### The two banks beside the 81C55 — 16 switches

| Bank | As found (1 = *closed circuit*) |
| --- | --- |
| A' | `1 0 0 0 0 1 0 1` |
| B' | `0 1 1 1 1 1 0 1` |

Three unknowns sit between these digits and the port values the firmware sees:
**polarity** (a pull-up with the switch to ground means closed reads 0),
**bit order** (which end of the printed bank is bit 7), and **which bank is which
port**. That gives eight self-consistent readings:

| A'→ | bit order | polarity | PA | PB | Port A decodes as | Port B decodes as |
| --- | --- | --- | --- | --- | --- | --- |
| PA | left=b7 | closed=1 | `0x85` | `0x7D` | addr 5, proto A | 2400, 7 data, even |
| PA | left=b7 | closed=0 | `0x7A` | `0x82` | addr 26, proto B | 4800, 8 data, **no parity** |
| PA | left=b0 | closed=1 | `0xA1` | `0xBE` | addr 1, proto A | 9600, 7 data, even |
| PA | left=b0 | closed=0 | `0x5E` | `0x41` | addr 30, proto B | 1200, 8 data, **no parity** |
| PB | left=b7 | closed=1 | `0x7D` | `0x85` | addr 29, proto B | 4800, 8 data, **no parity** |
| PB | left=b7 | closed=0 | `0x82` | `0x7A` | addr 2, proto A | 2400, 7 data, odd |
| PB | left=b0 | closed=1 | `0xBE` | `0xA1` | addr 30, proto A | 9600, 8 data, **no parity** |
| **PB** | **left=b0** | **closed=0** | **`0x41`** | **`0x5E`** | **addr 1, proto B** | **1200, 7 data, even** |

### Narrowing it

Two arguments, neither decisive alone, that point at the last row.

**Polarity: closed almost certainly reads 0.** The near-universal DIP arrangement
is a pull-up on the port pin with the switch shorting to ground. That halves the
table to the four `closed=0` rows.

**Seven data bits would explain the second `Ñ`.** [font.md](font.md) records
that `0x25` holds a second `Ñ` glyph inside the 7-bit range, which was already
noted as "what a Spanish installation needs when the link is not 8-bit clean". If
port B really is set for **7 data bits**, that is not a coincidence — it is the
reason. A 7-bit link physically cannot carry `0xA5`, the CP437 `Ñ`, so the font
carries a duplicate below `0x80`. Two independent findings corroborating each
other. **[L]**

Together those favour **address 1, command protocol, 1200 baud, 7 data bits, even
parity** — a thoroughly ordinary 1990s industrial-bus configuration, and a
sensible setting for the first sign on a platform. But it remains **[?]**.

### The device will tell us, once the link is readable

No guessing is ultimately needed, because **the status frame is a read-out of the
switches themselves**:

* Its last byte is `0x60 | (port A & 0x1F)` — **port A bits 4:0 directly**, with no
  assumption about polarity or bit order.
* It is transmitted at the switch-selected **baud rate** — giving port B bits 7:5.
* Its **character format** on the wire — data bits, parity — gives port B bits 4:2.

So a single clean capture of `18 18 1E 15 <addr>` resolves almost the whole
question, and cross-checks the eight-row table above. See
[../hardware/connectors.md](../hardware/connectors.md#attempt-2-capture-one-frame-properly).

### The other three switches

| Location | Count | As found |
| --- | --- | --- |
| Motherboard, away from the banks | 1 | **open** |
| Daughter 2 | 2 | **both open** |

**16 + 1 + 2 = 19 switches against the 81C55's 22 input bits.** So the two banks
of 8 are ports A and B, and **port C is not on a DIP bank at all** — it comes from
the single motherboard switch plus hardwired links. That fits what port C does
perfectly: bits 4:0 select the **display type** and bit 5 is the **serial
enable**, both factory decisions rather than installation ones, while ports A and
B carry the address, baud rate and character format an installer needs to set.
**[L]**

One inference worth testing: the unit was in service and receiving, so port C bit
5 must have read as *enabled*. The motherboard switch was found **open**. If that
switch is bit 5, then **open must read as 1** — independently confirming the
pull-up polarity argued above. **[L]**

The two daughter-2 switches are on the flip-dot board, so they more likely feed
U20's port C (I/O `0xE3`, which the firmware reads 14 times) than the 81C55.
**[?]**

## Recording the physical state — do this first

Before flipping anything, photograph each bank and write the as-found positions
down. This is the one piece of information that is destroyed by curiosity and
cannot be recovered from the ROM.

| Bank | Port | As-found positions (1 = closed/on) | Notes |
| --- | --- | --- | --- |
| SW1 | *TODO* | *TODO* | |
| SW2 | *TODO* | *TODO* | |
| SW3 | *TODO* | *TODO* | |

Also record, for each bank, whether a closed switch reads as 0 or 1 — pull-ups
to the 81C55 inputs mean **closed usually reads 0**, which inverts every table
above.

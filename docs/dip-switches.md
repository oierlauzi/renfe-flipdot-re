# DIP switch configuration

The firmware only ever **reads** ports A, B and C of the 81C55 (`0xA1`, `0xA2`,
`0xA3`) and writes command word `0xC0`, which makes all ports input. Those 22
input bits are the three DIP switch banks visible in
[switches1.jpg](../hardware/photos/switches1.jpg),
[switches2.jpg](../hardware/photos/switches2.jpg) and
[switches3.jpg](../hardware/photos/switches3.jpg).

## This unit's configuration

Established by talking to the unit rather than by reading switches: it answers at
**1200 baud, 7 data bits, even parity, 1 stop, at display address 1**, speaking
the ASCII command protocol, and renders through the panel family `0x11` driver.

Running those facts back through the decode tables below fixes the port values:

| Port | Value | Derived from |
| --- | --- | --- |
| **A** | `?1?0 0001` | address 1 in the reply byte `0x31`; bit 6 set for handler B |
| **B** | `0101 1100` | 1200 baud, 7 data bits, even parity, 1 stop, space padding |
| **C** | `10 0011` | bit 5 set for the command protocol; bits 4:0 = 3 selects panel family `0x11` |

Port A bits 5 and 7 are not determined by anything observed so far: bit 5 chooses
the blank substitute character and bit 7 chooses reset behaviour, neither of which
has been exercised.

## Port B bits 7:5 — line rate

At `0x0474` the firmware reads port B, masks `0xE0`, and selects an 81C55 timer
preload for each of six values. The preload is stored at `0xA003`/`0xA004` and
later written to `0xA4`/`0xA5`.

| `PB & 0xE0` | Branch | Preload (MSB, LSB) | Divider | Baud |
| --- | --- | --- | --- | --- |
| `0x00` | `0x04D2` | `0x42`, `0x80` | 640 | 300 |
| `0x20` | `0x04C5` | `0x41`, `0x40` | 320 | 600 |
| **`0x40`** | `0x04B8` | `0x40`, `0xA0` | 160 | **1200** |
| `0x60` | `0x04AB` | `0x40`, `0x50` | 80 | 2400 |
| `0x80` | `0x049E` | `0x40`, `0x28` | 40 | 4800 |
| `0xA0`, `0xC0`, `0xE0` | `0x0491` | `0x40`, `0x14` | 20 | 9600 |

The top two bits of the MSB are the 81C55 timer mode — `01`, continuous square
wave, in every case; the remaining 14 bits are the count.

A 6.144 MHz crystal sits next to the CPU. The 8085 halves it internally, so CLK
OUT is 3.072 MHz, and `baud = TIMER_IN / (divider x 16)` gives all six standard
rates exactly, with zero error. An arbitrary TIMER IN would not land on six
standard rates simultaneously.

One check remains: confirming TIMER IN is fed from CLK OUT rather than from the
raw crystal, which would double every rate. Continuity from 8085 pin 37 to 81C55
pin 3 settles it, and 81C55 pin 6 should reach the 8251A's TxC and RxC, pins 9
and 25.

## Port B bits 4:1 — character format

`0x04EA` builds the 8251A mode word in register `B`, starting from `0x02` — the
x16 baud factor — and OR-ing in bits taken from port B:

| Port B bit | Value | Mode word | Meaning |
| --- | --- | --- | --- |
| 4 | set | `0x08` → bits 3:2 = `10` | **7 data bits** |
| 4 | clear | `0x0C` → bits 3:2 = `11` | 8 data bits |
| 3 | clear | none | parity disabled |
| 3 set, 2 clear | | `0x10` → bits 5:4 = `01` | odd parity |
| 3 set, 2 set | | `0x30` → bits 5:4 = `11` | **even parity** |
| 1 | clear | `0x40` → bits 7:6 = `01` | **1 stop bit** |
| 1 | set | `0xC0` → bits 7:6 = `11` | 2 stop bits |

## Port B bit 0 — fill character and render pass

The bit is read in two unrelated places and does two things.

**Fill character.** At `0x0822` and `0x1351` the firmware tests bit 0 and, when
set, stores `0x23` into `0xA041`, the pad character. `0x23` is the solid-block
glyph, so bit 0 set pads with solid blocks rather than spaces.

**Render pass.** At `0x0547` the same bit sets `0xA008`, which selects between the
paged buffer walk at `0x124E` (bit clear) and the unpaged one at `0x13F7` (bit
set). This half only takes effect under receive handler A; handler B always uses
the paged pass.

For legible text the bit must be clear, and on this unit it is.

## Port A bits 4:0 — display address

At `0x0F69`, inside the `ETX` handler, the firmware builds its reply address:

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

So there are 32 addresses, encoded as printable ASCII. A host selects a unit with
`GS <encoded address>`, and the unit puts the same encoding in the address byte of
its `ACK`/`NAK` replies — which is how this unit's address was read back as 1.

Address 0 is special: `0x10B4` and `0x1304` test `port A & 0x1F` for zero and take
an unaddressed path, and `RS` is ignored entirely at address 0.

The same five bits also form handler A's frame delimiter. At `0x055A` the firmware
computes `0x60 | (port A & 0x1F)` into `0xA006`, so the two protocols share one
addressing scheme: handler B addresses with `GS <0x30|addr>`, handler A embeds
`0x60|addr` as its start-of-frame character.

## Port A bit 5 — blank substitute character

At `0x0664`, bit 5 set stores `0x3F` (`?`) into `0xA00B`; clear stores `0x20`
(space). `0xA00B` is the byte handler A renders as a space, so this switch chooses
whether a blank placeholder shows as `?` or as nothing.

## Port A bit 6 — receive protocol select

At `0x0675`, when no expansion board is present, bit 6 sets `0xA00C` and therefore
which receive handler the RST5.5 ISR dispatches to:

| Bit 6 | `0xA00C` | Handler |
| --- | --- | --- |
| clear | 0 | `0x08B7` — delimited text |
| set | 1 | `0x0ACB` — the full command protocol |

A fitted expansion board forces `0xA00C = 1` regardless.

## Port A bit 7 — blank or preserve the panel on reset

At `0x0753` the two branches differ in what they leave in the buffer and its
shadow, which is what decides whether a repaint happens:

| Bit 7 | Code | Buffer | Shadow | Mailbox | Result |
| --- | --- | --- | --- | --- | --- |
| clear | `0x075A` | `0x20` spaces | `0x23` blocks | `0x4A` | every character differs, so the panel repaints blank |
| set | `0x0773` | unchanged | copied from the buffer | `0x4E` | nothing differs, so the dots keep their image |

`0x0831` is the same idea on the other path: bit 7 clear fills `0xE001` with 64
spaces, bit 7 set skips it.

The option exists because flip-dots are bistable and hold their image with the
power off, so a sign that reboots can either blank itself or carry on showing its
last message. This is independent of the power-on sweep, which is a hardware
effect and happens either way — see
[flipdot-interface.md](flipdot-interface.md#power-on-behaviour).

## Port C bit 5 — protocol family

This bit selects which of two unrelated serial protocols the unit speaks, and the
line settings that go with it.

| Site | Bit 5 | Effect |
| --- | --- | --- |
| `0x0430`, init | clear | hardwired line config: timer preload `0x4014`, mode `0x7E` — 9600 8E1 |
| `0x0430`, init | set | switch-derived config: baud from port B bits 7:5, mode word built at `0x0533` |
| `0x0268`, RST5.5 | clear | `JZ 0x02A3` → `CALL rx_handler_c` (`0x2C80`) |
| `0x0268`, RST5.5 | set | dispatch on `0xA00C` to handler A or B |

So bit 5 clear runs **handler C** at a hardwired 9600 8E1 — a different, binary
protocol described in
[serial-protocol.md](serial-protocol.md#handler-c) — and bit 5 set runs the ASCII
command protocol with switch-selected line settings. An installed unit could be
running either family, and this is the switch that decides, so it is worth
knowing before assuming a baud rate.

## Port C bits 4:0 — panel family

At `0x055A` the startup path reads port C, masks `0x1F`, and dispatches 32 ways,
loading a panel family number into `0xA009`. `render_char` (`0x136A`) then
dispatches on that number to one of 13 per-character drivers; the remaining values
fall through to a default of 0, for which no driver exists.

This unit's switches select **3**, which loads family `0x11` and the driver at
`0x29D7` — the serial shift-register driver described in
[flipdot-interface.md](flipdot-interface.md).

The same `IN 0xA3 / ANI 0x1F` dispatch appears again at `0x09FF`.

## The physical switches

Recorded before anything was moved. The silkscreen numbers every motherboard
switch in one series, `S1`-`S17`, and letters the two banks beside the 81C55:

| Bank | Silkscreen | As found (1 = closed circuit) |
| --- | --- | --- |
| A' | `A`, `S9-16`, with `9` at the right-hand end | `1 0 0 0 0 1 0 1` |
| B' | `B`, `S1-8`, with `1` at the right-hand end | `0 1 1 1 1 1 0 1` |

| Location | Silkscreen | Count | As found |
| --- | --- | --- | --- |
| Motherboard, away from the banks | `S17` | 1 | open |
| Daughter 2 | not transcribed | 2 | both open |

Sixteen plus one plus two is 19 switches against the 81C55's 22 input bits, so the
two banks of eight are ports A and B, and port C is not on a DIP bank at all —
it comes from `S17` plus hardwired links. That matches what port C does: panel
family and protocol family are factory decisions, while ports A and B carry the
address, baud rate and character format an installer needs to set.

**How those positions map onto the port values is not established.** Neither
polarity of the switch sense reproduces the measured configuration above. Two
partial fits are worth checking against the board: bank B' as transcribed, with a
closed switch reading 1, gives every character-format bit correctly and misses
only bit 5 of the baud field; and bank A' read in the opposite bit order, same
polarity, gives address 1 exactly. The two banks would then need opposite bit
orders, which is possible if they are physically mirrored, but a single sample
cannot establish it.

Resolving it is bench work: continuity from the `S1` end of bank `B` to the
81C55's PB0 and from the `S9` end of bank `A` to PA0 checks bit order and bank
lettering at once, since 81C55 pins 21-28 are PB0-PB7 and pins 29-36 are PA0-PA7.
An ohmmeter from one side of a switch to ground and to Vcc gives the polarity.

`S17` is a plausible port C bit 5, but its as-found open position says nothing
about polarity, because the unit receives on either setting. The two daughter-2
switches are on the flip-dot board; they cannot feed U20's port C, which is
configured entirely as outputs, so they reach either the 81C55 or something else.

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

## Port C bit 5 — protocol family select **[C]**

This bit picks **which of two unrelated serial protocols the unit speaks**, and
the line settings that go with it. It is tested in two places:

| Site | Test | Effect |
| --- | --- | --- |
| `0x0430` (init) | bit 5 clear | hardwired line config: timer preload `0x4014`, 8251A mode `0x7E` (**9600 8E1**) |
| `0x0430` (init) | bit 5 set | switch-derived config: baud from port B bits 7:5, mode word built at `0x0533` |
| `0x0268` (RST5.5 ISR) | bit 5 clear | `JZ 0x02A3` → **`CALL rx_handler_c`** (`0x2C80`) |
| `0x0268` (RST5.5 ISR) | bit 5 set | dispatch on `rx_handler_sel` to handler **A** or **B** |

| Bit 5 | Meaning |
| --- | --- |
| **clear** | **Handler C**, at a hardwired 9600 8E1. A different, binary protocol — see [serial-protocol.md](serial-protocol.md#handler-c-a-third-protocol-c). |
| **set** | **Handlers A / B**, the ASCII control-code protocol, with baud and character format from the switches. |

> **Corrected 2026-08-08.** This section previously read "SERIAL ENABLE", and
> claimed bit 5 clear made the receive ISR "return immediately" so that "no serial
> data is ever received". **That is wrong.** `0x026C` is `JZ 0x02A3`, and `0x02A3`
> is `CALL 0x2C80` — a 930-byte third receive handler that reads the UART, latches
> framing and parity errors, drives RTS through `line_turnaround`, and transmits
> replies. The unit receives perfectly well with bit 5 clear; it just does not
> speak the protocol the tooling was sending.
>
> The simulator observation behind the old claim — *"with bit 5 clear, no command
> produces any reply"* — was real. It was the **explanation** that was invented:
> the handler-B command set gets no reply because handler C is not handler B.

**Consequence for a replacement controller:** the "9600 8E1" configuration is not
a dead-end test mode, it is the line setting for the other protocol. An installed
unit could be running either family, and the switch that decides is on port C, not
port B. Read port C bit 5 first — it determines which half of
[serial-protocol.md](serial-protocol.md) applies.

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

## Port A bit 7 — blank or preserve the panel on reset **[C]**

Now decoded. At `0x0753` the two branches differ in what they leave the buffer and
its shadow holding, which is what decides whether a repaint happens:

| Bit 7 | Code | Buffer | Shadow | Mailbox | Result |
| --- | --- | --- | --- | --- | --- |
| **clear** | `0x075A` | `0x20` spaces | `0x23` blocks | `0x4A` | every character differs → **full repaint, panel blanks** |
| **set** | `0x0773` | unchanged | **copied from the buffer** | `0x4E` | nothing differs → **no repaint, the dots keep their image** |

`0x0831` is the same idea on the other path: bit 7 clear fills `0xE001` with 64
spaces, bit 7 set skips it.

So bit 7 is **"clear the display on reset" versus "preserve it"** — and that is a
genuinely useful option for this technology rather than an arbitrary flag.
Flip-dots are bistable: they hold their image with the power off, so a sign that
reboots can either blank itself or carry on showing the last message it had. The
switch picks which.

Note this interacts with the power-on sweep, which is a *hardware* effect and
happens either way — see
[flipdot-interface.md](flipdot-interface.md#power-on-behaviour-the-panel-is-zeroed-c-observed).
With bit 7 set the panel is swept and then left alone; with it clear it is swept
and then written blank.

## Port A summary — fully attributed **[C]**

| Bits | Function |
| --- | --- |
| 4:0 | **display address**, 32 values |
| 5 | blank substitute: set = `'?'`, clear = space |
| 6 | receive protocol: set = handler B, clear = handler A |
| 7 | **reset behaviour**: set = preserve the displayed image, clear = blank the panel **[C]** |

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

## Port B bit 0 — fill character *and* render pass **[C]**

The bit is read in two unrelated places, so it does two things at once.

**Fill character.** At `0x0822` and `0x1351` the firmware tests bit 0 and when set
stores `0x23` into `0xA041`, the pad/substitute character. `0x23` is the
**solid-block** glyph, so bit 0 set makes the fill a solid block rather than a
space — confirmed by watching the block fill vanish from a panel capture with the
bit cleared.

**Render pass.** At `0x0547` the same bit sets `0xA008`, which selects between the
paged buffer walk at `0x124E` (bit clear) and the unpaged one at `0x13F7` (bit
set). This half only takes effect under receive handler A; handler B always uses
the paged pass.

Practical note: **for legible text this bit should be clear.** Set, the panel pads
with solid blocks, which is what made early captures look like the firmware could
not find its glyphs. See
[flipdot-interface.md](flipdot-interface.md#a-newly-attributed-switch-bit-port-b-bit-0-does-two-things).

## Still unmapped

Port B bit 1. **[?]**

## As found **[C] for the readings, [L] for the mapping**

Recorded 2026-08, before anything was moved. **Do not change these without
writing the new positions down.**

### The two banks beside the 81C55 — 16 switches

The silkscreen numbers every motherboard switch in one series, `S1`-`S17`, and
letters the two banks:

| Bank | Silkscreen | As found (1 = *closed circuit*) |
| --- | --- | --- |
| A' | `A`, `S9-16`, with `9` at the right-hand end below | `1 0 0 0 0 1 0 1` |
| B' | `B`, `S1-8`, with `1` at the right-hand end below | `0 1 1 1 1 1 0 1` |

Three unknowns stand between those digits and the port values the firmware sees:
**polarity** (a pull-up with the switch to ground means closed reads 0), **bit
order** (which end of the printed bank is bit 7), and **which bank is which
port**. The silkscreen settles two of them, each subject to one convention:

* **Bit order — resolved.** The `9` and the `1` mark the low-numbered end of
  each bank, and both banks are marked at the same end. Taking switch *n* as bit
  *(n-1) mod 8* — the convention that makes an `S1`-`S16` series usable in an
  installation manual — the **left-hand digit above is bit 7** and the right-hand
  digit is bit 0, on both banks. **[L]**
* **Bank to port — resolved.** `A` and `B` letter two banks pressed against a
  chip whose ports are named A, B and C, and ports A and B are exactly the two
  the firmware reads for installation settings. So bank `A` is **port A**.
  **[L]** The one thing that would overturn it is the letters naming something
  other than the ports; worth noting that the *lower*-numbered switches sit on
  the bank lettered `B`, so the numbering and the lettering do not run the same
  way round.

**Only polarity is left**, and it leaves two readings:

| Polarity | PA | PB | Port A decodes as | Port B decodes as |
| --- | --- | --- | --- | --- |
| closed = 0 | `0x7A` | `0x82` | address **26**, handler **B**, `?` substitute, blank on reset | **4800**, **8 data**, **no parity**, space fill |
| closed = 1 | `0x85` | `0x7D` | address **5**, handler **A**, space substitute, preserve image | **2400**, **7 data**, **even** parity, block fill |

If the letters turn out not to name the 81C55's ports, swap the banks and it is
instead:

| Polarity | PA | PB | Port A decodes as | Port B decodes as |
| --- | --- | --- | --- | --- |
| closed = 0 | `0x82` | `0x7A` | address **2**, handler **A**, space substitute, preserve image | **2400**, **7 data**, **odd** parity, space fill |
| closed = 1 | `0x7D` | `0x85` | address **29**, handler **B**, `?` substitute, blank on reset | **4800**, **8 data**, **no parity**, block fill |

### Narrowing the polarity

Two arguments, and they point in opposite directions.

**For closed = 0.** The near-universal DIP arrangement is a pull-up on the port
pin with the switch shorting it to ground, so a closed switch reads 0. **[L]**
Independently: **both banks were found with their bit 0 closed**, and port B bit
0 set is what makes the pad character a **solid block** rather than a space.
A sign that spent its service life padding station names with blocks is the less
likely of the two, and this argument holds whichever bank turns out to be port B.
**[L]**

**For closed = 1.** That reading gives **7 data bits with even parity**, and
[font.md](font.md) records a duplicate `Ñ` at `0x25`, inside the 7-bit range —
a 7-bit link is exactly what a font needs that duplicate for, since it physically
cannot carry `0xA5`, the CP437 `Ñ`.

**How much the `Ñ` is worth here.** Less than it first looked. The font is one
ROM table serving a product family whose character format is itself a switch, so
*some* installation running 7-bit is enough to explain the duplicate — it need
not be this one. The firmware masking every protocol parameter with `ANI 0x7F`
says the same thing about the protocol as a whole. Against that, the pull-up
convention is a claim about this board specifically.

So nothing here is decisive: **the polarity is genuinely open [?]**, and with it
the address, the baud rate and the character format. It is one ohmmeter reading
away from settled.

### The device will tell us, once the link is readable

No guessing is ultimately needed, because **the status frame is a read-out of the
switches themselves**:

* Its last byte is `0x60 | (port A & 0x1F)` — **port A bits 4:0 directly**, with no
  assumption about polarity or bit order.
* It is transmitted at the switch-selected **baud rate** — giving port B bits 7:5.
* Its **character format** on the wire — data bits, parity — gives port B bits 4:2.

So a single clean capture of `18 18 1E 15 <addr>` resolves almost the whole
question, and cross-checks the tables above. See
[../hardware/connectors.md](../hardware/connectors.md#attempt-2-capture-one-frame-properly).

### The other three switches

| Location | Silkscreen | Count | As found |
| --- | --- | --- | --- |
| Motherboard, away from the banks | `S17` | 1 | **open** |
| Daughter 2 | not transcribed | 2 | **both open** |

**16 + 1 + 2 = 19 switches against the 81C55's 22 input bits.** So the two banks
of 8 are ports A and B — which the `A`/`B` silkscreen now says outright — and
**port C is not on a DIP bank at all**: it comes from `S17` plus hardwired links.
That fits what port C does perfectly: bits 4:0 select the **display type** and
bit 5 selects the **protocol family**, both factory decisions rather than
installation ones, while ports A and B carry the address, baud rate and character
format an installer needs to set. **[L]**

`S17` is a plausible port C bit 5, but its as-found **open** position says nothing
about polarity: bit 5 selects a protocol family, and the unit receives on either
setting, so both positions are consistent with the sign that was in service.

The two daughter-2 switches are on the flip-dot board, so they more likely feed
U20's port C (I/O `0xE3`, which the firmware reads 14 times) than the 81C55.
**[?]**

## What is still not recorded

The positions themselves are safe — photographed, and written down above. Three
things are still missing, and all three are bench work.

* **Polarity**, the one remaining unknown in the tables above. Powered off, ohm
  one side of a switch in each bank to ground and to Vcc; on a typical layout one
  side of all eight is commoned to a rail, and finding that rail at ground means
  **closed reads 0**, which inverts every table above.
* **The switch-to-port wiring**, if the two conventions above are to be confirmed
  rather than assumed. Continuity from the `S1` end of bank `B` to the 81C55's
  PB0, and from the `S9` end of bank `A` to PA0, checks the bit order and the
  bank lettering at once — 81C55 pins 21-28 are PB0-PB7, pins 29-36 PA0-PA7.
* **The two daughter-2 switch designators**, which have not been transcribed.

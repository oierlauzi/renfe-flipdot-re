# Flip-dot drive interface

How a character in the message buffer becomes moving dots. The path is closed end
to end and validated in emulation: glyphs sent to the virtual panel come back
legible.

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess, **[!]** known
contradiction.

## What is settled

| | |
| --- | --- |
| **Topology** | 16 independent character modules on a shared 26-conductor bus, not daisy-chained **[C]** |
| **Addressing** | encoded binary position `0x00-0x3F` on port B, active low, `0x80` = idle **[C]** |
| **Data path** | serial shift register: port C bit 4 clock, bit 5 data (active low) **[C]** |
| **Frame** | 74 bits per character: 37 idle, 1 marker, 35 glyph, 1 trailing **[C]** |
| **Update model** | differential — only changed characters are redrawn **[C]** |
| **Attributes** | port C bits 1:0, four states, set by `BEL` **[C]** |
| **Driver** | `panel_11_draw` (`0x29D7`), selected by port C bits 4:0 = 3 **[C]** |
| **Electrical** | ULN2003A sinks only; how the high side is driven is unknown **[?]** |
| **U20's identity** | the ALT4 command word contradicts the 8155 reading **[!]** |

**Reading order.** [The panel is a serial shift register](#the-panel-is-a-serial-shift-register-c-from-the-rom)
is the core; everything before it is context and everything after is either a
consequence or an open question.

## Signal chain

```
81C55-class device @ I/O 0xE0        ULN2003A x3 (U17, U18, U19)
  port A (0xE1) ─┐                   7 Darlington sinks each
  port B (0xE2) ─┼─ daughter 2 ──▶   = 21 sinks, with     ──▶ ribbon ──▶ panel
  port C (0xE3) ─┘                   free-wheel diodes                  coils
                                                                    (16 x 5 x 7)
```

Three ULN2003A packages — U17, U18 and U19 — give **21 Darlington sinks**,
against 8 + 8 + 6 = **22 output bits** from the three ports.

> That near-match was once read as evidence the ports drive the Darlington inputs
> almost one-for-one. **That reading is dead.** The panel is fed *serially* — two
> port C bits carry a 74-bit-per-character bit stream, and port B carries an
> encoded address. The bit count is a coincidence. Kept here only because the
> one-for-one idea is the obvious first guess and this says why it is wrong.

Two facts about the part constrain the topology more than the count does:

* **The ULN2003A contains free-wheel clamp diodes** (common cathode on pin 9).
  Those exist for inductive loads. Their presence is good confirmation the panel
  is driven as **coils**, not as anything resistive.
* **The ULN2003A can only sink.** It pulls a line towards ground; it cannot
  source current. A flip-dot needs current through its coil in *both* directions
  to set and to reset, so sinks alone cannot do the job. Something must supply
  the high side. The candidates:
  1. **High-side drive on the panel itself**, with the ribbon carrying only the
     low-side selects.
  2. **Two windings per dot** — a set coil and a reset coil, each returning to
     ground through its own sink from a common +24 V. This needs only sinks, and
     would explain why there are no source drivers on the board at all.
  3. A split supply, with the coil common tied to a mid-rail.

Option 2 is the tidiest fit for what is actually on the board, but it is **[?]**
until the ribbon is traced. 21 lines cannot address 560 dots directly under any
scheme, so the panel must carry its own decoding or shift registers.

## Initialisation **[C]**

`init_display_ifaces` (`0x03F2`) sets both display port groups up, and does
nothing panel-specific:

* Timer preload `0x4600` → mode `01` (continuous square wave), count `0x600` =
  1536. A second, much slower timebase than the UART one — plausibly the
  dot-pulse or refresh clock. **[L]**
* Command `0xCF` → port A output, port B output, port C in ALT4, timer running.
  (The ALT4 part is the [known contradiction](#a7-has-a-contradiction-in-it-and-it-is-not-resolved-).)
* Ports A and B are pulsed `0x80` then `0x00` with three `NOP`s in between — a
  deliberate short pulse, i.e. a **reset/home pulse to the panel**. **[L]**

`CMA` before every data `OUT` says the drive is **active low**, as Darlington
*sinks* require. **[C]**

> **Removed 2026-08-08.** This section used to present `0x210D` as "the hot path",
> and drew two conclusions from it: that `0xE2` carrying a 4-bit value meant port
> B addressed 16 characters, and that `0xA021` was a panel-width configuration
> holding 16 or 32.
>
> Both are wrong, and the first mistake caused the second. **`0x210D` is
> `panel_0a_draw` — the driver for panel family `0x0A`, not for this unit**, which
> selects family `0x11` and `panel_11_draw` at `0x29D7`. Port B carries a
> **6-bit** encoded position over `0x00-0x3F`, not a 4-bit one; family `0x0A`
> happens to mask it to four bits because *that* panel is 16 characters wide.
> `0xA021` is the phase index of `render_pass_direct`, which runs its loop twice —
> phase 0 over `msg_buffer`, phase 1 over `0xE03D` — and has nothing to do with
> width.

## Power-on behaviour: the panel is zeroed **[C] observed**

**Observation (owner, 2026-08):** on power-up a brief buzzing sound *sweeps
across the display*. No visual change on this unit, because its dots were already
all showing black.

That single observation settles several things:

* **The rest state is black**, and power-on drives every dot to it.
* **The sweep is sequential**, not simultaneous — the sound moves across the
  panel. Coils are energised in groups small enough to be heard travelling, which
  is what the current budget of three ULN2003A packages would force anyway.
* **The firmware does not know the panel state at power-on and does not try to.**
  Flip-dots are bistable, so after a power cut the physical dots are wherever they
  were left, while the frame buffer in RAM is cleared to zero. Forcing every dot
  unconditionally is the only way to make the two agree. A controller therefore
  **cannot rely on the panel retaining content across a power cycle**.
* It is audible proof the drive electronics and at least some coils work — a
  useful bring-up signal before any serial work succeeds.

### It is not reproduced in emulation **[C]**

Running the real ROM in [pit100_sim.py](../tools/pit100_sim.py) produces only
**eight** display-port writes during boot, whatever the switch settings:

```
PC=0x03FB  0xE0 <- 0xCF     command: PA/PB out, PC ALT4, timer start
PC=0x03FF  0xE1 <- 0x80     the short pulse on ports A and B
PC=0x0401  0xE2 <- 0x80
PC=0x0407  0xE1 <- 0x00
PC=0x0409  0xE2 <- 0x00
PC=0x040B  0xE3 <- 0x00
PC=0x068D  0xE1 <- 0x00     a second clear later in startup
PC=0x068F  0xE2 <- 0x00
PC=0x0691  0xE3 <- 0x00
```

No sweep, over four million instructions and with the periodic interrupt driven
at several rates. So the full-panel drive is **gated on something the model does
not provide** — most likely a hardware handshake the render path waits on, since
the `0xC0` device and the panel's own status lines are stubbed. The `0x80`-then-
`0x00` pulse at `0x03FF` remains the best candidate for the *trigger*: it may
strobe a sequencer on daughter 2 that does the sweep in hardware, which would
explain both the audible scan and the absence of per-dot writes from the CPU.

**Concrete follow-up: time the buzz.** Duration divided by 560 dots gives the
per-dot drive time directly, which is the open question below. A one-second sweep
would mean roughly 1.8 ms per dot.

## The panel's physical topology **[C] observed**

From inspecting the panel (owner, 2026-08):

* **Each character is an independent module** — sixteen of them. A single large
  matrix was never an option.
* The modules hang off a **common bus**, the ribbon. They are **not daisy-chained**:
  the ribbon passes across them without being interrupted and re-emitted per
  module.
* **No address-setting switches or links are visible on the rear of the
  modules** — the only side inspected so far (the front needs extensive
  disassembly). Whether the modules carry addressing configuration elsewhere is
  still open — see below.
* The ribbon **exits the unit** after passing all sixteen characters, and was
  spliced to remove the display. It almost certainly continued to the **second
  LED panel**.

### The module address is encoded across 64 positions, not one-hot **[C]**

Port B does not carry sixteen dedicated select lines, one per module. It takes
**far more than sixteen distinct values** — a repaint of the whole buffer walks
all 64 — which no 8-bit one-hot scheme can produce. (The exact count depends on
how many characters changed, so it is not a property worth quoting: a full `FF`
fill produces 62 values, 61 positions plus the idle code.)

Undoing the `CMA` shows what port B actually carries:

```
post-CMA, as written : 80 C2 80 C1 80 C0 80 FF 80 FE 80 FD 80 FC 80 FB ...
pre-CMA, the value   : 7F 3D 7F 3E 7F 3F 7F 00 7F 01 7F 02 7F 03 7F 04 ...
with 0x7F removed    : 3D 3E 3F 00 01 02 03 04 05 06 07 08 09 0A 0B 0C ...
```

A plain **incrementing counter over `0x00-0x3F`**, of which only 6 values happen
to be one-hot. The `0x7F` between each is now identified: it
is a literal `MVI A,0x80 / OUT 0xE2` at `0x2A5B` that **deselects** at the end of
every character. See
[how an individual character is addressed](#how-an-individual-character-is-addressed-c-from-the-rom).

**So port B is an encoded module address, spanning 64 positions.** Three things
fall into place:

* **64 positions matches the 64-byte message buffer** exactly
  ([serial-protocol.md](serial-protocol.md)). One address per buffer column.
* **The conductor budget now works.** Roughly 6-7 address bits, plus clock, data
  and control, is about 10 signals — leaving most of the 26 conductors for supply
  *and* for a downstream display, which the one-hot reading could not accommodate.
* **The LED panel fits.** It has *more* characters than the flip-dot line and
  scrolls, so it needs a share of the same bus. A 64-position encoded address
  serves both displays from one ribbon; 16 dedicated selects could not.

### Consequence: the modules must decode their own address

If the address is encoded and broadcast, each module has to know which position it
is. That requires per-module configuration — switches, straps, or a wired-in code.
**Only the rear of the modules has been inspected**, so whether that
configuration is visible externally is open; the front faces are worth looking at
specifically. **[?]**

### What that forces

The firmware inverts (`CMA`) before writing the address port, and Darlington
sinks pull their outputs low, so whatever the modules compare against, the bus
carries it **active low**. **[L]**

Consistent with observation: the type-`0x11` driver writes only `0xE2` (49
distinct values) while the type-`0x10` and `0x12` drivers write both `0xE1` and
`0xE2` — plausibly a wider address for a longer panel, using one or both ports.

### The ribbon has 26 conductors **[C]**

Counted at the splice (owner, 2026-08). 26 is a standard IDC width (13 x 2), so
the header is very likely 26-way IDC.

Revised allocation, given that the address is encoded rather than one-hot:

| Conductors | Function | Conf. |
| --- | --- | --- |
| ~6-7 | **encoded module address** (port B) | **[L]** |
| 1 | shift clock — port C bit 4 | **[C]** |
| 1 | serial data — port C bit 5 | **[C]** |
| 1-2 | strobe / control — port B bit 7, port C bits 1:0 | **[L]** |
| remainder | 24 V and returns, and whatever the LED panel needs | **[L]** |

Tracing which conductor is which: look for the **group that carries a binary
count**, the **two that toggle per bit** (clock and data), and the supply.

### This opens a plan B for the whole project

The interface is an encoded address plus a clock and a data line, so **the panel
can be driven directly**, bypassing the original electronics entirely — no serial
link, no current loop, no unresolved physical layer. An ESP32 can produce 6-7
address bits, a clock and a data line without difficulty, and the bit format is
fully decoded above.

That is not the project's stated goal, which is to drive the display *through* its
own firmware over the DB37. But it is a genuine fallback if the electrical layer
proves awkward, and it needs no modification to the display — only a different
cable onto the existing ribbon header. Worth knowing it exists. See
[controller.md](controller.md).

## The panel is a serial shift register **[C] from the ROM**

**Scope of that [C]:** confirmed for what the firmware *emits* — the register
writes, their order, and their polarity are all read directly from the ROM. Whether
those writes appear on ribbon pins as a clock and a data line depends on
assumption **A7**, which currently has an unresolved contradiction — see
[below](#a7-has-a-contradiction-in-it-and-it-is-not-resolved-).

The panel is fed **serially**, which dissolves the "21 lines cannot address 560
dots" problem entirely.

| Port | Bit | Function |
| --- | --- | --- |
| `0xE3` (port C) | 4 (`0x10`) | **shift clock** — data is clocked on the rising edge |
| `0xE3` (port C) | 5 (`0x20`) | **serial data**, **active low**: low = dot ON |
| `0xE3` (port C) | 1:0 | **display attribute lines** — see below **[C]** |
| `0xE2` (port B) | 7:0 | **character position**, inverted, bit 7 forced set |

The driver for panel type `0x11` sits at `0x29F6-0x2A5D`. Annotated:

```
0x29F6  LDA 0xA00A          ; character position
0x29F9  CMA / ORI 0x80      ; inverted, bit 7 set
0x29FC  OUT 0xE2

0x29FE  MVI B,0x25          ; 37 idle bits
0x2A00  IN 0xE3 / ANI 3 / ORI 0x20 / OUT 0xE3   ; DATA high, low bits preserved
0x2A08: IN 0xE3 / ORI 0x10 / OUT 0xE3           ; CLOCK high
0x2A0E          ANI 0x2F / OUT 0xE3             ; CLOCK low
0x2A12  DCR B / JNZ 0x2A08                      ; x37

0x2A16  ... OUT 0xE3        ; one bit with DATA LOW - the frame marker
0x2A1D  ORI 0x10 / OUT ; ANI 0x2F / OUT         ; clock it

0x2A25  MVI B,0x05          ; 5 glyph columns
0x2A27: MVI E,0x07          ; 7 rows
0x2A29  MOV A,(HL)          ; glyph column byte, HL walks the font table at 0x1C0D
0x2A2A  RLC x5              ; bit 0 (top row) -> bit 5, the DATA position
0x2A30: MOV A,D / CMA / ANI 0x20 / ORA C / OUT 0xE3   ; DATA = inverted row bit
0x2A37  ORI 0x10 / OUT ; ANI 0x2F / OUT               ; clock it
0x2A3F  MOV A,D / RRC / MOV D,A                       ; next row
0x2A42  DCR E / JNZ 0x2A30                            ; x7 rows
0x2A46  INX HL / DCR B / JNZ 0x2A27                   ; x5 columns
```

`RLC` five times followed by `RRC` per row emits the glyph byte's bits in the
order **0, 1, 2 … 6** — top row first, exactly matching the font's bit-0-is-top
layout ([font.md](font.md)).

### The frame: 74 bits per character

```
[ 37 bits DATA high ][ 1 bit DATA LOW = marker ][ 35 glyph bits ][ 1 bit high ]
```

The 35 glyph bits are 5 columns of 7 rows, top row first, **DATA low = dot on**.

**Anchor on the marker, not on the idle run.** A blank glyph is 35 high bits and
is therefore indistinguishable from idle padding; only the single low marker bit
reliably locates a frame.

### How an individual character is addressed **[C] from the ROM**

The per-character transaction is bracketed by two writes to port B, which is what
makes the addressing unambiguous:

| Step | Code | Bus |
| --- | --- | --- |
| 1 | `0x29F6`: `LDA 0xA00A / CMA / ORI 0x80 / OUT 0xE2` | **select** character `pos` |
| 2 | `0x2A00-0x2A12` | 37 idle bits, clocked |
| 3 | `0x2A16-0x2A23` | 1 marker bit, DATA low |
| 4 | `0x2A25-0x2A48` | 35 glyph bits |
| 5 | `0x2A4B-0x2A59` | 1 trailing bit, DATA high |
| 6 | `0x2A5B`: `MVI A,0x80 / OUT 0xE2` | **deselect** — a literal `0x80` |

`0xA00A` is a plain 0-based character counter, reset at `0x1260`/`0x1407` and
incremented once per character by the render loop. So the address on the bus is

```
port B = ~position | 0x80
```

i.e. **encoded binary, active low, with bit 7 forced high**. Observed for a
six-character message, with the deselect between each:

```
written : FF 80  FE 80  FD 80  FC 80  FB 80  FA 80
position :  0      1      2      3      4      5
```

`0x80` is `~0x7F`, i.e. "address 127" — outside the 64 valid positions, so it acts
as a **bus-idle / no-module-selected state** rather than addressing anything. That
is what the earlier capture recorded as a mysterious alternating `0x7F`.

### The address width is panel-type specific **[C]**

Different drivers address differently, which is worth knowing before trusting any
one of them:

| Type | Address | Glyph data |
| --- | --- | --- |
| `0x11` (`0x29D7`) | port B, 6 bits used of 8, `~pos \| 0x80` | **serial**, shifted on port C |
| `0x0A` (`0x210D`) | port B bits 3:0, `~pos & 0x0F`, then a strobe OR-ed in from `C` (`0x10` or `0x20`) | **parallel byte** on port A (`0x2113`) |

So type `0x0A` is a 16-character parallel panel and type `0x11` a serially-fed one
with a 64-position address space. Both derive the position from the same `0xA00A`
counter.

### The LED panel is probably characters 16-63 of the same buffer **[L]**

Start with what the ROM settles. **There is no split at character 16 anywhere in
it.** `render_pass_paged` walks `msg_buffer` (`0xE001`) against `shadow_buffer`
(`0xE043`) in lockstep, bumping `char_pos` by one per position, and stops on the
`03h` sentinel that `render_task` itself writes at `msg_buffer+40h`. One flat
64-entry walk, one shadow, one position counter, one port group. `cursor_col`
runs `0-63`, `DC2` wraps at 64, and `cfg_buffer_len` (ROM `0x007B`) is `40h`.
**[C]**

Confirmed in emulation: a full repaint addresses **positions 0 through 63,
contiguously, one 74-bit frame each, all on `disp0_port_a/b/c`**. (A capture will
usually show slightly fewer frames — 61 in the `FF` run — because the pass is
differential and a few shadow bytes already matched. Every position is still
walked and counted.) **[C]**

The flip-dot line is 16 characters, so 48 of those addresses go somewhere else.
Three facts make the LED panel the candidate:

1. **The ROM's other second-display mechanism is dead in this build.** It is a
   whole second port group at `0xE8-0xED` with its own buffer and shadow
   (`msg_buffer2`/`shadow_buffer2` at `0xE084`/`0xE0C6`), driven by
   `render_pass_disp1`/`disp1_paint`. `render_pass_paged` only reaches it when
   `panel_family` is 0, and this unit's switches select `0x11`. Nothing ever
   paints `0xE8-0xED` here. **[C]**
2. **This unit had an LED panel** (owner; it could not be salvaged). Given (1), it
   cannot have been driven by the second port group. **[C] observed**
3. **The flip-dot character bus leaves the enclosure** (owner), so the chain
   continues past the 16 dot modules. **[C] observed**

**So the LED panel most likely answered to positions 16-63 of this same
64-character buffer.** The gap in the argument is the one thing nobody has traced:
**that the ribbon leaving the enclosure is the ribbon that reached the LED panel.**
Until someone follows it, (3) says only that the bus goes somewhere.

If it holds, it is good news for the project's goal: a host writing a
64-character message addresses the whole sign, and `VT` cursor positioning reaches
every character of it.

One more detail fits. At `0x1290`, when the position counter completes a block of
64, the firmware inserts `C-1` extra address slots before continuing — a gap
between blocks. **`C` is frozen at 1, so the gap is zero.** That knob only makes
sense for a chain assembled from more than one 64-character block. **[C]**

### But we are guessing: three readings of positions 16-63 **[?]**

Everything above this heading is what the firmware does. Everything below is
speculation about hardware nobody has traced, and it should be read that way.

This specimen probably came from a **double-sided** assembly
([device.md](device.md#variants-of-this-generation-c-observed-in-service-by-the-owner)),
and no second unit was recovered. The owner's objection to the 16-63 reading is
worth keeping in front: **48 characters seems thin for a display meant to carry
intermediate stations and service notices** on a suburban railway.

| # | Guess | 16-63 would be | Would predict |
| --- | --- | --- | --- |
| 1 | The LED row sits on the character bus | the LED row | a 64-character message draws a response past position 15 |
| 2 | The ribbon reaches the **far side's flip-dot row** | unused | the far side answers 0-15, in parallel with the near side |
| 3 | The LED assembly **is** the `0xE7` expansion and reads the buffer itself | unanswered | nothing on the ribbon responds past 15 |

On (2): the two faces of a back-to-back sign show the same content, so the far
side would simply be the same bus continued, answering the same 0-15. That costs
no addresses and involves the firmware not at all — which is why it sits
comfortably with everything observed, and equally why it explains nothing about
16-63.

On (3): the buffer at `0xE001` lives in the window `0xE7` pages, so it is on the
expansion board, and the LED assembly is the standing candidate for that expansion
(see below). An LED assembly holding that RAM could read the same 64 characters
and render them with its own controller, leaving the flip-dot chain never needing
more than 0-15.

The "48 is too few" objection may also be softer than it looks. This firmware
**cannot scroll** — its effects are `BEL` attributes, not motion — so whatever
scrolls the station list is upstream or on the LED assembly. A host can rewrite
the line repeatedly instead; at 9600 baud a 64-character message is roughly 70 ms,
and the differential update charges a frame only for positions that changed. Under
that reading 48 characters would be a *window*, not a message-length limit. **[?]**

**One test separates all three:** power the unit with the ribbon instrumented,
send a 64-character message, and watch which addresses draw a response. Nothing
past 15 responding rules out (1) outright.

### One row of 64, and the row machinery is switched off **[C]**

Worth stating because the station-wide variant stacks 2-4 rows and this ROM looks
at first as if it supports that. `cmd_vt_position` really does compute
`msg_buffer + 40h * row`, so the arithmetic for multiple 64-wide rows is present.
But `check_buffer_bounds` clamps the write pointer at `msg_buffer + 40h` =
`0xE041`, sets `cursor_row` to 1 and latches error `30h` — **row 1 is an overflow,
not a place to put text.**

So this build is a **single row of at most 64 positions**, and the row arithmetic
is generic code carried for other members of the family. Likewise `cfg_buffer_len`
(ROM `0x007B`, `40h`) has **no direct reference in the firmware**: it is reported
to the host in the `SUB` identify block, while the code itself uses a literal
`0040h` at `0x12DC` and `CPI 40h` at `0x0C26`.

### Port C bits 1:0 are the display attribute lines **[C]**

They are the **display attribute, driven out to the panel as two hardware
lines**, and the reason the shift loop carefully preserves them is that they hold
state between glyph frames.

The path is short. `BEL <param>` stores `param & 0x07` into `0xA014`
(`0x10D0`). Whenever that value *changes*, `0x09F3` dispatches on the panel family
(port C switch bits 4:0) and, for this panel's family, reaches `0x0A82`:

```
0x0A82  LDA  0xA015        ; the attribute
0x0A85  ANI  0x03          ; low two bits only
0x0A88  IN   0xE3          ; read-modify-write port C
0x0A8A  ANI  0xFC
0x0A8C  ORA  B
0x0A8D  OUT  0xE3          ; ...bits 1:0 := attribute bits 1:0
```

Swept in emulation, `BEL 0` through `BEL 7`:

| `BEL` param | `0xA014` | port C bits 1:0 |
| --- | --- | --- |
| 0, 4 | `0x00`, `0x04` | `00` |
| 1, 5 | `0x01`, `0x05` | `01` |
| 2, 6 | `0x02`, `0x06` | `10` |
| 3, 7 | `0x03`, `0x07` | `11` |

So of the "8 display attributes", **only the low 2 bits reach this panel** — four
states on two lines. Bit 2 does something different (below).

**What those two lines do is genuinely open.** Two constraints narrow it:

* **Not brightness on the flip-dots.** The dots are mechanical and bistable;
  there is nothing to dim.
* **Not the fluorescent tube.** The tube is **not coupled to the electronics in
  any way** (owner, 2026-08) — it has its own mains feed and its own ballast, so
  the firmware cannot switch it.

What remains, in rough order of plausibility:

1. **They target the LED matrix, not the flip-dots.** The same ribbon continues to
   it, LEDs *can* be dimmed or blinked, and it is the part of the sign that
   scrolls. Handler A's timed variants fit an attention effect on scrolling text.
2. **A hardware blink or flash enable** on the flip-dot modules, with the panel
   doing the toggling rather than the CPU. The durations at `0xA016`/`0xA018`
   would then set how long the effect lasts, not its rate.
3. A per-panel enable or mode select with no visual meaning of its own.

Tracing port C bits 1:0 through the ribbon, or simply sending `BEL 0`-`BEL 3` to a
powered unit and watching, settles it. **[?]**

Other panel families route the same two bits elsewhere, which is good
corroboration that they are attribute lines rather than anything panel-specific:

| Family (port C bits 4:0) | Routine | Destination |
| --- | --- | --- |
| 2, 3, `0x0D`, `0x12`-`0x15`, `0x17` | `0x0A82` | port C bits 1:0 |
| `0x19` | `0x0A90` | port B bits 1:0 |
| `0x18` | `0x0A9E` | attribute bit 0 → port C bit 5, bit 1 → port B bit 7 |

### Bit 2 makes the effect permanent; bits 0 and 1 are timed **[C]**

`0x199F`, called after every character, ages the attribute — but **only under
receive handler A**; handler B jumps straight past it.

* Attribute **bit 0** is paired with duration `0xA018`, **bit 1** with `0xA016`.
  Both durations count down on the RST7.5 tick (`0x02EF`). When one reaches zero
  the matching attribute bit is cleared, ending the effect.
* **Bit 2 suppresses that ageing for bit 0**: the test is
  `(attr & 0x05) == 0x01`, so with bit 2 set the auto-clear never runs and the
  effect persists.
* **Bit 7** short-circuits the whole thing — attribute off.
* `0xA013` is set to 1 whenever `attr & 0x03` is non-zero, i.e. "an effect is
  currently running", and is what gates the countdown at `0x02EF`.

That decodes handler A's in-band control characters exactly:

| Character | Sets | Meaning |
| --- | --- | --- |
| `0x7C` `\|` | `attr = (attr & 0x7F) \| 0x05` | effect 0, **permanent** (bit 2) |
| `0x7D` `}` | `attr \|= 0x02` plus `0xA016` = 80 or 2000 | effect 1, **timed** |
| `0x7F` DEL | `attr = (attr & 0xFA) \| 0x80` | effects off |

Two independent findings agreeing: the timing logic explains why `0x7C` sets
*two* bits rather than one, which had no explanation before.

### An aside: the glyph lookup is a linear search

`0x29DD-0x29F3` walks the font table five bytes at a time comparing the index
against the character code, up to 255 iterations, even though the table is
directly indexable as `0x1C0D + code * 5` ([font.md](font.md)). If the code is not
found it restarts the search for character `0`. Functionally irrelevant, but it
sets the per-character cost and is a reminder that this firmware is not
performance-tuned — useful when reasoning about how fast a host can push updates.

### Validated end to end

`tools/panel_view.py` recovers the bit stream from the emulated port writes and
reconstructs the glyphs. Sending `FF` (fill with `0x23`) produces:

```
#####  #####  #####
#####  #####  #####
#####  #####  #####
#####  #####  #####      ...which is exactly the ROM's glyph for 0x23,
#####  #####  #####      a solid block rather than a hash.
#####  #####  #####
#####  #####  #####
```

Frame length, marker position, bit order and polarity all had to be right
simultaneously for that to come out, which is the strongest single confirmation
in the project so far.

```sh
PYTHONPATH=tools python3 tools/panel_view.py --fill
```

## What the render path needs before it runs at all **[C]**

Four conditions, all of which had to be satisfied before a single dot moved. This
is why earlier attempts saw only eight port writes at boot.

1. **A panel type.** Port C bits 4:0 pick a branch at `0x055A`; each branch loads
   a type into **`0xA009`**, and `0x136A` dispatches on it to a panel-specific
   driver. The **default branch sets `0xA009 = 0`, for which no driver exists** —
   so a unit with unrecognised switches drives nothing. 13 of the 32 values select
   a real driver.
2. **The RST7.5 tick.** The idle loop at `0x1675` spins while `0xA01B & 1` is set,
   and that flag is cleared in the RST7.5 handler at `0x0336`. No tick, no repaint,
   forever.
3. **Changed content.** `0x136A` is reached by **`CNZ`** — a *conditional* call
   after comparing the new byte against the shadow copy (`0x1447`:
   `MOV A,(HL) / XCHG / CMP (HL) / CNZ 0x136A`). **Only characters that differ are
   redrawn.** An all-zero buffer against an all-zero shadow flips nothing.
4. **Somewhere for the buffer to live** — see the `0xE000` window question in
   [io-map.md](io-map.md).

5. **The unit must not be selected.** The repaint task spins at `0x130B` while
   the *selected* flag is set — see
   [serial-protocol.md](serial-protocol.md#the-repaint-interlock-rs-is-not-optional-c).
   This is the one that defeated the tooling longest, because it fails silently
   and the unit still answers `ACK`.

### Two render passes, chosen by port B bit 0 **[C]**

There are two nearly identical passes over the buffer, and which one runs is a
switch setting:

| Pass | Access | Selected when |
| --- | --- | --- |
| `0x124E` | via `xram_read`/`xram_compare`, i.e. **paged** | `0xA008 = 0` |
| `0x13F7` → `0x1445` | direct `MOV A,M`, **unpaged** | `0xA008 = 1` |

`0xA008` is loaded at `0x0547` from **DIP switch port B bit 0**, and the choice
only takes effect for receive handler A — under handler B, `0x1243` diverts to
the paged pass regardless. Both passes walk the buffer and its shadow in step,
call the panel driver through `0x136A` only where the two differ, and **terminate
on the byte matching `0xA007`** rather than on a count.

### What forces a full repaint **[C]**

`0x13C6`, called at the top of every pass, clears the 64-byte shadow to zero.
Every character then differs from it, so the next pass repaints everything. It
does that when:

* the panel type is `0x08` — and it also pulses `0x36` to port C first; or
* **port C bit 5 is clear**, i.e. the unit is running the handler C protocol; or
* `0xA023 = 0xFF`.

It explicitly does **not** for panel type `0x12`, which returns immediately.

`0xA023` is set to `0xFF` at `0x07D4` for panel types **2, 8, 9, 0x0B, 0x0C, 0x0D
and 0x0E**, and left at zero for every other type — including `0x11`, this
panel's. So the firmware divides its supported displays into two classes:

* **continuously refreshed** — the whole panel is rewritten every pass;
* **differentially updated** — only changed characters are touched.

**This independently supports assumption A1.** A flip-dot panel *must* be
differential: its dots are bistable, so rewriting unchanged ones is pure coil
heating for no visual effect, and the ULN2003A free-wheel diodes confirm coils.
A continuously refreshed type is what an LED or lamp matrix needs. Type `0x11`
being in the differential class is therefore consistent with it being the
flip-dot driver, arrived at from a completely different direction than the
"most drive activity" heuristic that originally picked it.

This also explains the puzzle in point 3 above: a purely differential update
would not sweep at power-on, and the shadow clear is what forces the full
repaint that does.

## Assumptions this rests on, and how to falsify each

Everything above is derived from the ROM plus emulation. These are the inputs that
were **chosen rather than measured**, in rough order of how much would need
revisiting if one is wrong.

| # | Assumption | Why it was chosen | How to check on the hardware | If wrong |
| --- | --- | --- | --- | --- |
| A1 | Panel type is `0x11`, from port C bits 4:0 = `0x03` | It produced the most drive activity of the 13 candidates, and it falls in the *differentially updated* class, which is the only sane way to drive bistable dots (see above) | Read port C bits 4:0 from the switches and hardwired links, then look up the branch at `0x055A` | The frame layout may differ per driver. The *method* here applies to whichever driver is real; re-run `panel_view.py --type N` |
| A2 | RST7.5 fires periodically | The idle loop demands it; the rate used (every 2000 instructions) is arbitrary | Scope 8085 pin 7 for a periodic edge and measure its rate | Only the repaint *rate* changes, not the frame format |
| A3 | Port C bit 4 is the clock and bit 5 the data | The code toggles bit 4 twice per bit while bit 5 carries the varying value | Scope both against the ribbon | Would invert the whole decode; the `FF` validation makes this very unlikely |
| A4 | Data is active low (low = dot on) | `CMA` precedes every DATA write, and `FF` then renders as a solid block | Send `FF` on the real panel and see whether it goes all-on or all-off | Every rendered glyph inverts |
| A5 | The frame is 37 + 1 + 35 + 1 bits | Read directly from the loop counters `0x25` and `0x05`/`0x07` | Count clock edges per character on a logic analyser | The 37 leading bits are the least certain part; they may address a longer register chain |
| A6 | An expansion board is present (`0xE7` != `0xFF`) for some captures | Without it the buffer at `0xE000` is unbacked | The unit has no LED matrix, so `0xE7` should read `0xFF` | The buffer question in [io-map.md](io-map.md) needs settling first — and see the prediction below |
| A7 | **U20, the DIP40 on daughter 2, is an 81C55** and the register at `0xE3` is its port C | The firmware initialises `0xE0-0xE5` in exactly the 8155 register layout, and 8155s are DIP40 | **Read U20's top marking.** Then count clock edges on the ribbon: 74 per character confirms the pins follow the register writes | See the contradiction below — this one is not merely unconfirmed, it currently *conflicts* with the ALT4 command word |

### A7 has a contradiction in it, and it is not resolved **[!]**

Worth stating plainly rather than leaving implied by an **[L]** mark. The firmware
writes only two command words to `0xE0`, and **both select port C mode ALT4**:

| Site | Command | PA | PB | PC mode | Timer |
| --- | --- | --- | --- | --- | --- |
| `0x03FD` | `0xCF` | out | out | **ALT4** | start |
| `0x06EC` | `0x0D` | out | **in** | **ALT4** | NOP |

On a genuine 8155, ALT4 assigns every port C pin to handshake duty:

```
PC0 = A INTR (out)   PC1 = A BF (out)   PC2 = A STB (in)
PC3 = B INTR (out)   PC4 = B BF (out)   PC5 = B STB (in)
```

**None of them is software-controlled.** But the flip-dot decode requires software
control of exactly `PC4` as the shift clock and `PC5` as the serial data — and on
an 8155 in ALT4, `PC4` is driven by the handshake logic and `PC5` is an *input*.
Those two statements cannot both be true.

So one of these must give:

1. **U20 is not a plain 81C55.** A DIP40 with the same register offsets but
   general-purpose port C would satisfy everything. The firmware is a generic
   multi-panel build, so it may have been written against a different part.
2. **`0xE3` is not the 8155's port C** but a separate latch. This is less far
   fetched than it sounds: `0xE7` is *already established* as a discrete latch
   outside the six 81C55 registers ([io-map.md](io-map.md#0xe7--expansion-memory-bank-register-c)),
   so the `0xE0-0xEF` decode demonstrably holds more than one device. The
   difficulty is that `0xE3` sits exactly at the 8155's port C offset.
3. The ALT-mode semantics of the specific OKI M81C55-5 differ from Intel's.

**What this does and does not undermine.** Everything derived purely from what the
firmware *writes* stands regardless: the command protocol, the buffer and shadow,
the repaint interlock, the address arithmetic, the glyph bit order and polarity,
and the 74-bit frame. Those are statements about the ROM. What is in question is
only the last step — whether those register writes reach ribbon pins as a clock
and a data line. The `FF`-renders-as-a-solid-block validation does **not** settle
it, because the emulator models `0xE3` as a plain readable latch; that check
confirms the firmware's intent, not the hardware's response.

**Resolving it needs one reading and one measurement**, both already on
[TODO.md](../TODO.md): U20's top marking, and a count of clock edges per character
on the ribbon.

### A falsifiable prediction about the missing LED panel

The ribbon continuing onward to the LED panel makes the `0xE000` question much
sharper. The chain of reasoning:

1. `0xE7` is a register **on the expansion**, and reads `0xFF` when it is absent.
2. The **message buffer lives at `0xE001-0xE040`**, inside the paged window that
   `0xE7` selects.
3. The LED panel assembly is the obvious candidate for that expansion.

This is a claim about where the *memory* is, and sits alongside — not against —
the addressing argument above: the LED panel assembly would carry the buffer RAM
**and** answer to character positions 16-63 on the same bus.

If all three hold, then **this unit is missing the RAM its message buffer lives
in** — which is precisely why the emulator needs `--xram board` before any text
renders, and why `STX AB ETX` paints the fill character instead of letters.

**The prediction, testable at first power-up:** with the LED panel absent the unit
should still perform its power-on sweep and still answer serial queries such as
`SUB` — neither needs the buffer — but should be **unable to display sent text**.

If it *does* display text with the panel gone, this reasoning is wrong and the
`0xE000` memory is somewhere on the boards that are present. Either outcome is
informative, and it costs nothing beyond powering the unit up and sending one
message.

### Message text renders correctly **[C]**

Neither of the two things that gate this is in the frame format itself.
`panel_view.py --text "HOLA MUNDO"` produces:

```
   #...#  .###.  #....  .###.  #...#  #...#  #...#  ####.  .###.
   #...#  #...#  #....  #...#  ##.##  #...#  #...#  #...#  #...#
   #...#  #...#  #....  #...#  #.#.#  #...#  ##..#  #...#  #...#
   #####  #...#  #....  #####  #.#.#  #...#  #.#.#  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #..##  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #...#  #...#  #...#
   #...#  .###.  #####  #...#  #...#  .###.  #...#  ####.  .###.
```

The space between the words produces no frame at all, which is the differential
update working correctly: it was already a space, so nothing needed flipping.

**Cause 1 — the repaint interlock.** A *selected* unit does not repaint. See
[serial-protocol.md](serial-protocol.md#the-repaint-interlock-rs-is-not-optional-c);
the render task spins at `0x130B` until `selected` clears, so text written and
ACKed during a selection stays invisible until `RS`. The tooling was never
sending `RS`. This is a protocol requirement, not a hardware one.

**Cause 2 — port B bit 0.** With it set, the pad character becomes the solid
block `0x23` and the alternator at `0x1348` repaints every column with blocks —
which is exactly the "fill character" that was being observed. The block was not
a stand-in for a glyph the firmware could not find; it was the pad character
doing its job. `panel_view.py` now defaults port B to `0xFE` and exposes
`--port-b`.

Assumption A6 is unaffected and still needs the hardware: this was run with
`--xram board`, so whether *this* unit has the memory remains open.

## A newly attributed switch bit: port B bit 0 does two things

**It selects the fill/substitute character.** At `0x0822` and `0x1351` the
firmware reads port B, tests bit 0, and when set stores `0x23` into `0xA041`.
`0x23` is the solid-block glyph, so **bit 0 set makes the pad character a solid
block instead of a space**. **[L]**

**It also selects the render pass.** At `0x0547` the same bit sets `0xA008`,
which chooses between the paged pass at `0x124E` and the unpaged one at `0x13F7`
(see above). One switch bit therefore couples the pad character to the buffer
access method — odd, but both readings are direct from the ROM. **[C]**

That closes one of the two remaining unattributed port B bits — see
[dip-switches.md](dip-switches.md).

## Open questions

- [ ] Trace the 26 ribbon conductors and match them to the port bits: the group
      carrying the binary address count, the clock, the data line, and supply.
- [x] ~~Why does message text not reach the rendered glyphs?~~ **Solved** — the
      repaint interlock plus port B bit 0. See above.
- [ ] Determine the pulse width used to flip a dot, and whether the firmware
      relies on the 81C55 timer or on software delay loops. **Timing the
      power-on sweep gives this almost for free** — see above.
- [ ] Does daughter 2 contain a hardware sequencer that performs the power-on
      sweep autonomously? The CPU issues only a short pulse at `0x03FF`, yet the
      panel scans audibly.
- [x] ~~Does it repaint the whole panel or only changed dots?~~ **Both, and which
      one is determined by the panel type** — `0xA023` splits the supported
      displays into continuously refreshed and differentially updated classes.
      This panel's type `0x11` is differential. See above.
- [x] ~~Where is the frame buffer, and in what layout?~~ Not in the `0xA000` RAM
      at all: it is the 64-byte buffer at `0xE001` with a 64-byte shadow at
      `0xE043`. → [io-map.md](io-map.md#what-is-in-the-window-c-from-emulation)
- [x] ~~What is the third 64-byte region at `0xE084`?~~ **Answered: it is a second
      buffer/shadow pair** (`0xE084` + `0xE0C6`, same `0x42` stride), selected by
      `0xA057` — which is **never set**, because its only writer sits behind an
      always-taken jump. A feature patched out at build time.
      → [io-map.md](io-map.md#the-second-buffer-pair-is-a-disabled-feature-c)

## Safety note

Flip-dot coils are designed for **short pulses**. Leaving one energised
continuously will overheat and can destroy it. Any experiment that drives the
panel directly — bypassing this firmware — must guarantee a bounded pulse width
in hardware or in a watchdog, not merely in software that could hang.

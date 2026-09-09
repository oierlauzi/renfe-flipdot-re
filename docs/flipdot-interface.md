# Flip-dot drive interface

How a character in the message buffer becomes moving dots.

## Summary

| | |
| --- | --- |
| **Topology** | 16 character modules on a shared 26-conductor bus, not daisy-chained |
| **Module carrier** | modules mount in pairs on a flip-dot carrier board — 8 boards, 16 modules — each with 6 address-setting switches |
| **Addressing** | encoded binary position `0x00-0x3F` on port B, active low, `0x80` = idle |
| **Data path** | serial shift register: port C bit 4 clock, bit 5 data, active low |
| **Frame** | 74 bits per character: 37 idle, 1 marker, 35 glyph, 1 trailing |
| **Update model** | differential — only changed characters are redrawn |
| **Attributes** | port C bits 1:0, four states, set by `BEL` |
| **Driver** | `panel_11_draw` (`0x29D7`), selected by port C bits 4:0 = 3 |
| **Electrical** | ULN2003A sinks only; how the high side is driven is not established |

## Signal chain

```
U20 M81C55-5 @ I/O 0xE0              ULN2003A x3 (U17, U18, U19)
  port A (0xE1) ─┐                   7 Darlington sinks each
  port B (0xE2) ─┼─ daughter 2 ──▶   = 21 sinks, with     ──▶ ribbon ──▶ panel
  port C (0xE3) ─┘                   free-wheel diodes                  coils
                                                                    (16 x 5 x 7)
```

Two facts about the ULN2003A constrain the topology:

* **It contains free-wheel clamp diodes**, common cathode on pin 9. Those exist
  for inductive loads, which confirms the panel is driven as coils.
* **It can only sink.** It pulls a line towards ground and cannot source current,
  but a flip-dot needs current through its coil in both directions to set and to
  reset. Something must supply the high side. The candidates are high-side drive
  on the panel itself with the ribbon carrying only low-side selects; two windings
  per dot, a set coil and a reset coil each returning to ground through its own
  sink from a common +24 V; or a split supply with the coil common at a mid-rail.
  The second fits what is on the board best, since it needs only sinks, but the
  ribbon has not been traced.

Twenty-one lines cannot address 560 dots directly under any scheme, so the panel
carries its own decoding.

## Initialisation

`init_display_ifaces` (`0x03F2`) sets up both display port groups and does nothing
panel-specific:

* Timer preload `0x4600`: mode `01`, continuous square wave, count `0x600` =
  1536. This is a much slower timebase than the UART's, possibly the dot-pulse or
  refresh clock.
* Command `0xCF`: port A output, port B output, port C in ALT2 with all six pins
  outputs, timer running. See
  [io-map.md](io-map.md#port-c-is-in-alt2-all-six-pins-outputs).
* Ports A and B are pulsed `0x80` then `0x00` with three `NOP`s in between — a
  deliberate short pulse, probably a reset or home pulse to the panel.

`CMA` precedes every data `OUT`, so the drive is active low, as Darlington sinks
require.

## Power-on behaviour

On power-up a brief buzzing sound sweeps across the display. On this unit there is
no visual change, because its dots were already all showing black.

That settles several things:

* The rest state is black, and power-on drives every dot to it.
* The sweep is sequential rather than simultaneous — the sound moves across the
  panel — so coils are energised in groups small enough to be heard travelling,
  which the current budget of three ULN2003A packages would force anyway.
* **The firmware does not know the panel state at power-on and does not try to.**
  Flip-dots are bistable, so after a power cut the physical dots are wherever they
  were left while the frame buffer is cleared. Forcing every dot unconditionally
  is the only way to make the two agree, so a controller cannot rely on the panel
  retaining content across a power cycle.
* It is audible proof that the drive electronics and at least some coils work,
  which is a useful bring-up signal before any serial work succeeds.

The sweep is not reproduced in emulation. Running the ROM in
[pit100_sim.py](../tools/pit100_sim.py) produces only eight display-port writes
during boot, whatever the switch settings:

```
PC=0x03FB  0xE0 <- 0xCF     command: PA/PB out, PC ALT2 (all out), timer start
PC=0x03FF  0xE1 <- 0x80     the short pulse on ports A and B
PC=0x0401  0xE2 <- 0x80
PC=0x0407  0xE1 <- 0x00
PC=0x0409  0xE2 <- 0x00
PC=0x040B  0xE3 <- 0x00
PC=0x068D  0xE1 <- 0x00     a second clear later in startup
PC=0x068F  0xE2 <- 0x00
PC=0x0691  0xE3 <- 0x00
```

No sweep appears over four million instructions at several interrupt rates, so the
full-panel drive is gated on something the model does not provide. The
`0x80`-then-`0x00` pulse at `0x03FF` is the best candidate for a trigger: it may
strobe a sequencer on daughter 2 that performs the sweep in hardware, which would
explain both the audible scan and the absence of per-dot writes from the CPU.

Timing the buzz would give the per-dot drive time directly: duration divided by
560 dots. A one-second sweep would mean roughly 1.8 ms per dot.

## The panel's physical topology

From inspecting the panel:

* Each character is an independent module — sixteen of them. A single large matrix
  was never an option.
* **Modules mount in pairs on a shared flip-dot carrier board.** Eight carriers
  make up this unit's 16-character line. Each carrier holds the ribbon bus
  connector and the pair's address-setting switches.
* The carriers hang off a common bus, the ribbon. They are not daisy-chained: it
  passes across them without being interrupted and re-emitted per board.
* The ribbon exits the unit after the last carrier, and was spliced to remove the
  display.

### The flip-dot carrier board

[The carrier board](../hardware/photos/Flip-dot%20carrier.jpg) has **6
address-setting switches**, DIP-style in layout but each implemented as a wire
hooked across two pins, the same style as every other switch in this project. Six
matches the firmware's 6-bit module-address field (`0x00-0x3F`) exactly, and it
falls inside the six or seven conductors estimated for the encoded address from
the ROM alone.

How the two modules on one carrier get distinct addresses from a single 6-switch
setting is not established. The switches probably set a base value with the two
module sockets wired to a fixed offset, so one carrier answers to a pair of
adjacent addresses and the 6 bits still span the full 64-position range — but
nothing has been traced to confirm it.

The carrier's silkscreen reads `37624.0 DV2 A.1` / `S7D5-2` and carries the
Telenorma logo, extending Telenorma's design past the boards inside the enclosure
and onto the panel itself.

Legible on the carrier, but not traced to a function: **CD4015BE** (dual 4-bit
static shift register) x6, **CD4503BE** (hex non-inverting 3-state buffer) x1,
**CD4068BE** (8-input NAND) x1, **HEF4077BP** (quad XNOR) x2. An XNOR-then-AND
combination is the textbook shape of an address comparator, and shift registers
are what turning a serial bit stream into parallel drive needs, so the parts list
reads as address comparison plus serial-to-parallel conversion.

**The character module itself is a diode matrix, not a local shift register.**
[The back of a module](../hardware/photos/Flip-dot%20daughter%20%28back%29.jpg)
shows 35 diodes silkscreened `D11`-`D75` wired against row and column nets
labelled `X1Y1`-`X7Y5` — a plain 5-column by 7-row matrix, matching the glyph cell
exactly. The serial-to-parallel conversion is therefore on the carrier board; the
module has no logic of its own.

### The ribbon has 26 conductors

Counted at the splice. Twenty-six is a standard IDC width, 13 x 2, so the header
is very likely 26-way IDC.

| Conductors | Function |
| --- | --- |
| 6 | encoded module address on port B, matching the carrier's 6 address switches |
| 1 | shift clock, port C bit 4 |
| 1 | serial data, port C bit 5 |
| 1-2 | strobe and control: port B bit 7, port C bits 1:0 |
| remainder | 24 V and returns, and whatever the LED panel needs |

To trace which conductor is which, look for the group carrying a binary count, the
two that toggle per bit, and the supply.

### Driving the panel directly

The interface is an encoded address plus a clock and a data line, so the panel can
be driven directly, bypassing the original electronics — no serial link and no
DB37. An ESP32 can produce six or seven address bits, a clock and a data line
without difficulty, and the bit format is fully decoded below. It needs no
modification to the display, only a different cable onto the existing ribbon
header.

That is not how this project drives the sign — the point was to go through the
original firmware — but it is a genuine alternative. See
[controller.md](controller.md).

## The panel is a serial shift register

| Port | Bit | Function |
| --- | --- | --- |
| `0xE3` (port C) | 4 (`0x10`) | shift clock; data is clocked on the rising edge |
| `0xE3` (port C) | 5 (`0x20`) | serial data, active low: low = dot on |
| `0xE3` (port C) | 1:0 | display attribute lines |
| `0xE2` (port B) | 7:0 | character position, inverted, bit 7 forced set |

The driver for panel family `0x11` sits at `0x29F6-0x2A5D`:

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
order 0, 1, 2 … 6 — top row first, matching the font's bit-0-is-top layout
([font.md](font.md)).

### The frame: 74 bits per character

```
[ 37 bits DATA high ][ 1 bit DATA LOW = marker ][ 35 glyph bits ][ 1 bit high ]
```

The 35 glyph bits are 5 columns of 7 rows, top row first, with DATA low meaning
the dot is on.

Anchor a decoder on the marker, not on the idle run: a blank glyph is 35 high bits
and is indistinguishable from idle padding, so only the single low marker bit
locates a frame reliably.

### How an individual character is addressed

The per-character transaction is bracketed by two writes to port B:

| Step | Code | Bus |
| --- | --- | --- |
| 1 | `0x29F6`: `LDA 0xA00A / CMA / ORI 0x80 / OUT 0xE2` | select character `pos` |
| 2 | `0x2A00-0x2A12` | 37 idle bits, clocked |
| 3 | `0x2A16-0x2A23` | 1 marker bit, DATA low |
| 4 | `0x2A25-0x2A48` | 35 glyph bits |
| 5 | `0x2A4B-0x2A59` | 1 trailing bit, DATA high |
| 6 | `0x2A5B`: `MVI A,0x80 / OUT 0xE2` | deselect, a literal `0x80` |

`0xA00A` is a plain 0-based character counter, reset at `0x1260`/`0x1407` and
incremented once per character by the render loop, so the address on the bus is

```
port B = ~position | 0x80
```

— encoded binary, active low, with bit 7 forced high. Observed for a six-character
message, with the deselect between each:

```
written  : FF 80  FE 80  FD 80  FC 80  FB 80  FA 80
position :  0      1      2      3      4      5
```

`0x80` is `~0x7F`, i.e. address 127, outside the 64 valid positions, so it is a
bus-idle state rather than an address.

Port B is therefore an **encoded** module address spanning 64 positions, not
sixteen dedicated select lines. A full repaint walks all 64 values; undoing the
`CMA` shows a plain incrementing counter over `0x00-0x3F` with the idle code
between each:

```
post-CMA, as written : 80 C2 80 C1 80 C0 80 FF 80 FE 80 FD 80 FC 80 FB ...
pre-CMA, the value   : 7F 3D 7F 3E 7F 3F 7F 00 7F 01 7F 02 7F 03 7F 04 ...
with 0x7F removed    : 3D 3E 3F 00 01 02 03 04 05 06 07 08 09 0A 0B 0C ...
```

Sixty-four addresses matches the 64-byte message buffer exactly — one address per
buffer column — and the conductor budget works out: six or seven address bits plus
clock, data and control is about ten signals, leaving most of the 26 conductors
for supply and for a downstream display.

### The address width is panel-family specific

| Family | Address | Glyph data |
| --- | --- | --- |
| `0x11` (`0x29D7`) | port B, 6 bits of 8, `~pos \| 0x80` | serial, shifted on port C |
| `0x0A` (`0x210D`) | port B bits 3:0, `~pos & 0x0F`, plus a strobe OR-ed in from C | parallel byte on port A (`0x2113`) |

So family `0x0A` is a 16-character parallel panel and family `0x11` a serially fed
one with a 64-position address space. Both derive the position from the same
`0xA00A` counter.

## What the rest of the character bus reaches

The firmware addresses **positions 0 through 63** on one bus, contiguously, one
74-bit frame each. There is no split at character 16 anywhere in the ROM:
`render_pass_paged` walks the message buffer against the shadow in lockstep,
bumping `char_pos` by one per position, and stops on the `0x03` sentinel that
`render_task` writes at `msg_buffer+0x40`. One flat 64-entry walk, one shadow, one
position counter, one port group.

The flip-dot line is 16 characters, so 48 of those addresses reach something else.
What that is has not been traced. Three readings fit what is known:

| Reading | Positions 16-63 are | Would predict |
| --- | --- | --- |
| The LED row sits on the character bus | the LED row | a 64-character message draws a response past position 15 |
| The ribbon reaches the far side's flip-dot row of a double-sided sign | unused | the far side answers 0-15, in parallel with the near side |
| The LED assembly reads the message buffer itself | unanswered | nothing on the ribbon responds past 15 |

On the second: the two faces of a back-to-back sign show the same content, so the
far side would be the same bus continued, answering the same 0-15. That costs no
addresses and involves the firmware not at all, which is why it fits everything
observed and equally why it explains nothing about 16-63.

Against the first, 48 characters is thin for a display meant to carry intermediate
stations and service notices. That objection is softer than it looks, though: this
firmware cannot scroll — its effects are `BEL` attributes, not motion — so whatever
scrolls the station list is upstream or on the LED assembly, and a host can
rewrite the line repeatedly instead. At 9600 baud a 64-character message is
roughly 70 ms, and the differential update charges a frame only for positions that
changed, so 48 characters would be a window rather than a message-length limit.

One test separates all three: instrument the ribbon, send a 64-character message,
and watch which addresses draw a response.

A related detail fits a chain longer than one panel. At `0x1290`, when the position
counter completes a block of 64, the firmware inserts `C-1` extra address slots
before continuing — a gap between blocks. `C` is frozen at 1, so the gap is zero,
but the knob only makes sense for a chain assembled from more than one block.

### One row of 64

The station-wide variant of this sign stacks two to four rows, and the ROM looks at
first as if it supports that: `cmd_vt_position` computes `msg_buffer + 0x40 * row`.
But `check_buffer_bounds` clamps the write pointer at `msg_buffer + 0x40` =
`0xE041`, sets `cursor_row` to 1 and latches error `0x30`, so row 1 is an overflow
rather than a place to put text.

This build is therefore a single row of at most 64 positions, and the row
arithmetic is generic code carried for wider members of the family.

## Display attributes on port C bits 1:0

The attribute is driven out to the panel as two hardware lines, which is why the
shift loop preserves the low bits of port C on every write: they hold state between
glyph frames.

`BEL <param>` stores `param & 0x07` into `0xA014` (`0x10D0`). Whenever that value
changes, `0x09F3` dispatches on the panel family and, for this family, reaches
`0x0A82`:

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

So of the eight display attributes only the low two bits reach this panel: four
states on two lines. Bit 2 does something different, below.

**What those two lines physically do is not known.** Two things are excluded:
they are not brightness, because the dots are mechanical and bistable; and they
are not the fluorescent tube, which is not coupled to the electronics in any way —
it has its own mains feed and ballast, so the firmware cannot switch it.

What remains, in rough order of plausibility: they target the LED matrix rather
than the flip-dots, which is the part of the sign that can be dimmed or blinked
and the part that scrolls; or they are a hardware blink enable on the flip-dot
modules, with the durations at `0xA016`/`0xA018` setting how long the effect lasts
rather than its rate; or they are a per-panel mode select with no visual meaning of
its own. Tracing port C bits 1:0 through the ribbon, or sending `BEL 0` through
`BEL 3` to a powered unit and watching, would settle it.

Other panel families route the same two bits elsewhere, which corroborates that
they are attribute lines rather than anything family-specific:

| Family | Routine | Destination |
| --- | --- | --- |
| 2, 3, `0x0D`, `0x12`-`0x15`, `0x17` | `0x0A82` | port C bits 1:0 |
| `0x19` | `0x0A90` | port B bits 1:0 |
| `0x18` | `0x0A9E` | attribute bit 0 → port C bit 5, bit 1 → port B bit 7 |

### Bit 2 makes the effect permanent

`0x199F`, called after every character, ages the attribute — but only under
receive handler A; handler B jumps straight past it.

* Attribute bit 0 is paired with duration `0xA018`, bit 1 with `0xA016`. Both
  count down on the RST7.5 tick (`0x02EF`), and when one reaches zero the matching
  attribute bit is cleared, ending the effect.
* Bit 2 suppresses that ageing for bit 0: the test is `(attr & 0x05) == 0x01`, so
  with bit 2 set the auto-clear never runs and the effect persists.
* Bit 7 short-circuits the whole thing, turning the attribute off.
* `0xA013` is set whenever `attr & 0x03` is non-zero — "an effect is running" —
  and gates the countdown at `0x02EF`.

That decodes handler A's in-band control characters:

| Character | Sets | Meaning |
| --- | --- | --- |
| `0x7C` `\|` | `attr = (attr & 0x7F) \| 0x05` | effect 0, permanent via bit 2 |
| `0x7D` `}` | `attr \|= 0x02`, plus `0xA016` = 80 or 2000 | effect 1, timed |
| `0x7F` DEL | `attr = (attr & 0xFA) \| 0x80` | effects off |

which is also why `0x7C` sets two bits rather than one.

## What the render path needs before it runs

Five conditions, all of which must hold before a single dot moves:

1. **A panel family.** Port C bits 4:0 pick a branch at `0x055A`; each loads a
   family into `0xA009`, and `0x136A` dispatches on it to a driver. The default
   branch sets `0xA009 = 0`, for which no driver exists, so a unit with
   unrecognised switches drives nothing. Thirteen of the 32 values select a real
   driver.
2. **The RST7.5 tick.** The idle loop at `0x1675` spins while `0xA01B & 1` is set,
   and only the RST7.5 handler clears it at `0x0336`.
3. **Changed content.** `0x136A` is reached by `CNZ`, a conditional call after
   comparing the new byte against the shadow (`0x1447`), so only characters that
   differ are redrawn.
4. **Somewhere for the buffer to live** — the `0xE000` window.
5. **The unit must not be selected.** The repaint task spins at `0x130B` while the
   selected flag is set; see
   [serial-protocol.md](serial-protocol.md#the-repaint-interlock-rs-is-not-optional).

### Two render passes

There are two nearly identical passes over the buffer, and a switch setting chooses
between them:

| Pass | Access | Selected when |
| --- | --- | --- |
| `0x124E` | via `xram_read`/`xram_compare`, paged | `0xA008 = 0` |
| `0x13F7` → `0x1445` | direct `MOV A,M`, unpaged | `0xA008 = 1` |

`0xA008` is loaded at `0x0547` from DIP switch port B bit 0, and the choice only
takes effect under receive handler A — under handler B, `0x1243` diverts to the
paged pass regardless. Both passes walk the buffer and its shadow in step, call the
driver only where the two differ, and terminate on the byte matching `0xA007`
rather than on a count.

### What forces a full repaint

`0x13C6`, called at the top of every pass, clears the 64-byte shadow to zero, so
every character then differs and the next pass repaints everything. It does that
when the panel family is `0x08` — pulsing `0x36` to port C first — or when port C
bit 5 is clear, or when `0xA023` is `0xFF`. It explicitly does not for family
`0x12`, which returns immediately.

`0xA023` is set to `0xFF` at `0x07D4` for families 2, 8, 9, `0x0B`, `0x0C`, `0x0D`
and `0x0E`, and left at zero for every other family including `0x11`. So the
firmware divides its supported displays into two classes: **continuously
refreshed**, where the whole panel is rewritten every pass, and **differentially
updated**, where only changed characters are touched.

A flip-dot panel must be differential, since its dots are bistable and rewriting
unchanged ones is pure coil heating. A continuously refreshed family is what an LED
or lamp matrix needs. Family `0x11` falling in the differential class is therefore
consistent with it being the flip-dot driver, reached independently of the drive
activity that first identified it.

## The message buffer is backed on this unit

The buffer at `0xE001` lives in the window that `0xE7` pages, which is on the
expansion board, and the accessors fall back to accessing `(HL)` directly when
`0xE7` reads `0xFF`. In emulation that fallback leaves the buffer unbacked and no
text renders, which is why `pit100_sim.py` needs `--xram board`.

On the real unit, text sent over the serial link **does** render. So the `0xE000`
window is backed here: either the expansion latch reports present, or `0xE001` is
aliased onto real memory by incomplete address decoding. Which of the two has not
been determined, and needs a probe of `0xE7`.

## An aside: the glyph lookup is a linear search

`0x29DD-0x29F3` walks the font table five bytes at a time comparing the index
against the character code, up to 255 iterations, even though the table is directly
indexable as `0x1C0D + code * 5` ([font.md](font.md)). If the code is not found it
restarts the search for character 0. Functionally irrelevant, but it sets the
per-character cost, which is worth knowing when reasoning about how fast a host can
push updates.

## Validated end to end

[panel_view.py](../tools/panel_view.py) recovers the bit stream from the emulated
port writes and reconstructs the glyphs. Sending `FF`, which fills the buffer with
`0x23`, produces:

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
simultaneously for that to come out. Text renders correctly too:

```sh
PYTHONPATH=tools python3 tools/panel_view.py --text "HOLA MUNDO"
```

```
   #...#  .###.  #....  .###.  #...#  #...#  #...#  ####.  .###.
   #...#  #...#  #....  #...#  ##.##  #...#  #...#  #...#  #...#
   #...#  #...#  #....  #...#  #.#.#  #...#  ##..#  #...#  #...#
   #####  #...#  #....  #####  #.#.#  #...#  #.#.#  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #..##  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #...#  #...#  #...#
   #...#  .###.  #####  #...#  #...#  .###.  #...#  ####.  .###.
```

The space between the words produces no frame, because the position already held a
space and the update is differential. A space is an ordinary glyph — `font_5x7`
entry `0x20` is five zero bytes — and writing one over a non-blank character does
emit a frame and does clear those dots.

## Safety note

Flip-dot coils are designed for short pulses. Leaving one energised continuously
will overheat and can destroy it. Any experiment that drives the panel directly,
bypassing this firmware, must guarantee a bounded pulse width in hardware or in a
watchdog, not merely in software that could hang.

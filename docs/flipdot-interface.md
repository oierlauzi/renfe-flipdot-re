# Flip-dot drive interface

**Status: barely started.** This document is about how the firmware physically
flips dots — the inner loop between the frame buffer and the coils.

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Signal chain

```
81C55-class device @ I/O 0xE0        ULN2003A x3 (U18a, U18b, U19)
  port A (0xE1) ─┐                   7 Darlington sinks each
  port B (0xE2) ─┼─ daughter 2 ──▶   = 21 sinks, with     ──▶ ribbon ──▶ panel
  port C (0xE3) ─┘                   free-wheel diodes                  coils
                                                                    (16 x 5 x 7)
```

The 81C55's three ports give 8 + 8 + 6 = **22 output bits**, against **21 sinks**.
That near-match is unlikely to be coincidence: it suggests the ports drive the
Darlington inputs almost one-for-one, with a single bit left over for a strobe or
an enable.

**Three** ULN2003A packages — U18a, U18b and U19 — give **21 Darlington sinks**.

Two facts about that part constrain the topology more than the count does:

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

## What the firmware does

Initialisation (`FUN_ram_03F2`, see [io-map.md](io-map.md#0xe0-0xef--display-interface-l)):

* Timer preload `0x4600` → mode `01` (continuous square wave), count `0x600` =
  1536. A second, much slower timebase than the UART one — plausibly the
  dot-pulse or refresh clock. **[L]**
* Command `0xCF` → port A output, port B output, port C in ALT4, timer running.
* Ports A and B are pulsed `0x80` then `0x00` with three `NOP`s in between — a
  deliberate short pulse, i.e. a **reset/home pulse to the panel**. **[L]**

Runtime write counts: `0xE1` 33, `0xE2` 39, `0xE3` 39. The hot path is at
`0x210D`:

```
0x210D  PUSH DE / PUSH HL
0x210F  CALL 0x1B69        ; banked read of (HL) - see docs/io-map.md
0x2112  CMA                ; complement
0x2113  OUT 0xE1           ; port A  <- inverted value
0x2115  MVI C,0x10         ; 16 ...
0x2117  LDA 0xA021
0x211A  CPI 0x01
0x211C  JNZ 0x2121
0x211F  MVI C,0x20         ; ... or 32
0x2121  LDA 0xA00A
0x2124  CMA
0x2125  ANI 0x0F           ; low nibble only
0x2127  OUT 0xE2           ; port B  <- inverted low nibble
```

Two readings worth testing:

* `CMA` before every `OUT` says the drive is **active low** — expected with
  Darlington *sinks*. **[L]**
* `C = 0x10` or `0x20` (16 or 32) selected by `0xA021` looks like a
  **character count**: 16 for this panel, 32 if a second board is present.
  That would make `0xA021` the "panel width" configuration. **[L]**
* `0xE2` receiving only a 4-bit value suggests port B carries a **column or
  character address**, not pixel data — 4 bits addresses 16 characters exactly. **[L]**

## Power-on behaviour: the panel is zeroed **[C] observed**

**Observation (owner, 2026-08):** on power-up a brief buzzing sound *sweeps
across the display*. No visual change on this unit, because its dots were already
all showing black.

That single observation settles several things:

* **The rest state is black**, and power-on drives every dot to it.
* **The sweep is sequential**, not simultaneous — the sound moves across the
  panel. Coils are energised in groups small enough to be heard travelling, which
  is what the current budget of two ULN2003A packages would force anyway.
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

## The panel is a serial shift register **[C] from the ROM**

This overturns the earlier working assumption of a parallel row/column matrix, and
it dissolves the "21 lines cannot address 560 dots" problem: the panel is fed
**serially**.

| Port | Bit | Function |
| --- | --- | --- |
| `0xE3` (port C) | 4 (`0x10`) | **shift clock** — data is clocked on the rising edge |
| `0xE3` (port C) | 5 (`0x20`) | **serial data**, **active low**: low = dot ON |
| `0xE3` (port C) | 1:0 | preserved across every write; function unknown **[?]** |
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

Point 3 also bears on the power-on sweep: a purely differential update would
*not* sweep, yet the panel audibly does. Something must force a full repaint at
startup, which the shadow buffer alone does not explain.

## Assumptions this rests on, and how to falsify each

Everything above is derived from the ROM plus emulation. These are the inputs that
were **chosen rather than measured**, in rough order of how much would need
revisiting if one is wrong.

| # | Assumption | Why it was chosen | How to check on the hardware | If wrong |
| --- | --- | --- | --- | --- |
| A1 | Panel type is `0x11`, from port C bits 4:0 = `0x03` | It produced the most drive activity of the 13 candidates | Read port C bits 4:0 from the switches and hardwired links, then look up the branch at `0x055A` | The frame layout may differ per driver. The *method* here applies to whichever driver is real; re-run `panel_view.py --type N` |
| A2 | RST7.5 fires periodically | The idle loop demands it; the rate used (every 2000 instructions) is arbitrary | Scope 8085 pin 7 for a periodic edge and measure its rate | Only the repaint *rate* changes, not the frame format |
| A3 | Port C bit 4 is the clock and bit 5 the data | The code toggles bit 4 twice per bit while bit 5 carries the varying value | Scope both against the ribbon | Would invert the whole decode; the `FF` validation makes this very unlikely |
| A4 | Data is active low (low = dot on) | `CMA` precedes every DATA write, and `FF` then renders as a solid block | Send `FF` on the real panel and see whether it goes all-on or all-off | Every rendered glyph inverts |
| A5 | The frame is 37 + 1 + 35 + 1 bits | Read directly from the loop counters `0x25` and `0x05`/`0x07` | Count clock edges per character on a logic analyser | The 37 leading bits are the least certain part; they may address a longer register chain |
| A6 | An expansion board is present (`0xE7` != `0xFF`) for some captures | Without it the buffer at `0xE000` is unbacked | The unit has no LED matrix, so `0xE7` should read `0xFF` | The buffer question in [io-map.md](io-map.md) needs settling first |

### The one thing that did not work

**Message text does not reach the rendered glyphs in emulation.** Sending
`STX AB ETX` paints the *fill* character rather than `A` and `B`, so the link
between the message buffer and the render source is still missing a step. The
frame mechanism above is unaffected — it was validated with `FF`, which drives the
same path — but do not trust `panel_view.py --text` to show real text yet.

Most likely related to assumption A6 and the unresolved `0xE000` window.

## A newly attributed switch bit

**Port B bit 0 selects the fill/substitute character.** At `0x0822` and `0x1351`
the firmware reads port B, tests bit 0, and when set stores `0x23` into `0xA041`.
`0x23` is the solid-block glyph, so **bit 0 set makes the pad character a solid
block instead of a space**. With it clear, the fill activity disappears entirely
from the capture. That closes one of the two remaining unattributed port B bits —
see [dip-switches.md](dip-switches.md). **[L]**

## Open questions

- [ ] Count the ribbon conductors and match them to the port bits. Now much more
      informative: with serial data on two lines, most of the 21 sinks must be
      doing something else — buffering the register chain, or per-character
      selects from port B.
- [ ] **Why does message text not reach the rendered glyphs?** The fill character
      is painted instead. Probably tied to the `0xE000` window.
- [ ] Determine the pulse width used to flip a dot, and whether the firmware
      relies on the 81C55 timer or on software delay loops. **Timing the
      power-on sweep gives this almost for free** — see above.
- [ ] Does daughter 2 contain a hardware sequencer that performs the power-on
      sweep autonomously? The CPU issues only a short pulse at `0x03FF`, yet the
      panel scans audibly.
- [ ] Does it repaint the whole panel or only changed dots? (Matters for how
      fast an external controller can push updates, and for coil heating.)
- [ ] Where is the frame buffer in the `0xA000` RAM, and in what layout?

## Safety note

Flip-dot coils are designed for **short pulses**. Leaving one energised
continuously will overheat and can destroy it. Any experiment that drives the
panel directly — bypassing this firmware — must guarantee a bounded pulse width
in hardware or in a watchdog, not merely in software that could hang.

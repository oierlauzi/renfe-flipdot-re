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

## Open questions

- [ ] Count the ribbon conductors and match them to the port bits.
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

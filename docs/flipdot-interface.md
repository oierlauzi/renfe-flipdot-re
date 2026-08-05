# Flip-dot drive interface

**Status: barely started.** This document is about how the firmware physically
flips dots — the inner loop between the frame buffer and the coils.

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Signal chain

```
81C55-class device @ I/O 0xE0        ULN2003A x2 (U18)
  port A (0xE1) ─┐                   7 Darlington
  port B (0xE2) ─┼─ daughter 2 ──▶  sinks each   ──▶ ribbon ──▶ panel coils
  port C (0xE3) ─┘                                              (16 x 5 x 7)
```

Two ULN2003A packages give 14 Darlington sinks. Seven of those plausibly drive
the seven dot rows; the remainder, plus the third port, would drive the column
select and the flip polarity. **[?]**

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

## Open questions

- [ ] Count the ribbon conductors and match them to the port bits.
- [ ] Determine the pulse width used to flip a dot, and whether the firmware
      relies on the 81C55 timer or on software delay loops.
- [ ] Does it repaint the whole panel or only changed dots? (Matters for how
      fast an external controller can push updates, and for coil heating.)
- [ ] Where is the frame buffer in the `0xA000` RAM, and in what layout?

## Safety note

Flip-dot coils are designed for **short pulses**. Leaving one energised
continuously will overheat and can destroy it. Any experiment that drives the
panel directly — bypassing this firmware — must guarantee a bounded pulse width
in hardware or in a watchdog, not merely in software that could hang.

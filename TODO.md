# TODO

Two lists. **Hardware** tasks require physical access to the unit and bench
instruments. **Firmware** tasks require only the ROM image in this repository.

Results go straight into the `Result:` slots below. A filled-in slot is worth
more than a tidy empty one; record negative results too.

Deeper technical detail for each item lives in
[docs/open-questions.md](docs/open-questions.md).

---

# Hardware

## P0 — Before anything else

The first two items are destroyed or made harder by any prior poking around; the
third is a safety precondition.

- [ ] **Identify both AC feeds and confirm which one powers the PSU.** The
      fluorescent tube has its own mains input and its ballast is inside the
      enclosure, so mains is live in more than one place. Prefer all measurements
      with the tube feed disconnected — it removes both a hazard and a large
      inductive noise source. → [hardware/psu.md](hardware/psu.md)
      - Result:

- [ ] **Record the DIP switch positions as found.** All three banks,
      photographed close enough to read *and* written down. This is the only
      state in the machine that cannot be recovered from the ROM.
      - Result — SW1 (port A):
      - Result — SW2 (port B):
      - Result — SW3 (port C):
- [ ] **Determine the switch sense**: does a *closed* switch read 0 or 1 at the
      81C55? Pull-ups usually mean closed = 0, which **inverts every table** in
      [docs/dip-switches.md](docs/dip-switches.md). An ohmmeter from a switch pin
      to ground/Vcc with the unit powered off is enough.
      - Result:

## P1 — One measurement, six answers

- [x] **8085 clock frequency — done.** A **6.144 MHz** crystal sits beside the
      CPU. The 8085 halves it, so the machine runs at 3.072 MHz and the whole baud
      ladder comes out exact: 9600 / 4800 / 2400 / 1200 / 600 / 300, zero error on
      all six. → [docs/dip-switches.md](docs/dip-switches.md#confirmed-by-the-crystal-c)
- [ ] **Confirm the last two links of the clock chain.** Continuity from **8085
      pin 37 (CLK OUT)** to **81C55 pin 3 (TIMER IN)**, and from **81C55 pin 6
      (TIMER OUT)** to the **8251A TxC and RxC (pins 9 and 25)**. If TIMER IN were
      fed from the raw crystal instead, every rate would double.
      - Result — CLK OUT → TIMER IN:
      - Result — TIMER OUT → TxC/RxC:

## P2 — Critical path to driving the display

- [ ] **DB37 pinout.** Number the pins from the connector shell, photograph the
      numbering next to the wires, then buzz each of the four conductors through
      to the daughter-1 terminal block.
      - Result — orange → pin / terminal:
      - Result — blue → pin / terminal:
      - Result — green → pin / terminal:
      - Result — yellow → pin / terminal:
- [ ] **Locate the receive pair.** Trace from the terminal block to U7 (6N139).
      Whatever reaches its pins 2 and 3 is the **RX** pair.
      - Result:
- [ ] **Read the series resistors feeding the 6N139 input.** Together with the
      supply voltage these give the expected drive current directly.
      - Result:
- [x] ~~Scope the four pins.~~ Attempted 2026-08: **transitions visible**, signal
      unclear with an AC component. The transitions are the unit's own status
      frame, so the pins carrying them are the **transmit** side, and the
      transmitter is not a bare passive switch.
      → [hardware/connectors.md](hardware/connectors.md#attempt-1-oscilloscope-2026-08--transitions-present-signal-unclear)
- [ ] **Which pins showed the transitions?** That identifies the TX pair outright.
      - Result:
- [ ] **Capture one frame cleanly.** Tube feed disconnected; probe ground clipped
      to one conductor of the pair, probe on the other; DC coupled; edge trigger,
      single shot; sweep the timebase from 200 µs/div to 20 ms/div.
      - Result — peak-to-peak amplitude (gives the standard):
      - Result — narrowest pulse width (gives the baud rate):
      - Result — idle level (gives the polarity):
- [ ] **Decode the captured frame against the known answer** `18 18 1E 15 <addr>`
      where `<addr>` = `0x60 | (port A & 0x1F)`. A match confirms baud, polarity
      and framing simultaneously.
      - Result:
- [ ] **Identify the electrical standard — diode test, power OFF, no scope
      needed.** Multimeter in diode mode across each pair, both polarities:
      - ~1.1-1.6 V one way and open the other → **20 mA current loop** (the
        6N139's LED)
      - no diode behaviour, ~120 Ω or ~12 kΩ → **RS-422/485**
      - Result:
- [x] ~~120 Ω termination?~~ Daughter 1's only resistor values are **560 Ω** and
      **~1 kΩ**. Nothing near 120 Ω, and **no differential transceiver IC at all** —
      the parts are a DC-DC converter, a hex Schmitt trigger and six probable
      optocouplers. That is a discrete opto-isolated interface, not RS-422.
      → [hardware/connectors.md](hardware/connectors.md#what-daughter-1s-parts-imply-l)
- [ ] **Measure the DC-DC converter's output voltage.** This is now the highest-
      value single measurement on the board. ~24 V makes the ~1 kΩ resistors
      loop-current setting (~20 mA) and nearly closes the case for a current loop;
      5 V means they are ordinary opto drive and prove nothing.
      - Result:
- [ ] **Measure the three ~1 kΩ resistors** with a meter, to remove the colour-band
      ambiguity (1.0 / 1.02 / 1.2 kΩ are all plausible readings).
      - Result:
- [ ] **Trace one ~1 kΩ resistor**: does it sit between a terminal-block screw and
      an optocoupler pin?
      - Result:
- [ ] **How are the 4 diodes arranged?** A full bridge across an input pair means
      a polarity-insensitive current-loop input — a strong signature. Individual
      diodes across opto LEDs are just reverse protection.
      - Result:
- [ ] **Three of each resistor value suggests three signals per direction**, not
      one. Do not assume only TX and RX exist when tracing — look for a third
      (RTS, or a second channel for the LED matrix).
- [ ] **Is the transmit side passive or active?** Does anything on daughter 1
      supply current to the TX pair, or are U14-U17 bare switches expecting an
      external supply?
      - Result:
- [ ] **Locate the transmit driver.** Part numbers on U14-U17 (the yellow DIP8s
      on daughter 1). If they are optocouplers, whichever pair they drive is TX.
      - Result:

## P3 — Settles specific open questions

- [ ] **U20 top marking** (DIP40, daughter 2). The whole flip-dot interpretation
      rests on this being an 81C55 — see
      [docs/io-map.md](docs/io-map.md#0xe0-0xef--display-interface-l).
      - Result:
- [ ] **U8, U9 (DIP16), U10 (DIP20), U19 (DIP16, ST)** markings.
      - Result:
- [ ] **Does RST7.5 fire?** Scope on 8085 pin 7. Settles whether the `0xC0`
      device is present or absent.
      - Result:
- [ ] **Panel ribbon conductor count.** Discriminates the row/column drive
      arrangement immediately.
      - Result:
- [ ] **Time the power-on buzz** with a stopwatch or a phone recording. Divided
      by 560 dots it gives the per-dot drive time directly, which is otherwise an
      open question. → [docs/flipdot-interface.md](docs/flipdot-interface.md)
      - Result — sweep duration:
- [ ] **Solder-side photos** of the motherboard and both daughter boards, for
      tracing the chip selects.

## P4 — Shopping, once the standard is known

See [docs/controller.md](docs/controller.md) for the full comparison. The hedge
that covers every outcome: a USB-to-TTL serial adapter, a MAX490 and a MAX485
breakout, a few 6N137/6N139 optocouplers, and a current-limited bench supply.

- [ ] Decide the interface hardware once the diode test or a clean capture
      settles the electrical standard.

## P4b — Long shots worth an hour

- [ ] Any original controller, or any recording of the line, would nail the
      protocol outright.
- [ ] PAGE Ibérica still operates in Tres Cantos. A polite email asking for
      PI-T100 documentation costs nothing —
      see [docs/references.md](docs/references.md).
- [ ] A second read of the EPROM, to confirm the dump's SHA-256.
      → [original/README.md](original/README.md)

---

# Firmware

Detail in [docs/open-questions.md](docs/open-questions.md).

- [x] **8085 emulator harness** — `tools/emu8085.py` + `tools/pit100_sim.py`.
      Reproduces the documented 8251A init from a cold boot.
- [x] **Receive handler A (`0x08B7`) disassembled.** Framing, per-character
      filtering, error handling and the `0x4A` completion token are documented in
      [docs/serial-protocol.md](docs/serial-protocol.md).
- [x] **Receive handler B (`0x0ACB`) command table extracted** — 28 ASCII
      control codes, `STX`/`ETX` framing, `DLE` = reset, and **the unit replies
      ACK/NAK on ETX**. → [docs/serial-protocol.md](docs/serial-protocol.md)
- [x] **DIP switch port A attributed**: bits 4:0 = display address (confirmed by
      sweeping it in the simulator), bit 6 = receive protocol select.
- [x] **STX, ETX, CAN, RS, DLE, ACK, BEL, ENQ and GS decoded**, along with the
      64-byte buffer, the full reply frame and the ROM configuration block.
      → [docs/serial-protocol.md](docs/serial-protocol.md)
- [x] **All handler-B commands decoded.** The buffer is a cursor-addressable
      2-D grid of 64-column rows; VT/CR/LF/DC1-DC4/ETB/EM/CAN/FF are terminal
      editing commands, and TAB/SO/SI/SUB/BS are queries that reply with real
      data. → [docs/serial-protocol.md](docs/serial-protocol.md)
- [x] **Addressing understood and validated**: `GS <addr>` selects a unit,
      `0x7F` broadcasts, `RS` deselects, and until selected only EOT/VT/GS are
      accepted. A wrong address leaves the unit silent.
- [x] **Port C bit 5 is the serial enable**, not merely an init-path selector.
      Clear = standalone with hardwired 9600 8E1 and reception disabled.
- [ ] **`0x04` EOT sub-modes** — the parameter is an ASCII digit; `'3'` diverts
      to `0x121A`. What are the others?
- [ ] **`0x08` BS status report** — what does it actually return?
- [ ] **What do the 8 display attribute values mean?** BEL's parameter selects
      one of 8 via `0xA014` bits 2:0. Cross-reference with handler A's
      `0x7C`/`0x7D`/`0x7F` and the durations at `0xA016`/`0xA018`.
- [x] **Port A fully attributed**: bits 4:0 address, bit 5 blank-substitute
      character, bit 6 protocol select, bit 7 a render option. The address also
      forms handler A's start-of-frame byte as `0x60 | addr`, which is the same
      value the status frame carries.
- [ ] **Port A bit 7's exact render effect** (`0x0753`, `0x0831`). Sweeping it
      in the simulator changes nothing observable, so its effect is downstream of
      hardware the model stubs.
- [ ] **Which port C bits 4:0 value matches this panel?** They select the display
      type via a 32-way dispatch at `0x055A`; seven values have distinct
      handlers. → [docs/dip-switches.md](docs/dip-switches.md)
- [ ] **Why does the panel sweep at power-on but the CPU issue no per-dot
      writes?** Suspect a hardware sequencer on daughter 2 strobed by the pulse
      at `0x03FF`. → [docs/flipdot-interface.md](docs/flipdot-interface.md)

- [x] **Status frame explained**: `CAN CAN RS NAK <0x60|addr>`, with the
      address coming from port A. Two variants differing in bits 7:6 of the last
      byte — those flag bits are still unidentified.
- [ ] The stack reaches `0x9FFF` once per ~600k steps, entering at `0x2C80` —
      a real deep-nesting path in the firmware, not runaway recursion in the
      model. Three bytes past the RAM floor: sloppiness, or RAM extends below
      `0xA000`. Needs the hardware to settle.
- [ ] Trace the render path: frame buffer layout, the 97-iteration loop at
      `0x03CE`, and `0xA021` (the 16-vs-32 character count).
- [x] **Code page identified: IBM CP437.** Every region matches — control
      pictograms, accented Latin, box drawing, Greek and maths. `Ñ` is `0xA5`,
      and `text.encode("cp437")` produces correct bytes.
      → [docs/font.md](docs/font.md)
- [ ] **What is `0x24`?** `0x24` and `0x25` are accented capitals, not `$`/`%`.
      `0x25` reads as a second `Ñ`; `0x24` is unidentified.
- [ ] A friendlier glyph tool: generating and editing 5-byte glyphs by hand.

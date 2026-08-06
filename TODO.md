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

Both items are destroyed or made harder by any prior poking around.

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

- [ ] **8085 clock frequency.** Read the crystal marking on X1/X2 (pins 1-2), or
      scope CLK OUT (pin 37), then confirm what feeds the 81C55 TIMER IN.
      This fixes the entire baud ladder at once — see
      [docs/dip-switches.md](docs/dip-switches.md#port-b-bits-75--line-rate-c).
      Expected: 6.144 MHz crystal → 3.072 MHz CLK OUT → 9600 baud at divider 20.
      - Result — crystal marking:
      - Result — CLK OUT:
      - Result — 81C55 TIMER IN:

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
- [ ] **Identify the electrical standard.** Powered, nothing connected, measured
      across each pair:
      - near-0 V differential with a few volts common mode → **RS-422/485**
      - constant ~20 mA flowing, or a supply feeding a series resistor into the
        loop → **20 mA current loop**
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
- [ ] **Solder-side photos** of the motherboard and both daughter boards, for
      tracing the chip selects.

## P4 — Long shots worth an hour

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
- [ ] **Decode the individual handler-B commands.** The dispatch targets are
      known; what each one *does* is not. Highest value: `0x0F17` STX,
      `0x0F29` ETX, `0x101E` CAN, `0x10A3` RS.
- [ ] **What are port A bits 7 and 5?**
- [ ] **Decode the `0xA014` attribute bits.** The in-band control characters
      `0x7C`, `0x7D`, `0x7F`, `0x64-0x66` set display effects and durations;
      which bit means scroll, blink or static is still open.
- [x] **Status frame explained**: `CAN CAN RS NAK <0x60|addr>`, with the
      address coming from port A. Two variants differing in bits 7:6 of the last
      byte — those flag bits are still unidentified.
- [ ] Find why the emulated stack dips below `0xA000` (~120 writes to
      `0x9FFD-0x9FFF`). Either a model artefact or a real clue about RAM size.
- [ ] Attribute DIP switch **port A** — seven unknown bits, likeliest home of a
      display address.
- [ ] Trace the render path: frame buffer layout, the 97-iteration loop at
      `0x03CE`, and `0xA021` (the 16-vs-32 character count).
- [ ] Identify the upper-half font code points (`0x80-0xFF`) and produce a
      candidate code page. → [docs/font.md](docs/font.md)
- [ ] A friendlier glyph tool: generating and editing 5-byte glyphs by hand.

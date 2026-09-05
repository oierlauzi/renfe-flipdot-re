# Open questions

The working queue. Move items into the relevant document once answered, and
delete them from here — this file should shrink over time.

## Blocking the goal (external control without hardware modification)

- [ ] **DB37 pinout.** Which 4 of the 37 pins are populated, and which is TX+,
      TX-, RX+, RX-? Continuity-trace from the connector to the daughter 1
      terminal block and on to U7 (6N139). → [hardware/connectors.md](../hardware/connectors.md)
- [x] **Electrical standard — answered: opto-isolated RS-485 / RS-422.** Two
      SN75176BP transceivers and three 6N139 optocouplers on daughter 1.
      → [hardware/connectors.md](../hardware/connectors.md#the-physical-layer-opto-isolated-rs-422rs-485-c)
- [ ] **4-wire or 2-wire?** Two transceivers and four conductors say 4-wire full
      duplex, but the pins have not been traced. Decides which adapter to buy.
- [ ] **Frame format.** Start/end delimiters, addressing, checksum, text
      payload encoding. → [docs/serial-protocol.md](serial-protocol.md)
- [ ] **Does the unit reply?** Only two `OUT 0x80` sites exist. If one is an
      ACK/NAK, protocol discovery by experiment becomes far cheaper.
- [ ] **Upper-half character encoding** (`0x80-0xFF` glyphs). What byte
      produces `Ñ`? → [docs/font.md](font.md)

## Cheap wins available right now (no hardware needed)

- [ ] **Disassemble the two receive handlers `0x08B7` and `0x0ACB` in full.**
      The framing is inside them. Highest-value work left, no hardware needed.
      → [docs/serial-protocol.md](serial-protocol.md)
- [ ] What does `0xA00C` select between, and which handler runs on a unit with
      no expansion board and default switches?
- [ ] Attribute DIP switch **port A**. Bit 6 feeds `0xA00C` at `0x0689`; the
      other seven bits are unknown and are the likeliest home of a display
      address. → [docs/dip-switches.md](dip-switches.md)
- [ ] Which bits of the `0xE7` capability register mean what? `0x112E` tests
      bit 4. Only reachable with an expansion board present.
- [ ] Map the rest of the `0xA000` RAM variables.
- [ ] What drives the 8085 SOD pin's consumer? SOD is set in RST7.5 and cleared
      at `0x0344`, so something external watches it.

## Hardware identification

- [ ] U8, U9 (DIP16), U10 (DIP20) on the motherboard.
- [ ] The five unlabelled DIP8s on daughter 1, three of them yellow and likely
      optocouplers. They carry no silkscreen designators.
- [ ] U19 (DIP16, ST) and U20 (DIP40) on daughter 2. Is U20 a third 81C55?
      That would confirm the `0xE0`/`0xE8` interpretation.
- [ ] Which address lines generate the `0x80` / `0xA0` / `0xC0` / `0xE0` chip
      selects.
- [ ] Measure the 81C55 TIMER IN frequency → pins the baud rate exactly.
- [ ] What does `IN 0xDE` read?

## Flip-dot drive

- [ ] Column/row scan order and the pulse width used to flip a dot.
- [ ] Does the firmware only flip *changed* dots, or repaint the whole panel?
- [ ] Ribbon cable pinout to the panel.
- [ ] What the `0xE8` group would drive if the second board were present.

## Nice to have

- [ ] Identify the 5x7 font's provenance — is it a stock PAGE Ibérica font or
      a standard one?
- [ ] Find other PAGE Ibérica PI-T100 documentation, service manuals, or
      RENFE Cercanías installation notes.
      → [docs/references.md](references.md)

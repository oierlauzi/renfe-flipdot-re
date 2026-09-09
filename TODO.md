# What is left

Everything already established lives in [docs/](docs/) and
[hardware/](hardware/); [notes.md](notes.md) is the chronological record of how
it got there. This file is only the open list.

**Hardware** items need physical access and bench instruments. **Firmware** items
need only the ROM image in this repository. Record results in the `Result:`
slots, negative ones included.

---

# Hardware

## The link

- [ ] **DB37 pin numbers** for the four conductors. Number the pins from the
      connector shell, photograph the numbering next to the wires, and buzz each
      through to the daughter 1 terminal block. This is the last gap in an
      otherwise complete wiring diagram.
      - Result — green (RX `A`) → pin:
      - Result — yellow (RX `B`) → pin:
      - Result — blue (TX `A`) → pin:
      - Result — red (TX `B`) → pin:
- [ ] **Is `DE` on the blue-red transceiver driven from RTS, or tied high?**
      Follow pin 3 back. Driven means a multidrop bus where several signs share
      the return pair; tied high means point-to-point. It changes whether an
      undriven pair between replies is normal or a fault.
      - Result:
- [ ] **Confirm the blue-red package's `DI` is driven** rather than strapped.
      - Result:
- [ ] **Trace the motherboard's `RP RN TP TN` terminal block**, unconnected in
      this unit. Does it reach daughter 1's SN75176BP `A`/`B` pins, or `U7` and
      `U8`, or the 8251A's `TxD` (pin 19) and `RxD` (pin 3) directly?
      → [hardware/connectors.md](hardware/connectors.md#the-motherboard-terminal-block)
      - Result:
- [ ] **U8 marking**, the last unidentified part on the motherboard. With U9 and
      U10 identified as glue logic, U8 decides whether the terminal block above is
      a second line interface or just daughter 1's pairs brought out.
      - Result:

## The switches

The measured configuration is 1200 baud, 7E1, address 1, which fixes the port
values. How the recorded switch positions map onto them does not follow from
either polarity, so the mapping is unresolved.
→ [docs/dip-switches.md](docs/dip-switches.md#the-physical-switches)

- [ ] **Switch sense**: does a closed switch read 0 or 1 at the 81C55? An
      ohmmeter from one side of a switch to ground and to Vcc, powered off.
      - Result:
- [ ] **Switch-to-port wiring**: continuity from the `S1` end of bank `B` to
      81C55 PB0 and from the `S9` end of bank `A` to PA0. 81C55 pins 21-28 are
      PB0-PB7, pins 29-36 PA0-PA7. This checks bit order and bank lettering at
      once.
      - Result:
- [ ] **Is `S17` port C bit 5?** It selects the protocol family, so it is worth
      knowing on its own.
      - Result:
- [ ] **What do the two daughter-2 switches feed?** Not U20's port C, which is
      configured entirely as outputs. Their designators have not been transcribed
      either.
      - Result:

## The panel

- [ ] **Instrument the ribbon and send a 64-character message.** Watch which
      addresses draw a response. Nothing past position 15 would rule out the LED
      row sitting on the character bus.
      → [docs/flipdot-interface.md](docs/flipdot-interface.md#what-the-rest-of-the-character-bus-reaches)
      - Result:
- [ ] **Trace the 26 ribbon conductors.** Look for the group carrying a binary
      count, the two that toggle per bit, and the supply.
      - Result — address group:
      - Result — clock and data:
      - Result — supply:
- [ ] **How do the two modules on one carrier get distinct addresses** from a
      single 6-switch setting? Probably a base value plus a fixed per-socket
      offset, but nothing has been traced.
      - Result:
- [ ] **Where does the flip-dot high side come from?** ULN2003As can only sink,
      but a coil needs current both ways. Part numbers on the 9 transistors near
      the ribbon, and which side of the load they sit on, would settle it.
      - Result:
- [ ] **What do the two attribute lines do?** Not brightness, and not the
      fluorescent tube. Send `BEL 0` through `BEL 3` to a powered unit and watch,
      or trace port C bits 1:0 through the ribbon.
      - Result:
- [ ] **Time the power-on buzz.** Duration divided by 560 dots gives the per-dot
      drive time directly.
      - Result:
- [ ] **Ribbon pitch and header type**, 26-way IDC expected.
      - Result:

## The rest of the electronics

- [ ] **Probe `0xE7`.** Text renders on this unit, so the `0xE000` window is
      backed — but whether the expansion latch reports present or `0xE001` is
      aliased onto real memory by incomplete decoding is unknown.
      - Result:
- [ ] **Map `IN 0xDE` bit by bit.** It is the unit's hardware status input,
      reported by `BS`. Grounding whatever drives it one line at a time while
      watching `BS` maps the bits directly.
      - Result:
- [ ] **Is the clock board fitted?** `clock_service` returns immediately when
      `IN 0xCD` reads `0xFF`, so an open bus disables the feature silently. Look
      for a keypad and four more digits on the ribbon.
      - Result:
- [ ] **Does RST7.5 fire, and at what rate?** Scope 8085 pin 7. It drives the
      watchdog, the attribute ageing, every `wait_ticks` and the keypad debounce.
      - Result:
- [ ] **Confirm the last two links of the clock chain.** Continuity from 8085
      pin 37 (CLK OUT) to 81C55 pin 3 (TIMER IN), and from 81C55 pin 6 (TIMER
      OUT) to the 8251A's TxC and RxC, pins 9 and 25. If TIMER IN came from the
      raw crystal instead, every rate would double.
      - Result:
- [ ] **Confirm which board `U7` is on.** Daughter 1 already has a complete
      isolation barrier, so a fourth 6N139 on the motherboard is anomalous.
      - Result:

## Power and photographs

- [ ] **Identify both AC feeds and confirm which one powers the PSU.** The
      fluorescent tube has its own mains input and its ballast is inside the
      enclosure, so mains is live in more than one place. Prefer all measurements
      with the tube feed disconnected.
      - Result:
- [ ] **Measure idle and peak current**, and record the DC connector type and
      polarity.
      - Result:
- [ ] **Photograph the converter's QA sticker legibly, then remove it.** It is
      the only marking on that board and is destroyed by peeling.
      - Result:
- [ ] **Solder-side photographs** of the motherboard and both daughter boards,
      for tracing the chip selects and the DB37 wiring.

---

# Firmware

- [ ] **Decode handler C's frame layout.** 930 bytes at `0x2C80`, the largest
      undecoded region of the ROM, and it needs no hardware. The command bytes,
      the reply frames and the length-prefixed message staging are known; the
      frame layout, any checksum, and which command opens a transaction are not.
      Single bytes fed to it in the simulator produce silence, so it wants a
      sequence. → [docs/serial-protocol.md](docs/serial-protocol.md#handler-c)
- [ ] **Give the remaining structural labels real names.** 379 of the 553 labels
      in `src/firmware.asm` are `<routine>_<role><n>`, which is navigation rather
      than meaning. `rx_handler_c` (31 labels) and `init_uart_from_switches` (19)
      are the two worth reading next.
      → [src/README.md](src/README.md)
- [ ] **The 3x5 font's spare bits.** The low 5 bits of each 10-bit column field
      hold something close to a row-shifted copy of the glyph, and group B differs
      from group A for 36 of the 42 characters. Neither is explained, nor is why
      80 clocked bits are spent placing a 3x5 glyph.
      → [docs/font.md](docs/font.md#what-is-still-open)
- [ ] **Why `0x06EC` configures U20's port B as an input.** Nothing in the
      flip-dot interpretation accounts for it.

---

# Documentation and provenance

- [ ] **PAGE Ibérica still operates in Tres Cantos.** A polite email asking for
      PI-T100 documentation, a service manual or a protocol specification costs
      nothing.
- [ ] **Search under Telenorma and Bosch Telecom**, not only PAGE Ibérica. The
      motherboard silkscreen numbers are the most specific identifiers the unit
      carries: `300-48.0216.5586`, `324-40.0219.9078`, `11.0238.4030`, `37623.6`,
      and the carrier board's `37624.0 DV2 A.1`. Verify the digits against the
      boards before quoting them.
      → [docs/references.md](docs/references.md)
- [ ] **Verify the Telenorma corporate history properly** — Telefonbau und
      Normalzeit, and the route into Bosch Telecom. Currently recalled rather than
      sourced.
- [ ] **Any original controller, or any recording of the line**, would settle
      handler C and the upstream protocol at a stroke.

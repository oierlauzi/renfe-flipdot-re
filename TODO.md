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

- [x] **DIP switch positions recorded as found** — the one piece of state that
      could not have been recovered later.
      - Bank A' (`A`, `S9-16`, beside the 81C55): `1 0 0 0 0 1 0 1`  (1 = closed,
        `9` end at the right)
      - Bank B' (`B`, `S1-8`, beside the 81C55): `0 1 1 1 1 1 0 1`  (`1` end at
        the right)
      - `S17`, elsewhere on the motherboard: **open**
      - Two switches on daughter 2: **both open**
      → analysis in [docs/dip-switches.md](docs/dip-switches.md#as-found-c-for-the-readings-l-for-the-mapping)
- [x] **Which bank is port A and which is port B**, and which end of each printed
      bank is bit 7? **Answered by the silkscreen [L]**: the banks are lettered
      `A` and `B` beside a chip whose ports are A/B/C, and the `S1`/`S9` end
      markers make the right-hand digit above bit 0 on both banks. Each rests on
      one convention, so it is [L] rather than [C].
- [ ] **Determine the switch sense**: does a *closed* switch read 0 or 1 at the
      81C55? **This is now the only unknown left** between the recorded positions
      and the unit's actual configuration — closed = 0 gives address 26 at 4800
      8N1, closed = 1 gives address 5 at 2400 7E1. An ohmmeter from a switch pin
      to ground and to Vcc, powered off, is enough. Pull-ups (closed = 0) are
      expected and would **invert every table**.
      - Result:
- [ ] **Confirm the switch-to-port wiring** rather than assuming the two
      conventions above: continuity from the `S1` end pin to 81C55 PB0 and the
      `S9` end pin to PA0 (pins 21-28 are PB0-PB7, 29-36 PA0-PA7).
      - Result:
- [ ] **Is `S17` port C bit 5?** Worth knowing on its own — it selects which of
      the two protocol families the unit speaks. Its as-found **open** position
      tells us nothing about polarity, because either setting receives.
      - Result:
- [ ] **What do the two daughter-2 switches feed?** **Not U20's port C** — that
      is in ALT2, all six pins outputs, so nothing can be read in on it. They must
      reach U2's port A/B/C like the others, or something outside the 81C55s.
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
      and framing simultaneously. **But the unit must be polled first** — that
      frame belongs to `rx_handler_c`, which only ever replies; re-checked
      2026-08-08, the firmware transmits nothing unprompted. The earlier note
      claiming it transmits continuously is retracted.
      → [docs/serial-protocol.md](docs/serial-protocol.md#the-transmitted-status-frame-c)
      - Result:
- [ ] **So what drove the transitions seen on the DB37 pins?** They were
      attributed to the unit's own status frame; that explanation is now dead and
      nothing replaces it. Re-scope with the link quiet and note whether the
      activity is periodic.
      - Result:
- [x] **Electrical standard identified: opto-isolated RS-485 / RS-422.** Daughter
      1 carries **2 x TI SN75176BP** differential transceivers (the two black
      DIP8s) and **3 x HP 6N139** optocouplers (the three yellow ones) — owner,
      2026-09. No diode test needed; no current loop.
      → [hardware/connectors.md](hardware/connectors.md#the-physical-layer-opto-isolated-rs-422rs-485-c)
- [x] ~~120 Ω termination?~~ Absent, and **no longer evidence of anything**:
      RS-485 terminates only at the two ends of the bus, and a station sign is
      unlikely to be an end node. Your host end will need the 120 Ω.
- [x] **The converter is genuinely isolated.** Four nets (`VI+`/`VI-`/`VO+`/`VO-`)
      with no shared ground — a buck or linear regulator would expose three pins.
      The doubled through-holes per pin are just current capacity and anchoring.
      → [hardware/pcbs.md](hardware/pcbs.md#the-dc-dc-converter)
- [x] **Converter identified: `24 IMP 3-05-7`, Melcher IMP series** (owner,
      2026-09): **10-36 V DC in, 5 V / 500 mA out**, isolated. It is the
      **line-side** supply — the motherboard's own TO-220 regulators make the
      logic rails. → [hardware/pcbs.md](hardware/pcbs.md#the-dc-dc-converter)
- [x] ~~Find the logic 5 V regulator.~~ **Already documented, and never missing:
      two TO-220 regulators on gold heatsinks, on the motherboard**
      ([hardware/pcbs.md](hardware/pcbs.md#motherboard)).
- [x] ~~Trace the ~1 kΩ resistors / measure their value / are they loop-current
      setting?~~ **All moot.** With SN75176BPs on the board, the 560 Ω are 6N139
      LED drive and the ~1 kΩ are its output pull-ups. Three of each because three
      signals cross the barrier: TX data, RX data, driver enable.
- [x] **4-wire confirmed** (owner, 2026-09): each cable pair lands on the `A`/`B`
      pins of a *different* SN75176BP. Two independent half-duplex channels are
      ruled out by there being one USART and three optocouplers. **Buy a 4-wire
      RS-422 adapter, not a 2-wire RS-485 dongle.**
      → [hardware/connectors.md](hardware/connectors.md#it-is-4-wire-one-pair-per-transceiver-c)
- [ ] **Which pair is inbound, which outbound?** Follow `DI` (pin 4) and `RO`
      (pin 1) on each SN75176BP back through the optocouplers; the package whose
      `DI` is driven feeds the pair the display **transmits** on. Then map both
      back to DB37 pin numbers and wire colours.
      - Result — display transmits on:
      - Result — display receives on:
- [ ] **Follow `DE` (pin 3) back.** If it reaches the 8251A's RTS through an
      optocoupler, the shared-return / turnaround reading is confirmed.
      - Result:
- [ ] **Photograph the converter's QA sticker legibly, then remove it.** It looks
      handwritten, something like `30-01-9?` / `OK`, which would be the **only date
      evidence on the board** — the chassis plate has a serial but no date. Once
      peeled it is gone.
      - Result — sticker text:
- [ ] **How are the 4 diodes arranged?** Previously framed as a current-loop
      test (a full bridge would mean a polarity-insensitive loop input). With the
      link now known to be RS-485 they are more likely transient protection across
      the pairs.
      - Result:
- [ ] **Three of each resistor value suggests three signals per direction**, not
      one. Do not assume only TX and RX exist when tracing — look for a third
      (RTS, or a second channel for the LED matrix).
- [ ] **Is the transmit side passive or active?** Does anything on daughter 1
      supply current to the TX pair, or are the yellow DIP8s bare switches expecting an
      external supply?
      - Result:
- [ ] **Locate the transmit driver.** Part numbers on the three yellow DIP8s
      on daughter 1). If they are optocouplers, whichever pair they drive is TX.
      - Result:

## P3 — Settles specific open questions

- [x] **U20 top marking read: OKI M81C55-5** (owner, 2026-09) — the same part as
      U2. This closed assumption **A7**, and with it the project's only recorded
      contradiction. There was never an ALT4 problem: both command words carry
      bits 3:2 = `11`, which is **ALT2, all six port C pins output**, exactly what
      the flip-dot drive needs. The "ALT4" reading was our own decode error.
      → [docs/io-map.md](docs/io-map.md#port-c-is-in-alt2--all-six-pins-are-outputs-c)
- [x] ~~U19 marking.~~ It is a **third ULN2003A** (ST), so daughter 2 has 21
      Darlington sinks against the 81C55's 22 output bits — near enough
      one-for-one to look deliberate.
- [ ] **U8, U9 (DIP16), U10 (DIP20)** markings — motherboard.
      - Result:
- [ ] **Where does the flip-dot high side come from?** ULN2003As can only sink,
      but a dot needs current both ways. Either the panel carries high-side
      drive, or each dot has separate set and reset windings returning through
      their own sinks. → [docs/flipdot-interface.md](docs/flipdot-interface.md)
      - Result:
- [ ] **Is the clock board fitted?** `0xC0-0xCF` is a clock — four `HHMM` digit
      registers written into the first four display positions, plus a
      time-setting keypad on `0xCD`. `clock_service` bails on `IN 0xCD == 0xFF`,
      so an open bus disables the whole feature silently. Look for a keypad and
      four more digits on the ribbon. → [docs/io-map.md](docs/io-map.md#0xc0-0xcf--a-clock-with-keys-c)
      - Result:
- [ ] **Does RST7.5 fire?** Scope on 8085 pin 7. It drives the watchdog, the
      attribute alternator, every `wait_ticks` and the keypad debounce.
      - Result:
- [x] **Panel ribbon conductor count: 26.** Confirms the select-line drive model
      and rules out position being decoded on the panel.
      → [docs/flipdot-interface.md](docs/flipdot-interface.md#the-ribbon-has-26-conductors-c)
- [ ] **Trace the 26 conductors.** The address is **encoded, not one-hot**, so look
      for a **group carrying a binary count** (~6-7 conductors), the **two that
      toggle per data bit** (clock and data), a strobe, and supply.
      - Result — address group:
      - Result — clock and data:
      - Result — supply:
- [ ] **Follow the character bus out of the enclosure — what does it reach?** The
      firmware addresses **positions 0-63** on one bus (confirmed by instrumenting
      port `0xE2` in the emulator) and the flip-dot line is 16 of them, so 48
      addresses go somewhere unknown. Three guesses are on the table: the LED row,
      the far side of a double-sided pair, or nothing at all.
      → [docs/flipdot-interface.md](docs/flipdot-interface.md#but-we-are-guessing-three-readings-of-positions-16-63-)
      - Result:
- [ ] **Instrument the ribbon and send a 64-character message.** Watch which
      addresses draw a response. **This one test separates all three guesses**:
      nothing responding past position 15 rules out the LED-on-the-bus reading
      outright.
      - Result:
- [ ] **Inspect the FRONT of the display modules for address switches or straps.**
      Only the rear has been seen. An encoded broadcast address means each module
      must know its own position somehow, so something must configure it.
      - Result:
- [ ] **Part numbers on the 9 transistors near the ribbon** (each with a diode and
      a 22 kΩ resistor). Are they bus drivers for the address/clock/data lines, or
      the **missing high-side drivers** the ULN2003As cannot provide? Which side of
      the load they sit on decides it.
      → [hardware/pcbs.md](hardware/pcbs.md#the-9-discrete-transistor-stages)
      - Result — part numbers:
      - Result — high side or bus driver:
- [ ] **Ribbon pitch and header type** (26-way IDC expected).
      - Result:
- [ ] **Test the missing-RAM prediction.** With the LED panel absent the unit
      should still sweep at power-on and still answer `SUB`, but should be
      **unable to display sent text** — because the message buffer may live in RAM
      on the absent expansion. If it *does* display text, that reasoning is wrong.
      → [docs/flipdot-interface.md](docs/flipdot-interface.md#a-falsifiable-prediction-about-the-missing-led-panel)
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

**Host: ESP32 is the recommendation.** Three hardware UARTs with parity (which
rules out Arduino's `SoftwareSerial` and the Pi's mini-UART), WiFi for pushing
text, and enough capability to replace the original upstream controller outright —
the display itself computes nothing. Watch the 3.3 V logic level: use MAX3485 /
MAX3490, **not** the 5 V MAX485/MAX490 on most cheap breakouts.

- [ ] Decide the interface hardware once the diode test or a clean capture
      settles the electrical standard.
- [ ] Plan on bringing the link up on a PC with a USB adapter first, then moving
      the working protocol to an ESP32 for the permanent installation.

## P4b — Long shots worth an hour

- [ ] Any original controller, or any recording of the line, would nail the
      protocol outright.
- [ ] PAGE Ibérica still operates in Tres Cantos. A polite email asking for
      PI-T100 documentation costs nothing —
      see [docs/references.md](docs/references.md).
- [ ] **Transcribe every silkscreen part number from the boards, carefully.**
      These are Telenorma drawing numbers and are the **single best lever for
      finding documentation** — far more specific than any product name. Read from
      photos so far and needing verification: `300-48.0216.5586` /
      `324-48.0216.5586` (mother), `300-40.0219.9078` / `324-40.0219.9078`
      (daughter 1), `41.0222.0026`, `11.0238.4030`, `37623.6`. Also check
      daughter 2, which has not been transcribed at all.
      → [hardware/pcbs.md](hardware/pcbs.md#telenorma-part-numbers-c--but-verify-the-digits-against-the-board)
      - Result — mother:
      - Result — daughter 1:
      - Result — daughter 2:
- [ ] **Photograph the Telenorma logo** on the motherboard silkscreen, for the
      record.
- [ ] **Chase the TELENORMA lead.** Both the EPROM label and the motherboard logo
      are Telenorma, so hardware *and* firmware originate with the German clock
      and time-distribution firm; PAGE Ibérica supplied the sign around them. The
      documentation hunt is therefore German, not Spanish — Telenorma /
      Telefonbau und Normalzeit / Bosch Telecom passenger-information and
      clock-bus material. → [docs/references.md](docs/references.md)
- [ ] Verify the Telenorma corporate history properly (Telefonbau und Normalzeit
      → Telenorma → Bosch Telecom, clock systems). Currently recorded from
      recollection and marked **[L]**.
- [x] **EPROM provenance complete.** Second read taken, SHA-256 identical, and
      the UV window is covered by the TELENORMA sticker — which therefore must
      **not** be removed: it is both the date evidence and the UV shield.
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
- [x] **Port C bit 5 selects the protocol family**, not merely an init path, and
      **not** a "serial enable" as recorded until 2026-08-08. Clear routes RST5.5
      to `rx_handler_c` (`0x2C80`) at a hardwired 9600 8E1 — a different binary
      protocol, not silence. → [docs/serial-protocol.md](docs/serial-protocol.md)
- [x] **`0x04` EOT sub-modes decoded and verified.** `'1'` writes 64 bytes to
      `0xE084`; `'2'` takes a 16-bit offset and count and block-writes at
      `0xE084 + offset`; **`'3'` stores the next byte verbatim, bypassing the
      command dispatcher** — the only way to display CP437's control-code glyphs,
      which are otherwise silently eaten. Anything else latches error `'0'` and
      behaves like `STX`.
      → [serial-protocol.md](docs/serial-protocol.md#eot-is-a-block-write-command-c)
- [x] **`0x08` BS status report decoded.** Replies `BS <status>`; in this build
      the byte is the latched accumulator `0xA024` and **reading clears it**. With
      ROM `0x008B` non-zero it would instead report the inverted state of I/O port
      `0xDE`, which nothing else in the ROM touches.
      - Open: what sets `0xA024`? A non-zero reading is currently uninterpretable.
- [x] **`SI` reads the shadow buffer, not the message buffer** — so it reports
      what is *displayed*. Confirmed end to end: `SI` returns spaces while the unit
      is still selected and `CHAMARTIN` after deselecting. That single experiment
      validates the interlock, the shadow and `SI` together.
- [x] **Display attributes decoded.** `BEL` stores `param & 7` in `0xA014`; only
      **bits 1:0 reach this panel**, driven out on **port C bits 1:0** as two
      attribute lines (four states, not eight). Bit 2 marks effect 0 permanent,
      exempting it from the countdown that `0x199F` runs against `0xA016`/`0xA018`
      — which also explains why handler A's `0x7C` sets two bits. Other panel
      families route the same two bits to port B instead.
      → [docs/flipdot-interface.md](docs/flipdot-interface.md#port-c-bits-10-are-the-display-attribute-lines-c)
- [ ] **What do the two attribute lines physically do?** Not brightness (the dots
      are mechanical) and **not the fluorescent tube** (electrically independent).
      Most likely they target the **LED matrix** further along the same ribbon, or
      are a hardware blink enable. Trace port C bits 1:0 through the ribbon, or
      send `BEL 0`-`BEL 3` to a powered unit and watch. **[?]**
      - Result:
- [x] **Port A fully attributed**: bits 4:0 address, bit 5 blank-substitute
      character, bit 6 protocol select, bit 7 a render option. The address also
      forms handler A's start-of-frame byte as `0x60 | addr`, which is the same
      value the status frame carries.
- [x] **Port A bit 7 decoded: blank or preserve the panel on reset.** Clear fills
      the buffer with spaces and the shadow with blocks, so every character differs
      and the panel repaints blank; set copies the buffer into the shadow, so
      nothing differs and the dots keep their image. Exactly the option bistable
      dots call for.
      → [docs/dip-switches.md](docs/dip-switches.md#port-a-bit-7--blank-or-preserve-the-panel-on-reset-c)
- [ ] **Which port C bits 4:0 value matches this panel?** They select the display
      type via a 32-way dispatch at `0x055A`; seven values have distinct
      handlers. → [docs/dip-switches.md](docs/dip-switches.md)
- [ ] **Why does the panel sweep at power-on but the CPU issue no per-dot
      writes?** Suspect a hardware sequencer on daughter 2 strobed by the pulse
      at `0x03FF`. → [docs/flipdot-interface.md](docs/flipdot-interface.md)

- [x] **Status frame explained**: `CAN CAN RS NAK <0x60|addr>`, with the
      address coming from port A. Two variants differing in bits 7:6 of the last
      byte — those flag bits are still unidentified.
- [x] ~~The stack reaching `0x9FFF`.~~ **Retracted — it was an emulator bug.**
      `poll_interrupts` latched RST5.5 instead of tracking RxRDY as a level, so
      the receive handler re-entered indefinitely on stale data. Fixed; the stack
      now peaks at `0xA0C1`, using 50 of the 243 available bytes, with zero writes
      outside mapped memory. There is nothing to explain.
- [x] **Render path traced end to end.** The buffer is 64 bytes at `0xE001` with a
      64-byte shadow at `0xE043`; two passes (`0x124E` paged, `0x13F7` unpaged,
      chosen by port B bit 0) walk them in step and repaint only what differs.
      `panel_view.py --text` now renders real text.
      → [io-map.md](docs/io-map.md#what-is-in-the-window-c-from-emulation)
- [x] **Why message text did not render: solved, two causes.** A *selected* unit
      never repaints (the task spins at `0x130B`), so `RS` is mandatory; and port B
      bit 0 set makes the pad character a solid block, which is what the "fill
      character" actually was.
      → [serial-protocol.md](docs/serial-protocol.md#the-repaint-interlock-rs-is-not-optional-c)
- [x] **Whole-panel versus changed-dots settled.** `0xA023` splits the supported
      displays: types 2, 8, 9, 0x0B-0x0E refresh continuously, everything else
      including this panel's `0x11` updates differentially. A standalone unit
      (port C bit 5 clear) also refreshes continuously.
- [x] **The `0xE084` region identified: a second buffer/shadow pair**, not an
      attribute plane. `0xE084` + `0xE0C6` with the same `0x42` stride as
      `0xE001` + `0xE043`, and the render path substitutes one pair for the other
      on `0xA057`. That flag is **never set** — its only writer is behind
      `MVI A,0x01 / CPI 0x01 / JZ`, an always-taken jump — so this is a second
      display patched out at build time. `EOT` still writes there, invisibly.
      → [docs/io-map.md](docs/io-map.md#the-second-buffer-pair-is-a-disabled-feature-c)
- [ ] What are the 97-iteration loop at `0x03CE` and `0xA021` (the 16-vs-32
      character count) for?
- [x] **Code page identified: IBM CP437.** Every region matches — control
      pictograms, accented Latin, box drawing, Greek and maths. `Ñ` is `0xA5`,
      and `text.encode("cp437")` produces correct bytes.
      → [docs/font.md](docs/font.md)
- [x] **`0x24` and `0x25` identified: `ñ` and `Ñ`.** They duplicate CP437's
      `0xA4`/`0xA5` at exactly the codes a 7-bit link delivers them to
      (`0xA4 & 0x7F = 0x24`, `0xA5 & 0x7F = 0x25`) — the two characters Spanish
      cannot do without, placed so they survive the link. Agrees with the switches
      decoding to 7 data bits and with the firmware's pervasive `ANI 0x7F`.
      → [docs/font.md](docs/font.md#the-two-substitutions-ñ-and-ñ-reachable-over-a-7-bit-link-c)
- [x] **8085 disassembler with symbol annotation and cross-reference search** —
      `tools/disasm.py`. Reproduces the hand-annotated listings in the docs
      exactly, and is the tool the matching disassembly in `src/` will need.
- [ ] **Which panel type does the differential/continuous split imply?** The
      classes are known; matching them to the seven distinct drivers would narrow
      down which port C value belongs to a flip-dot panel without the hardware.

- [x] **Matching disassembly done and verified.** `src/firmware.asm` reassembles
      byte-identically to the ROM over `0000h-3047h` (12 360 bytes) under Macro
      Assembler AS. Generated by `make disasm` from `tools/disasm.py --asm`, so it
      tracks `ghidra/symbols.csv`: every symbol added improves it.
      → [src/README.md](src/README.md)
- [x] **Every `LXXXX` label is gone.** 553 labels: **174 real names** in
      `symbols.csv` — all 41 unnamed `CALL` targets, the 13 per-family panel
      drivers and their flush hooks — and the other 379 generated as
      `<routine>_<role><n>` from the control flow, so a label says which routine
      it is in and whether it is a loop head, a shared epilogue or a forward
      branch. → [src/README.md](src/README.md#how-the-other-379-labels-are-named)
- [ ] **Give the remaining structural labels real names.** The `_br`/`_loop`
      names are navigation, not meaning. The two routines worth reading next are
      `rx_handler_c` (`0x2C80`, 31 labels — a third receive path, entered when
      `sw_port_c` bit 5 is clear) and `init_uart_from_switches` (19).
- [x] **Second character set decoded: 3 x 5.** Each 9-byte entry is `03h` plus two
      30-bit groups; within a group the 3 columns occupy 10-bit fields and the glyph
      is the top 5 bits of each. All 42 characters render legibly.
      **Not a 6x7 layout** — every 6x7 arrangement renders as noise.
      → [docs/font.md](docs/font.md#the-second-character-set-at-0x2692--3-x-5-decoded-c)
- [x] **Third glyph table found: 4 x 7 at `0x23CF`.** 59 entries, codes `0x20-0x5A`,
      columns stored right to left, reached by a linear search at `0x234B`. Three
      searches (LXI bases, bit-7-clear runs, pointer tables) now agree there is no
      fourth. → [docs/font.md](docs/font.md#the-rom-holds-three-glyph-tables-c)
- [ ] **What are the low 5 bits of each column field, and group B?** The spare bits
      hold something close to a row-shifted copy of the glyph, and group B differs
      from group A for 36 of 42 characters. Neither is explained. Nor is why 80
      clocked bits are spent placing a 3 x 5 glyph. **[?]**

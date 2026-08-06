# Lab notebook

Chronological, newest first. One entry per session. Record what was *tried*, not
just what worked — a hypothesis that failed is the most useful thing in the file
six months later, because it stops you re-running it.

Settled conclusions belong in [docs/](docs/); this file is the trail that leads
to them.

Entry template:

```
## YYYY-MM-DD — short title

**Did:**
**Found:**
**Concluded:** (with a [C]/[L]/[?] mark)
**Next:**
```

---

## 2026-08-05 — The whole command protocol, validated end to end

**Did:** Decoded the remaining handler-B commands, then drove the real firmware
in the simulator and captured its replies.

**The unblocking discovery: port C bit 5 is a serial enable.** Two sites test it
— the init path at `0x0430` *and* the RST5.5 ISR at `0x0268`. With it clear the
ISR returns immediately, so **the unit never receives anything**. Every earlier
experiment ran with bit 5 clear (`--port-c 0xDF`) and was therefore testing a
deaf machine. That also reframes the "9600 8E1 default": that configuration
belongs to a unit that cannot receive. A working installation uses the port B
switches.

**Second key discovery: the unit must be addressed before it listens.** Handler B
guards its dispatch table on a *selected* flag at `0xA040`. Until it is set, only
`EOT`, `VT` and `GS` are accepted. `GS <addr>` selects — comparing against the
port A address, with **`0x7F` as a broadcast** — and `RS` deselects. So every
transaction is `GS <addr> <commands> [RS]`.

**The command set is a small cursor-addressable terminal.** The 64-byte buffer is
rows of 64 columns with the cursor in `0xA043`/`0xA044`: `VT <row> <col>`
positions (two ASCII-digit parameters), `CR`/`LF`/`DC1`/`DC2`/`DC3`/`DC4` move,
`ETB`/`EM` erase to end of line/display, `CAN` clears, `FF` fills with `#`.

**Five commands make it answer with real data**, each wrapped in ROM-defined
prefix/suffix strings: `SUB` identifies, `TAB` reports the cursor, `SO` reads the
character at the cursor, `SI` reads back the display, `BS` reports status.

**Verified in emulation** — actual captured replies:

```
GS '5' SUB           -> 16 16 16 16 "AK0052.139U1" + the config block
GS 0x7F SUB          -> identical (broadcast works)
GS '5' TAB           -> "00"
GS '5' STX HOLA ETX  -> ACK '5'
GS '5' ... SI        -> ACK '5' then the buffer contents
GS '9' SUB           -> nothing at all
```

**Concluded:**

* All of it **[C]**: read from the listing *and* reproduced by executing the ROM.
* `SUB` is the best bring-up command — no prior state, works under broadcast, and
  returns a known-answer string while dumping the unit's own config block.
* `ESC` is **disabled in this build** (`0x0E8C` requires ROM `0x0077 == 1`; it is
  `0x00`).

**Also closed port A completely** by enumerating every `IN 0xA1` site: bits 4:0
address, bit 5 selects the blank-substitute character (`'?'` or space), bit 6
picks the receive protocol, bit 7 is a render option. And `0x055A` computes
`0x60 | (port A & 0x1F)` into `0xA006` — **handler A's start-of-frame byte is the
display address**, the same value the status frame carries as its last byte. The
two protocols share one addressing scheme.

**Next:** the `EOT` sub-modes and what `BS` returns; then the render path and the
8 display attributes, which need the panel to interpret.

---

## 2026-08-05 — The code page is IBM CP437

**Did:** Rendered the whole glyph table to a PNG map (`make font-map`) and read
it, then spot-checked individual code points against the raw bytes.

**Found:** **The encoding is IBM Code Page 437**, and every region of the table
agrees: `0x01-0x0F` smileys and card suits, `0x10-0x1F` the arrow set,
`0x80-0x9F` the accented Latin block, `0xA0-0xAF` ending in `¿ ⌐ ¬ ½ ¼ ¡ « »`,
`0xB0-0xDF` shading and box drawing, `0xE0-0xEF` Greek, `0xF0-0xFE` maths.
Byte-level spot checks: `0xA5` is an unmistakable `Ñ`, `0xA8` `¿`, `0xAD` `¡`,
`0x82` `é`.

**Two substitutions**: `0x24` and `0x25` are *not* `$` and `%`. Both are accented
Latin capitals, and `0x25` reads clearly as a second `Ñ` — exactly what a Spanish
installation needs when the link or the controller is not 8-bit clean. `0x24` is
another accented capital of uncertain identity.

**Concluded:**

* CP437 for `0x80-0xFF` and the control ranges **[C]**. This **corrects**
  `docs/font.md`, which previously said "do not assume Latin-1 or IBM code page
  437" — the evidence now says otherwise, across seven independent regions.
* Practically: a controller can just use `text.encode("cp437")`, but must avoid
  `$` and `%`.
* `0x24`'s identity **[?]**.

**Next:** the remaining handler-B commands, and what the 8 display attributes do.

---

## 2026-08-05 — Handler B command semantics; error reporting and a config block

**Did:** Followed the dispatch targets for STX, ETX, CAN and RS, and the state
variables that turn some commands into parameterised ones.

**Found:**

* **The message buffer is `0xE001-0xE040`, 64 bytes**, and the length is read
  from the ROM constant at `0x007B` rather than hard-coded. Overflow is caught at
  `0x0C41`: the pointer is clamped and error code `0x30` latched, so an
  over-long message gets NAKed instead of silently truncated. A second buffer
  sits at `0xE084`. 64 bytes for a 16-character panel means the surplus is for
  scrolling.
* **The reply is richer than just ACK/NAK.** Full frame:
  `[4xSYN] <ACK|NAK> <address> [errcode [param1 param2]]`. Error codes are
  printable ASCII; `0x30` is overflow, `0x32` is another bare failure, and any
  other code carries two detail bytes from `0xA050`/`0xA051`. A NAK says *why*.
* `0x1A7F` does line turnaround before replying: re-issues the 8251A command
  word to drive RTS, then optionally four `SYN` bytes or a fixed delay. **That
  explains why `0x16` SYN has no receive handler — it is a transmit-side
  preamble.**
* Command semantics: **CAN** clears the display (fills 64 bytes with spaces, or
  the `0xE084` buffer with zero, then posts `0x4A` to repaint). **RS** resets the
  receive state but is *ignored when the unit's address is 0*. **DLE** resets the
  unit. **ACK**, oddly, also clears the display.
* **Three commands take a parameter byte** via a state variable, which is what
  those state variables at the top of handler B are for: `BEL` (low 3 bits of the
  parameter become the display attribute `0xA014` — 8 effects), `ENQ` (parameter
  to `0xA041`, with DC2 translated to `'#'`), and `GS` (parameter is an address,
  matched against port A). That is why `0x1D` GS never appears in the dispatch
  table — it is consumed a level earlier.
* **`0x0070-0x008B` is a build-time configuration block.** Buffer length, address
  mode, turnaround options and feature bits all come from there. The same
  firmware was clearly customised per installation by patching these bytes.

**Concluded:**

* All of the above is **[C]** from the listing.
* **Caveat worth carrying forward:** anyone building a controller should read the
  config block out of *their own* ROM. A different PI-T100 may be configured with
  a different buffer length, addressing mode or turnaround behaviour, and nothing
  in the protocol announces it.

**Next:** the remaining ~19 handler-B commands, and what the 8 display attribute
values actually look like on the panel.

---

## 2026-08-05 — Handler B is a command protocol; the unit answers ACK/NAK

**Did:** Extracted the handler B (`0x0ACB`) dispatch table, followed the ETX
path, and used the simulator to sweep DIP switch port A.

**Found:**

* **Handler B is a 28-command protocol** keyed on ASCII control codes, with a
  multi-state parser: several state variables each divert following bytes to a
  different continuation, so commands take parameters. `STX` opens a message,
  `ETX` closes it, and **`DLE` jumps straight to `fatal_or_init` — a reset
  command**. `0x01`, `0x15`, `0x16` and `0x1D` have no handler, so they are
  transmit-side only.
* **On ETX the unit replies `ACK` (0x06) or `NAK` (0x15)** depending on an error
  flag accumulated at `0xA03A`, then sends its address. This is the most useful
  single fact found so far: it turns protocol discovery from reading into
  experiment, and gives an unambiguous link test that does not depend on the
  panel being healthy.
* **DIP switch port A bits 4:0 are the display address.** Confirmed twice: from
  `0x0F69`, which encodes them as printable ASCII (`0x30-0x3F` when bit 4 is
  clear, `0x40-0x4F` when set — 32 addresses); and by experiment, sweeping port A
  in the simulator and watching the transmitted frame's last byte track
  `0x60 | (port A & 0x1F)` exactly.
* Port A bit 6 selects which receive protocol runs. That accounts for most of
  port A; bits 7 and 5 remain unknown.
* The continuously transmitted frame decodes as **`CAN CAN RS NAK <0x60|addr>`**,
  with a second variant at `0xA0|addr`.

**Concluded:**

* The command table and the ACK/NAK behaviour are **[C]**. What each individual
  command *does* is still **[?]** — the dispatch targets are known, the semantics
  are not.
* The status frame is **[C]** as an observation. Calling it a beacon rather than
  a poll is **[L]**, and it cuts against the MAESTRA reading: this unit may be
  master of its *display chain* while being a slave on the *serial bus*.
* Revised yesterday's guess that the frame was a master polling slaves.

**Next:** decode the individual handler-B commands, starting with `0x0F17` STX,
`0x0F29` ETX, `0x101E` CAN and `0x10A3` RS. Then the `0xA014` attribute bits.

---

## 2026-08-05 — Receive handler A decoded; emulator running the real ROM

**Did:** Disassembled `rx_handler_a` (`0x08B7`) and built an 8085 emulator
(`tools/emu8085.py`) plus a machine model (`tools/pit100_sim.py`) that boots the
actual firmware against stub peripherals.

**Found — the framing, from the listing:**

* **Start and end delimiters are runtime variables**, not literals: `0xA006` and
  `0xA007`. A second start byte (`0xA006 + 1`) selects an alternate buffer at
  `0xE03C`. Messages accumulate at `0xE000`, pointer in `0xA00E`.
* On the end byte the handler posts **`0x4A`** to the mailbox — that is the
  "message complete" signal the main loop waits for at `0x03A0`.
* Per-character filtering is layered: bit 7 set means skip-but-advance; `0x00-0x1F`
  is ignored; `0x60-0x7F` changes state without storing; a configurable byte at
  `0xA00B` stores as a space.
* In-band control characters `0x7C`, `0x7D`, `0x7F` and (in an extended sub-mode)
  `0x64`/`0x65`/`0x66` write an attribute byte `0xA014` and duration words
  `0xA016`/`0xA018`. These are display effects with timings.
* **A parity error inserts `0x23` `#` into the message** and re-issues command
  `0x17`. Externally observable, and a good live-link test.

**Found — from running it:**

* The emulator reproduces the documented cold-boot 8251A init exactly: mode
  `0x7E`, command `0x37`, with port C bit 5 clear. Independent confirmation that
  both the listing reading and the model are right.
* With port C bit 5 *set* the switch-derived path produces mode `0xFA` instead,
  as `docs/dip-switches.md` predicts.
* **The firmware transmits continuously**: a 5-byte frame `18 18 1E 15 7F`, with
  `18 18 1E 15 BF` periodically substituted. Only two distinct frames in ~2000
  bytes. Given the MAESTRA label this looks like a poll or beacon, and the last
  byte differs only in bits 7:6 — an address or flag field. Note `0x7F` is also
  what `0xA006` holds.
* Both transmit routines poll TxRDY before writing. The one at `0x1A1A` runs
  configurable inter-character delay loops first, which is what a half-duplex or
  current-loop link needs for turnaround.

**Concluded:**

* Framing structure **[C]**; the meaning of the individual `0xA014` attribute
  bits **[?]**.
* The beacon is **[C]** as an observation of the firmware, **[L]** as "a poll".
* **The firmware writes to `0xE000-0xE083` even with no expansion board and
  handler A selected.** So something must answer there. The simulator offers
  `none` / `board` / `alias` and reports unbacked writes; deciding between them
  needs the hardware.

**Next:** handler B (`0x0ACB`), then the `0xA014` attribute bits. Also worth
chasing: ~120 emulated writes land at `0x9FFD-0x9FFF`, i.e. the stack dipping
below `0xA000`. Either a model artefact or a real clue about RAM size.

---

## 2026-08-05 — Baud ladder, stack, and the receive architecture

**Did:** Worked through the "cheap wins, no hardware needed" list in
`docs/open-questions.md`. All of it was mechanical listing reading.

**Found:**

* **The complete baud table.** The five unread branch targets give dividers
  40, 80, 160, 320, 640 alongside the default 20 — an exact 1:2:4:8:16:32
  ladder. `baud = TIMER_IN / (divider x 16)`, so one frequency measurement fixes
  all six rates. 3.072 MHz gives 9600 down to 300.
* **Port B is fully attributed.** `0x04EA` builds the 8251A mode word from port
  B: bits 7:5 baud, bit 4 word length (7 vs 8 data bits), bits 3:2 parity
  (off / odd / even). The hardwired `0x7E` (8E1) is what the common switch
  setting produces.
* **Stack is at `0xA0F3`** (`LXI SP,0xA0F3` at `0x034C`), growing down. The RAM
  clear stops at `0xA0EF`, so it never touches the stack. The phantom Ghidra
  symbols at `0xA0ED`/`0xA0EF`/`0xA0F1` are stack slots.
* **RST5.5 is the serial receive interrupt.** It masks 5.5 and 6.5 but leaves
  7.5 unmasked and re-enables interrupts, so the tick can preempt reception. It
  is gated by DIP switch port C bit 5 — that switch turns reception off
  entirely. It then dispatches on `0xA00C` to **one of two receive handlers**,
  `0x08B7` or `0x0ACB`, both of which touch the 8251A.
* `0xA00C` is set at `0x0689` from either the expansion board or DIP switch port
  A bit 6. So there are **two protocols or two link roles**, chosen by
  configuration.
* **RST7.5 is a tick and watchdog**, not part of the receive path: reads `0xCD`,
  decrements `0xA01E`, and jumps to `fatal_or_init` on expiry. It also drives the
  8085 SOD pin as a GPIO output (`SIM` `0xC0` sets it, `0x0344` clears it).
* `0x4A`/`0x4E` at `0x03A0` are an **ISR/main-loop mailbox**, swapped atomically
  under `DI`/`EI`. The earlier guess that they might be `'J'`/`'N'` framing
  delimiters is wrong.

**Concluded:**

* Everything above is **[C]** from the listing. The only remaining unknown on the
  link layer is the absolute clock frequency, and the ladder's exactness makes
  even that low-risk.
* **Revised an earlier claim:** `0xC0-0xCF` was labelled "LED matrix interface,
  board absent". RST7.5 reads `0xCD` every tick and there is no absence check
  anywhere for `0xC0` — unlike the careful probing done for `0xE7`. Either the
  device is on the motherboard, or RST7.5 never fires here. Downgraded to **[?]**
  pending a scope probe on the RST7.5 pin.

**Next:** disassemble `0x08B7` and `0x0ACB` in full. The framing lives there and
needs no hardware. Then attribute DIP switch port A, which is the likeliest home
of a display address.

---

## 2026-08-05 — I/O port 0xE7 is a memory bank register, not a peer handshake

**Did:** Chased every one of the 11 `IN 0xE7` / 10 `OUT 0xE7` sites, after the
one-line "board detect" claim in the first pass was questioned.

**Found:** `0xE7` is a **bank/page register plus capability register on the
expansion board**, and the earlier description of it was too shallow.

* Three accessor functions wrap every banked memory access: `0x1B69` (read,
  21 callers), `0x1B95` (write, 21 callers), `0x1BC4` (compare, 4 callers).
* Each one reads `0xE7` first. `0xFF` means the expansion board is absent, and
  the accessor falls straight through to a plain `(HL)` access on local RAM.
  Otherwise it writes the target address's high byte to `0xE7` as a page select
  and forces the address into the `0xE000-0xFFFF` window with `ORI 0xE0`.
* `0xA05A` is set to 1 inside the ISRs (`0x0271`, `0x0296`); when set, the
  accessors also save the previous page in `0xA059` and restore it afterwards.
  That is re-entrancy protection, so an interrupt cannot leave the page
  switched under the code it interrupted.
* `0xE7` also carries status bits when a board is present: `0x112E` tests bit 4.
* This explains the stray `DAT_ram_e000`, `DAT_ram_e03b`, `DAT_ram_e07d`
  references in the Ghidra decompilation — they are banked addresses, not real
  locations in this machine.

**Concluded:**

* **There is no handshake with the expansion board, and no possibility of one.**
  All 11 read sites are immediate conditional branches. Not one loops, retries,
  waits, or has an error path. **[C]**
* **Standalone operation is the designed behaviour, not a lucky accident.**
  `0x0675` is the clincher: with the board absent the firmware reads DIP switch
  port A bit 6 instead, deliberately substituting a switch for a value the board
  would otherwise have supplied. **[C]**
* Corrected `docs/io-map.md`, `docs/memory-map.md` (the `0xE000-0xFFFF` window
  was previously listed as "unmapped [?]", which was wrong),
  `docs/flipdot-interface.md` and `ghidra/symbols.csv`.

**Next:** unchanged — the serial framing is still the blocker. The `0xE7` bits
cannot be probed further without an expansion board, so that question is parked.

---

## 2026-08-05 — Project structure, and a first pass over the ROM

**Did:** Reorganised the repository (see [README.md](README.md#layout)) and read
the Ghidra export looking for peripheral accesses.

**Found:**

* 322 `IN`/`OUT` instructions across ports `0x80-0x81`, `0xA0-0xA5`,
  `0xC0-0xCF`, `0xDE`, `0xE0-0xED`. Everything is in 8085 I/O space; nothing is
  memory-mapped except the 81C55's RAM at `0xA000`.
* `0x0443` is a textbook 8251A wake-up: 3 dummy writes, reset `0x40`, mode
  `0x7E`, command `0x37`. Mode `0x7E` decodes to 8 data bits / even parity /
  1 stop / x16 clock.
* `0x03F2` initialises **two** more 81C55-shaped register groups at `0xE0` and
  `0xE8`, and `IN 0xE7` at `0x040F` is a presence check that skips the second
  group when it reads `0xFF`.
* The 81C55's ports A/B/C are only ever *read* → all three are the DIP switch
  banks. Port B bits 7:5 index a 6-way baud table at `0x0474`; port C bit 5
  picks between two startup paths at `0x0430`.
* Only `0x0000-0x3047` of the 32 KiB EPROM is programmed; the rest is `0xFF`.
* Region `0x1C0D-0x210C` is a 5x7 font: 256 entries x 5 bytes, column-major,
  bit 0 = top row. Verified by rendering — `'0'`, `'A'`, `'B'` come out right,
  and bit 7 is clear in all 1280 bytes.
* Chassis sticker: **PAGE Ibérica S.A., PI-T100-M, MAESTRA**, "1 línea de texto,
  módulos de PLACA MÓVIL de 100 mm".

**Concluded:**

* Link layer is **9600 8E1** **[L on the 9600, C on the 8E1]** — the 9600
  depends on the 81C55 TIMER IN being 3.072 MHz, which has not been measured.
* Font format **[C]**.
* Master/slave with a switch-set display address **[L]** — inferred from
  "MAESTRA" plus the second-board probe, not yet from the protocol code.

**Next:** the application-layer framing. Follow the `IN 0x80` sites back to
their buffer and forward to the state machine at `0x03A0`, which compares
against the constants `0x4A`/`0x4E`. Also fill in the five missing rows of the
baud table — pure listing reading, no hardware needed.

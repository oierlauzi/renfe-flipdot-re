# Serial protocol

**This is the primary deliverable of the project.** Everything else exists to
make this document correct.

Goal: send bytes into the DB37 connector and get arbitrary text onto the
16-character panel, with no hardware modification.

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Physical layer

| Property | Value | Conf. |
| --- | --- | --- |
| Connector | DB37 female, only 4 conductors populated | **[C]** |
| Wire colours | orange, blue, green, yellow (grey overall shield) | **[C]** |
| Isolation | HP/Agilent 6N139 optocoupler (U7) on the motherboard | **[C]** |
| Signalling | 2 pairs — either RS-422/RS-485 or a 20 mA current loop | **[L]** |
| Pinout | **unknown** | **[?]** |

The 6N139 is the decisive clue: an opto-isolated receiver on a 1980s-90s
Spanish rail installation almost always means either a 20 mA current loop or an
isolated RS-422 link, not plain RS-232. Two pairs is consistent with
full-duplex (TX pair + RX pair). See [../hardware/connectors.md](../hardware/connectors.md)
for the measurement plan.

## Link layer **[C] unless noted**

From the 8251A initialisation (see [io-map.md](io-map.md#0x80-0x81--m82c51a-2-usart)):

| Property | Value |
| --- | --- |
| Mode | asynchronous |
| Data bits | 8 |
| Parity | even |
| Stop bits | 1 |
| Baud factor | x16 |
| Default line rate | **9600 baud [L]** — 81C55 timer divider 20, assuming a 3.072 MHz TIMER IN |

So the default is **9600 8E1**, pending a scope measurement of TIMER IN.

Both the frame format and the divider are overridden from the DIP switches at
power-on (`0x0533` writes the mode word from a register; `0x0474` picks the
timer preload from port B bits 7:5), so a unit in the field may be running
something else — see [dip-switches.md](dip-switches.md).

## Receive architecture **[C]**

**RST5.5 is the serial receive interrupt.** The handler at `0x0260`:

1. `MVI A,0x0B / SIM` masks RST5.5 and RST6.5 but leaves RST7.5 unmasked, then
   `EI` — so the timer tick can preempt the receive path.
2. `IN 0xA3 / ANI 0x20` — checks DIP switch port C bit 5 and does nothing if it
   is clear. **The mode switch gates reception entirely.**
3. Writes token `0x4E` to the mailbox byte `0xA012`.
4. Sets `0xA05A = 1` for the duration, so banked memory accessors save and
   restore their page (see [io-map.md](io-map.md#0xe7--expansion-memory-bank-register-c)).
5. Dispatches on `0xA00C`: **two different receive handlers**, `0x08B7` and
   `0x0ACB`. Both touch the 8251A. `0xA00C` is set at `0x0689` from either the
   expansion board or DIP switch port A bit 6 — so the unit implements **two
   protocols or two link roles**, selected by configuration.

RST7.5 (`0x02A9`) is a periodic tick, not part of the receive path: it services
the `0xC0` device and decrements a counter at `0xA01E` that jumps to
`fatal_or_init` on expiry — i.e. a **watchdog**. It also drives the 8085's SOD
pin (`SIM` with `0xC0` sets it, `0x0344` clears it), so SOD is used as a GPIO
output — possibly a heartbeat or an external watchdog strobe. **[L]**

### The 0xA012 mailbox

`0x03A0` in the main loop performs an atomic swap on `0xA012` under `DI`/`EI`:
write `0x4E`, and branch on whether the old value was `0x4A`. Combined with the
ISR writing `0x4E`, **`0x4A` and `0x4E` are ISR/main-loop handshake tokens, not
framing bytes** — an earlier guess that they might be `'J'`/`'N'` delimiters is
wrong.

## Frame format, receive handler A (`0x08B7`) **[C]**

Handler A runs when `0xA00C = 0` — no expansion board **and** DIP switch port A
bit 6 clear. It is a byte-at-a-time state machine driven by the RST5.5 ISR.

### Framing

| Element | Source | Notes |
| --- | --- | --- |
| Start-of-frame | byte at `0xA006` | **configurable at runtime**, not a literal |
| Alternate start | `0xA006 + 1` | selects a second buffer at `0xE03C` |
| End-of-frame | byte at `0xA007` | configurable; on match, posts `0x4A` to the mailbox |
| Buffer | `0xE000` (or `0xE03C`) | write pointer kept in `0xA00E` |

On the start byte the handler resets the write pointer to `0xE000` and stores the
byte. On the end byte it posts token **`0x4A`** to the mailbox at `0xA012`, which
is what the main loop at `0x03A0` is waiting for. **`0x4A` means "message
complete"**.

### Per-character filtering

In the order the handler tests them:

| Condition | Action |
| --- | --- |
| `== (0xA006)` | start of frame — reset buffer to `0xE000` |
| `== 0x7D` `}` | set duration `0xA016` to 80 or 2000 (per port C bit 5), `0xA014 \|= 0x02` |
| `== 0x7C` `\|` | `0xA014 = (0xA014 & 0x7F) \| 0x05` |
| `== 0x7F` DEL | `0xA014 = (0xA014 & 0xFA) \| 0x80` |
| `== (0xA007)` | end of frame — post `0x4A` |
| bit 7 set | **not stored**; write pointer still advances |
| bits 6:5 == `00` (`0x00-0x1F`) | ignored entirely |
| bits 6:5 == `11` (`0x60-0x7F`) | state change only, not stored |
| `== (0xA00B)` | stored as `0x20` (space) — a "blank" substitute character |
| otherwise | **stored**, pointer advances |

In an extended sub-mode (`0xA008 = 1`, entered at `0x097B`) further codes are
recognised: `0x11`, `0x12`, and `0x64`/`0x65`/`0x66` which set the duration
words at `0xA016`/`0xA018`.

**`0xA014` is a display attribute byte** and `0xA016`/`0xA018` are durations, so
these in-band control characters select display effects — static, scroll, blink
and their timings. Working out the exact meaning of each `0xA014` bit is the
natural next step.

### Error handling

The handler reads the 8251A status and masks `0x78` (PE, OE, FE, SYNDET):

| Status & 0x78 | Action |
| --- | --- |
| `0x00` | normal — read and process the byte |
| `0x08` (parity error only) | read and discard the byte, re-issue command `0x17`, and **substitute `0x23` `#`** into the message |
| anything else | full 8251A re-init (reset `0x40`, restore mode from `0xA005`, command `0x17`), post `0x4E` or `0x4A` |

**A parity error puts a `#` on the display.** That is a very characteristic,
externally observable behaviour and a good way to confirm the link is live.

## Transmit **[C]**

The unit does transmit. Two send routines, both polling TxRDY (`IN 0x81`,
`ANI 0x01`) before `OUT 0x80`:

* `0x1A43`, reached via `0x1A1A`, which runs configurable inter-character delay
  loops before sending — the signature of a half-duplex or current-loop link
  needing turnaround time.
* `0x303F`, via `0x3027` — called from 21 sites across `0x2D00-0x3000`, a whole
  transmit subsystem. It uses the watchdog counter `0xA01E` as a TxRDY timeout.

Run under emulation with default switches, the firmware continuously emits a
repeating **5-byte frame**:

```
18 18 1E 15 7F      and periodically      18 18 1E 15 BF
```

The two variants differ only in bits 7:6 of the last byte. Given the MAESTRA
label this looks like a **poll or beacon** — the master soliciting from slaves,
with the last byte carrying an address or a flag. `0x7F` is also the value found
in `0xA006` (the start-of-frame byte), which is unlikely to be a coincidence.

Reproduce with:

```sh
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0xBF --port-c 0xDF \
    --xram board --send '7F 48 4F 4C 41 03'
```

## Frame format, receive handler B (`0x0ACB`) **[C] for the table**

Handler B runs when `0xA00C = 1` — an expansion board is fitted, or DIP switch
port A bit 6 is set. It is a far richer, **command-oriented** protocol driven by
single-byte ASCII control codes, with a multi-state parser: several state
variables (`0xA03F`, `0xA040`, `0xA053`, `0xA055`, `0xA05D`, `0xA039`, `0xA03E`,
`0xA04C`) each divert incoming bytes to a different continuation, so a command
can consume following bytes as parameters.

### Command dispatch, from `0x0B76`

| Code | Name | Target | Notes |
| --- | --- | --- | --- |
| `0x00` | NUL | `0x0C70` | ignore |
| `0x02` | STX | `0x0F17` | **start of text** — sets `0xA042 = 1`, buffer to `0xE001` |
| `0x03` | ETX | `0x0F29` | **end of text** — validates and **replies ACK/NAK** |
| `0x04` | EOT | `0x110D` | |
| `0x05` | ENQ | `0x1035` | |
| `0x06` | ACK | `0x1057` | |
| `0x07` | BEL | `0x10C8` | |
| `0x08` | BS | `0x10E1` | |
| `0x09` | TAB | `0x0DF5` | |
| `0x0A` | LF | `0x0D31` | |
| `0x0B` | VT | `0x0CA7` | |
| `0x0C` | FF | `0x0FE9` | |
| `0x0D` | CR | `0x0D1B` | |
| `0x0E` | SO | `0x0E1A` | |
| `0x0F` | SI | `0x0E3C` | |
| `0x10` | DLE | **`0x034B`** | **jumps to `fatal_or_init` — a reset command** |
| `0x11` | DC1 | `0x0D54` | |
| `0x12` | DC2 | `0x0C1B` | |
| `0x13` | DC3 | `0x0D6D` | |
| `0x14` | DC4 | `0x0D94` | |
| `0x17` | ETB | `0x0DAA` | |
| `0x18` | CAN | `0x101E` | also the first byte of the status frame |
| `0x19` | EM | `0x0DC9` | |
| `0x1A` | SUB | `0x0E69` | |
| `0x1B` | ESC | `0x0E8C` | |
| `0x1C` | FS | `0x0C70` | ignore |
| `0x1E` | RS | `0x10A3` | also appears in the status frame |
| `0x1F` | US | `0x0C70` | ignore |

Bytes outside this table with the high nibble `0x0` or `0x1` are ignored
(`0x0C03`); everything else is message text.

`0x01` SOH, `0x15` NAK, `0x16` SYN and `0x1D` GS have **no handler** — they are
transmit-side only.

### The unit answers ACK or NAK **[C]**

The most useful fact in this document. On ETX (`0x0F48`):

```
0x0F48  LDA  0xA03A        ; error flag accumulated over the message
0x0F4B  CPI  0x00
0x0F4D  MVI  C,0x06        ; ACK
0x0F4F  JZ   0x0F54
0x0F52  MVI  C,0x15        ; NAK
0x0F54  MOV  A,C
0x0F55  CALL 0x1A0C        ; transmit it
```

**A well-formed `STX ... ETX` message is acknowledged; a bad one is NAKed.**
Once the link is physically connected this makes the rest of the protocol
discoverable by experiment rather than by reading: send a candidate frame, watch
for `0x06` or `0x15`. It also gives an unambiguous "the link works" test that
does not depend on the panel being healthy.

The reply is followed by the address byte built from DIP switch port A — see
[dip-switches.md](dip-switches.md#port-a-bits-40--display-address-c).

## The transmitted status frame **[C]**

Independently of either handler, the unit continuously emits:

```
18 18 1E 15 <0x60 | address>        and      18 18 1E 15 <0xA0 | address>
```

`CAN CAN RS NAK <addr>`. The address is DIP switch port A bits 4:0, verified by
sweeping it in the simulator. The two variants differ only in bits 7:6 of the
last byte, so those are flags — status, or "have data" versus "idle".

This is a slave announcing itself on a shared bus, not a master polling. Worth
weighing against the MAESTRA label: this unit may be the master *of its display
chain* while still being a slave *on the serial bus*.

## Still unknown

* The sticker reads **MAESTRA** ("master"), and DIP switch port A is largely
  unattributed. A master/slave addressing scheme on the serial bus is therefore
  likely, meaning frames probably carry a **display address** taken from those
  switches. **[L]**
* The main loop at `0x03B2` waits for `0xA07E & 0xC0 == 0x40` and
  `0xA07C & 0x01 == 0` before rendering 0x61 (97) items from a buffer at
  `0xA039`. Those are almost certainly the "message ready" flags. **[L]**

### Where to look next

1. **Disassemble `0x08B7` and `0x0ACB` fully.** These are the two receive
   handlers; everything about the framing is inside them. This is the single
   highest-value piece of work left and it needs no hardware.
2. Work out what selects between them, i.e. what `0xA00C` means, and which one
   a unit with no expansion board and default switches actually uses.
3. Look for the usual framing bytes in those routines: `STX` `0x02`, `ETX`
   `0x03`, `SOH` `0x01`, `EOT` `0x04`, `ACK` `0x06`, `NAK` `0x15`.
4. Check whether the reply path (`OUT 0x80`, only 2 sites) sends an ACK/NAK —
   if it does, the frame format becomes cheap to confirm by experiment.

## Protocol scratchpad

Record every hypothesis and every experiment result here, including the ones
that failed — a negative result is data.

| Date | Frame sent (hex) | Expected | Observed | Verdict |
| --- | --- | --- | --- | --- |
| | | | | |

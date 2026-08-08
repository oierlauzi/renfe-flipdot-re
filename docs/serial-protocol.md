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

From the 8251A initialisation (see [io-map.md](io-map.md#0x80-0x81--m82c51a-2-usart-c)):

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
2. `IN 0xA3 / ANI 0x20` — checks DIP switch port C bit 5. **Clear routes the
   whole interrupt to a third handler**, `rx_handler_c` at `0x2C80`
   (`JZ 0x02A3` → `CALL 0x2C80`), and the steps below never run. See
   [handler C](#handler-c-a-third-protocol-c).
3. Writes token `0x4E` to the mailbox byte `0xA012`.
4. Sets `0xA05A = 1` for the duration, so banked memory accessors save and
   restore their page (see [io-map.md](io-map.md#0xe7--expansion-memory-bank-register-c)).
5. Dispatches on `0xA00C`: handler **A** (`0x08B7`) or handler **B** (`0x0ACB`).
   `0xA00C` is set at `0x0689` from either the expansion board or DIP switch
   port A bit 6.

**So there are three receive handlers, not two**, and two independent switches
choose between them:

| port C bit 5 | `0xA00C` | Handler | Protocol |
| --- | --- | --- | --- |
| set | `!= 1` | **A** (`0x08B7`) | delimited byte stream into the buffer |
| set | `== 1` | **B** (`0x0ACB`) | the ASCII control-code command set, most of this document |
| **clear** | — | **C** (`0x2C80`) | a binary poll/select protocol, barely decoded |

RST7.5 (`0x02A9`) is a periodic tick, not part of the receive path: it decrements
a counter at `0xA01E` that jumps to `fatal_or_init` on expiry — i.e. a
**watchdog** — samples the status input `0xDE`, and debounces the clock board's
keypad ([io-map.md](io-map.md#0xc0-0xcf--a-clock-with-keys-c)). It also drives the
8085's SOD pin (`SIM` with `0xC0` sets it, `0x0344` clears it), so SOD is used as
a GPIO output — possibly a heartbeat or an external watchdog strobe. **[L]**

### The 0xA012 mailbox

`0x03A0` in the main loop performs an atomic swap on `0xA012` under `DI`/`EI`:
write `0x4E`, and branch on whether the old value was `0x4A`. Combined with the
ISR writing `0x4E`, **`0x4A` and `0x4E` are ISR/main-loop handshake tokens, not
framing bytes** and not `'J'`/`'N'` delimiters.

## Frame format, receive handler A (`0x08B7`) **[C]**

Handler A runs when `0xA00C = 0` — no expansion board **and** DIP switch port A
bit 6 clear. It is a byte-at-a-time state machine driven by the RST5.5 ISR.

### Framing

| Element | Source | Notes |
| --- | --- | --- |
| Start-of-frame | byte at `0xA006` | computed at `0x055A` as **`0x60 \| (port A & 0x1F)`** — the display address |
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

The `0x3027` subsystem belongs to **handler C**, and it emits a **5-byte frame**:

```
18 18 1E 15 7F      and                   18 18 1E 15 BF
```

The two variants differ only in bits 7:6 of the last byte; the low five bits are
the port A address, so `0x7F` is the `0x60 | 0x1F` form a unit reading all-ones
switches produces.

> **Not a beacon.** An earlier draft recorded this as continuously emitted and
> read it as a master polling slaves. Re-checked 2026-08-08 across both port C
> bit 5 settings: **the firmware transmits nothing unprompted**, and never even
> enters `isr_rst55` without received data. Every handler C transmit path hangs
> off the received-byte dispatch. See
> [the transmitted status frame](#the-transmitted-status-frame-c).

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
| `0x04` | EOT | `0x110D` | **block write**, or the literal-byte escape — sub-mode digit follows |
| `0x05` | ENQ | `0x1035` | |
| `0x06` | ACK | `0x1057` | |
| `0x07` | BEL | `0x10C8` | |
| `0x08` | BS | `0x10E1` | **status query** — replies `BS <status>`, and reading clears it |
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

### The message buffer **[C]**

Every path converges on the bounds check at `0x0C41`:

```
0x0C41  LXI  HL,0xE001     ; buffer base
0x0C44  LXI  DE,0x40       ; length, from ROM constant 0x007B
0x0C47  DAD  HL,DE         ; limit = 0xE041
        ...compare the write pointer at 0xA03B against the limit...
0x0C59  XCHG               ; clamp the pointer to the limit
0x0C5B  STA  0xA044 = 0
0x0C60  STA  0xA043 = 1    ; overflow flag
0x0C6B  MVI  A,0x30
0x0C6D  STA  0xA03A        ; error code '0' -> this message will be NAKed
```

* **The message buffer is `0xE001-0xE040`, exactly 64 bytes.** The length comes
  from the ROM constant at `0x007B`, not from a literal.
* A second buffer lives at `0xE084`.
* **Overflow is detected, not ignored**: the pointer is clamped and error code
  `0x30` is latched, so an over-long message is answered with NAK rather than
  silently truncated.

64 characters for a 16-character panel implies the extra 48 are for scrolling.

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

#### The full reply frame

```
[4 x SYN]  <ACK|NAK>  <address>  [errcode  [param1 param2]]
```

| Field | Source | Condition |
| --- | --- | --- |
| `SYN` x4 | `0x1AAA` | only if ROM byte `0x008A` bit 1 is set — **not in this build** |
| `ACK` `0x06` / `NAK` `0x15` | `0xA03A == 0` ? | always |
| address | DIP switch port A bits 4:0 | only if ROM byte `0x0088` bit 1 is set — **it is** |
| errcode | `0xA03A` | only when non-zero |
| `param1`, `param2` | `0xA050`, `0xA051` | only when errcode is neither `0x30` nor `0x32` |

**Error codes are printable ASCII.** `0x30` `'0'` is buffer overflow. `0x32`
`'2'` is some other self-describing failure; both are reported bare, while any
other code carries two extra bytes of detail. So a NAK is not just a rejection,
it says *why*.

Before replying, `0x1A7F` performs line turnaround: it re-issues the 8251A
command word to drive RTS (`0x37` asserted / `0x17` deasserted), then optionally
sends four `SYN` bytes or waits a fixed delay. Which of those happens is
controlled by ROM byte `0x008A` (`0x05` here: enabled, no SYN preamble, delay of
40 units). **This is why `0x16` SYN has no receive handler — it is a transmit-side
preamble only.**

After the reply, `0x0F8C` clears `0xA03A`, `0xA04F`, `0xA050`, `0xA051`,
`0xA052` and `0xA054`, resetting the error state for the next message.

### Addressing: the unit must be selected first **[C]**

This is the key to the whole protocol, and it is why early experiments got no
reply. Handler B opens with a guard at `0x0ACB`: while `0xA040` (the *selected*
flag) is zero, **only three commands are accepted** — `EOT`, `VT` and `GS`.
Everything else falls straight through to `RET`. The 28-entry dispatch table at
`0x0B76` is reachable only once the unit is selected.

`GS` (`0x1D`) does the selecting. Its parameter handler at `0x105E`:

```
0x105E  IN   0x80          ; the address byte
0x1060  ANI  0x7F
0x1065  CPI  0x7F          ; 0x7F -> broadcast, select unconditionally
0x1078  IN   0xA1          ; else build this unit's address from port A
        ...0x30 | bits3:0, or 0x40 | bits3:0 when bit 4 is set...
0x108D  CMP  B             ; compare with the received byte
0x108E  JZ   0x1098        ; match -> select
0x1091  STA  0xA03F = 0    ; no match -> stay deselected and ignore everything
0x1098  MVI  A,1
0x109A  STA  0xA040        ; selected
```

* **`0x7F` is the broadcast address** — every unit on the bus selects.
* A non-matching address leaves the unit silent, so several displays can share
  one line.
* `RS` (`0x1E`) clears `0xA040`, i.e. **deselects**.

So every transaction looks like:

```
GS <addr>  <commands...>   RS
```

### The repaint interlock: `RS` is not optional **[C]**

The trailing `RS` above was originally written as optional. **It is not, if you
want anything to appear on the panel.**

The repaint task reaches this at `0x130B`:

```
0x130B  LDA  0xA040        ; the selected flag
0x130E  CPI  0x00
0x1310  JNZ  0x130B        ; spin, for as long as this unit is selected
0x1313  MVI  A,0x01
0x1315  STA  0xA045
0x1318  JMP  0x124E        ; ...and only then render
```

**A selected unit never repaints.** Text sent between `GS` and `RS` is stored in
the buffer and acknowledged with `ACK`, and the dots do not move. Deselecting
releases the spin and the panel updates.

Measured in emulation, feeding `GS '5' STX A B ETX` and running 800 000
instructions:

| Sent | Spin hits at `0x130B` | Render passes | Characters drawn |
| --- | --- | --- | --- |
| without `RS` | 259 016 | **0** | **0** |
| with `RS` | 131 | 131 | 2 — exactly the two that changed |

This is almost certainly deliberate: it stops the panel from repainting halfway
through an incoming message, so a partially-written line is never displayed. The
buffer and its shadow ([io-map.md](io-map.md#what-is-in-the-window-c-from-emulation))
make the update atomic, and the selection defines the transaction boundary.

**Consequence for a host:** `ACK` means *stored*, not *displayed*. Do not treat
it as confirmation that the message is on the panel — send `RS` and only then is
the update committed. `tools/pit100.py`'s `show()` does this; a hand-rolled
controller must too.

### Decoded commands **[C]**

| Code | | Effect |
| --- | --- | --- |
| `0x02` STX | begin message | pointer := `0xE001`; clears `0xA043`/`0xA044`; sets `0xA042 = 1` |
| `0x03` ETX | end message | validate, reply `ACK`/`NAK` + address + error detail, reset state |
| `0x18` CAN | **clear the display** | fill 64 bytes from `0xE001` with `0x20` (space) — or from `0xE084` with `0x00` when `0xA057` is set — then post `0x4A` to the mailbox so the main loop repaints |
| `0x1E` RS | reset receive state | clears `0xA040`, `0xA00D`, `0xA06F`. **Ignored when the unit's address is 0**, so address 0 is effectively "unaddressed" |
| `0x10` DLE | **reset the unit** | jumps to `fatal_or_init` |
| `0x06` ACK | clears `0xA03D`, then performs the same clear as CAN |

### The buffer is a cursor-addressable 2-D grid **[C]**

The 64-byte buffer is addressed as **rows of 64 columns**, with the cursor kept
in two variables:

| Variable | Meaning |
| --- | --- |
| `0xA043` | row |
| `0xA044` | column (0-63) |
| `0xA03B` | the write pointer these two describe |

That makes the command set a small terminal:

| Code | | Effect |
| --- | --- | --- |
| `0x0B` VT | **position cursor** — takes *two* parameter bytes, `VT <row+0x30> <col+0x30>`, each an ASCII digit; recomputes the pointer as `0xE001 + row*64 + col` |
| `0x0D` CR | **carriage return** — back to column 0 of the current row |
| `0x0A` LF | **line feed** — advance to column 64, then row++, column := 0 |
| `0x11` DC1 | **cursor up** — pointer -= 64, row-- |
| `0x14` DC4 | **cursor down** — pointer += 64, row++ |
| `0x13` DC3 | **cursor left** — pointer--, column--; wraps to the previous row at column 63 |
| `0x12` DC2 | **cursor right** — pointer++, column++; wraps to the next row at column 64 |
| `0x17` ETB | **erase to end of line** — fill spaces from the cursor to column 64 |
| `0x19` EM | **erase to end of display** — as ETB, continuing across rows |
| `0x18` CAN | **clear** — fill the whole buffer with spaces |
| `0x0C` FF | **fill with `#`** (`0x23`) — a test pattern |

Cursor moves that run off the buffer jump to `0x0C66`, which latches error `0x30`
— the same overflow error a too-long message produces.

### Query commands: the unit reports back **[C]**

Five commands make the display *answer* with real data. Each one wraps its reply
in a ROM-defined prefix and suffix (pointers `0x00D2-0x00D5` and
`0x00DA-0x00DD`, mostly empty in this build except a four-`SYN` preamble for
`SUB`), with `0x1A7F` handling line turnaround first.

| Code | | Reply |
| --- | --- | --- |
| `0x1A` SUB | **identify** — 45 bytes starting at ROM `0x0064`: the version string followed by the whole configuration block |
| `0x09` TAB | **report cursor** — two ASCII digits, `row+0x30` then `col+0x30` |
| `0x0E` SO | **read character at cursor** — one byte from the buffer |
| `0x0F` SI | **read back what is displayed** — see below |
| `0x08` BS | **status** — `BS` then one status byte, see below |

#### `SI` reads the shadow, not the message buffer **[C]**

`0x0E45` computes the source as `write_ptr + 0x42`, which for a cursor at
`0xE001` is `0xE043` — **the shadow copy, i.e. what the panel is actually
showing**, not what was last written into the message buffer.

That makes `SI` the natural way to confirm an update committed. Demonstrated
end to end in emulation:

```
select, message("CHAMARTIN"), goto(0,0), SI
  -> '                    '     nothing displayed yet: still selected
deselect, select, goto(0,0), SI
  -> 'CHAMARTIN           '     the repaint has now happened
```

The same experiment confirms the repaint interlock, the shadow buffer and `SI`'s
source simultaneously.

One side effect: the firmware plants a `0xFF` sentinel at `0xE083` to bound the
reply, clobbering the shadow's last byte. Character 63 is therefore repainted on
the next pass. Harmless, but `SI` is not quite side-effect free.

#### `BS` returns one status byte, and reading clears it **[C]**

```
0x10E1  CALL 0x1A7F        ; line turnaround
0x10E4  MVI  A,0x08        ; echo BS itself
0x10E6  CALL uart_send
0x10E9  LDA  0x008B        ; a build-time selector
0x10EE  JNZ  0x1101
0x10F1  LDA  0xA024        ; ...this build: the latched status byte
0x10F4  CALL uart_send
0x10FA  SUB  A
0x10FB  STA  0xA024        ; read-and-CLEAR
0x1101  IN   0xDE          ; ...other builds: a hardware input, inverted
0x1103  CMA
```

ROM `0x008B` is `0x00` in this build, so the reply is `08 <0xA024>` and **reading
clears the byte**. Observed reply: `08 00`.

**`0xA024` is a sticky accumulator of I/O port `0xDE`.** At `0x19FC`:

```
0x19FE  IN   0xDE
0x1A00  CMA                ; the port is active low
0x1A01  MOV  B,A
0x1A02  LDA  0xA024
0x1A05  ORA  B             ; sticky: bits latch until read
0x1A06  STA  0xA024
```

and `0x19FC` is called from the **RST7.5 tick handler** at `0x02C0`, so the port is
sampled on every periodic interrupt. The two `BS` variants are therefore the same
hardware input read two ways:

| ROM `0x008B` | `BS` reports |
| --- | --- |
| `0x00` (this build) | `0xA024` — **latched**: any bit active since the last query, then cleared |
| non-zero | `IN 0xDE` complemented — the **instantaneous** state |

So a host gets edge-triggered semantics here: a fault that came and went between
two queries is still reported once, and polling repeatedly will read `0` even while
the condition persists. Treat each reply as "since you last asked".

`0xDE` is an unidentified 8-bit input, almost certainly hardware status —
temperature, door, lamp or supply monitoring would all fit a station sign. What
each bit means is open, and only the hardware can answer it. **[?]**
→ [io-map.md](io-map.md)

### `EOT` is a block-write command **[C]**

`EOT` (`0x04`) sets a state byte and the *next* byte is an ASCII digit selecting
one of three sub-modes. All three are confirmed in emulation.

| Sub-mode | Form | Effect |
| --- | --- | --- |
| `'1'` | `EOT '1' <64 bytes>` | write exactly 64 bytes to `0xE084` |
| `'2'` | `EOT '2' <ofs-hi> <ofs-lo> <cnt-hi> <cnt-lo> <data…>` | write `cnt` bytes at `0xE084 + ofs` |
| `'3'` | `EOT '3' <byte>` | **store one byte verbatim**, bypassing the dispatcher |
| other | | latches error `'0'` and behaves like `STX` |

Verified: `EOT '1'` with `00 01 … 3F` lands byte for byte at `0xE084`; `EOT '2'`
with offset 4 and count 6 writes exactly `0xE088-0xE08D` and leaves the pointer
at `0xE08E`.

Both offset and count are **16-bit**, so this is not bounded by the 64-byte
region — it can reach into paged expansion memory. Nothing in the render path is
known to read `0xE084`, so what it is *for* remains open; a per-character
attribute plane is the obvious guess. **[?]**

#### `EOT '3'` is the escape that makes control-code glyphs reachable **[C]**

This one matters for anyone driving the panel. The dispatcher consumes `0x00-0x1F`
as commands, so a bare control byte is **silently eaten** — it never reaches the
buffer and no error is raised:

```
STX 'A' 0x01 'B' ETX            -> buffer: 41 42        the 0x01 vanished
STX 'A' EOT '3' 0x01 'B' ETX    -> buffer: 41 01 42     stored
STX EOT '3' 0x00 'A' ETX        -> buffer: 00 41        even 0x00 works
```

CP437 puts real glyphs at `0x01-0x1F` — smileys, card suits, arrows, note
symbols ([font.md](font.md)) — and this escape is the only way to display them.
It works mid-message and needs no state setup beyond being selected.
`tools/pit100.py` applies it automatically in `message()`; `escape_controls()`
exposes it directly.

Verified in the simulator. Actual replies, with the repeating status frame
stripped:

```
GS '5' SUB          -> 16 16 16 16 41 4B 30 30 35 32 2E 31 33 39 55 31 00 00 01 ...
                       [4x SYN]    A  K  0  0  5  2  .  1  3  9  U  1  <config block>
GS 0x7F SUB         -> identical (broadcast)
GS '5' TAB          -> 30 30                       row '0', column '0'
GS '5' STX HOLA ETX -> 06 35                       ACK, address '5'
GS '5' ... SI       -> 06 35 23 23 23 ...          ACK, address, then buffer contents
GS '9' SUB          -> (nothing - wrong address)
```

**`SUB` is the single most useful command for bring-up.** It needs no prior
state, works under broadcast addressing, and returns a known-answer string, so
it doubles as a link test *and* dumps the unit's configuration block — which
[the config section](#build-time-configuration-block-rom-0x0070-0x008b-c) warns
may differ from this one.

### Commands that take a parameter byte **[C]**

These set a state variable; the *next* byte received is consumed as the
parameter rather than dispatched as a command. That is what the state variables
at the top of handler B are for.

| Command | State var | Parameter handler | Effect |
| --- | --- | --- | --- |
| `0x07` BEL | `0xA055` | `0x10D0` | **low 3 bits of the parameter become the display attribute `0xA014`** — 8 display modes |
| `0x05` ENQ | `0xA03E` | `0x103D` | parameter stored at `0xA041`; a `0x12` DC2 parameter is translated to `0x23` `'#'`; sets `0xA03D = 1` |
| `0x1D` GS | `0xA03F` | `0x105E` | parameter is an **address**; `0x7F` broadcasts. Selects the unit — see [Addressing](#addressing-the-unit-must-be-selected-first-c). This is why `0x1D` never reaches the dispatch table |
| `0x04` EOT | `0xA05D` | `0x1115` | ASCII digit selecting a sub-mode: `'1'` and `'2'` block-write to `0xE084`, `'3'` stores the next byte literally |
| `0x0B` VT | `0xA039` | `0x0CB7`, `0x0CE9` | takes **two** parameters, row then column |
| `0x1B` ESC | `0xA04C` | — | collects bytes into a buffer at `0xA046`. **Disabled in this build**: `0x0E8C` requires ROM byte `0x0077` to be 1 and it is `0x00` |

`BEL` + one byte is the display-effect selector for handler B, the counterpart of
the in-band `0x7C`/`0x7D`/`0x7F` characters in handler A.

### Build-time configuration block, ROM `0x0070-0x008B` **[C]**

Handler B reads its behaviour out of ROM constants rather than hard-coding it —
the same firmware was evidently customised per installation by patching these
bytes.

| Address | Value | Meaning |
| --- | --- | --- |
| `0x0072` | `0x01` | protocol enable; zero short-circuits the ETX reply |
| `0x007A` | `0xFF` | address mode: `0xFF` = take the address from DIP switch port A, `0x00` = unaddressed |
| `0x007B` | `0x40` | **message buffer length, 64 bytes** |
| `0x007E` | `0x40` | a second length, also 64 |
| `0x0088` | `0x02` | feature bits; bit 1 = append the address to the reply |
| `0x008A` | `0x05` | turnaround options: bit 0 enable, bit 1 four-SYN preamble, bit 2 delay, bit 3 RTS polarity |

Anyone building a controller should read these out of *their* ROM rather than
trusting this table — a different unit may well be configured differently.

### Options frozen into the code, not just into the block **[C]**

The configuration block is not the only place customisation lives. The ROM also
contains **three constant tests** — a value loaded and immediately compared with
itself — which are what a compiled-out option looks like once it is in the image:

| Site | Test | Effect |
| --- | --- | --- |
| `0x07E9` | `MVI A,0x01 / CPI 0x01 / JZ` | skips setting `0xA057` → **the second display is disabled** |
| `0x1A0D` | `MVI A,0x00 / CPI 0x00 / JZ` | selects the turnaround-capable transmit path, orphaning the plain one at `0x1A46` |
| `0x088A` | `MVI A,0x00 / CPI 0x00 / JZ` | skips a reply that would send the ROM string at `0x00DE`, which is empty here anyway |

Found by scanning the whole image for the `MVI A,k / CPI k` pattern followed by a
conditional; there are exactly three, all always-taken. Only the first changes
anything substantial — see
[io-map.md](io-map.md#the-second-buffer-pair-is-a-disabled-feature-c).

**Why this matters beyond curiosity:** it means behaviour differences between
units are not all discoverable from the configuration block. Another installation's
ROM could have any of these three constants flipped, enabling a whole second
display's worth of function. Compare images before assuming two units behave alike.

## Reference implementation

[../tools/pit100.py](../tools/pit100.py) implements everything below and is
exercised against the emulator, so the protocol described here is executable
rather than merely documented:

```sh
PYTHONPATH=tools python3 tools/pit100.py --sim --address 5 --identify --text HOLA
```

## Timing **[C]**

With the clock chain known (6.144 MHz crystal → 3.072 MHz, 325.5 ns per cycle)
the firmware's delay loops convert to real numbers. Measured by executing them in
the simulator and counting cycles:

| Quantity | Cycles | Time |
| --- | --- | --- |
| One delay unit, `FUN_1B4F` | 277 | **90 µs** |
| Line turnaround before a reply, `0x1ABD` (40 units) | 12017 | **3.9 ms** |
| Short turnaround variant, `0x1AD0` (2 units) | ~600 | ~0.2 ms |

**Inter-character delay is zero in this build.** The transmit routine at `0x1A1A`
has two configurable gap loops, both loaded with `MVI C,0x00`, so they are
skipped entirely and characters go out back to back. Like the configuration block
these look meant to be patched per installation.

Frame times, for a host-side timeout budget:

| Baud | Bit time | 5-byte status frame (11 bits/char) |
| --- | --- | --- |
| 9600 | 104 µs | 5.7 ms |
| 4800 | 208 µs | 11.5 ms |
| 2400 | 417 µs | 22.9 ms |
| 1200 | 833 µs | 45.8 ms |
| 600 | 1.67 ms | 91.7 ms |
| 300 | 3.33 ms | 183 ms |

So after sending a command, **a host should not expect the first reply byte for
at least ~4 ms**, and should allow the frame time above on top. At 300 baud a
reply to `SUB` — 45 payload bytes plus preamble — takes about 1.8 seconds.

## The transmitted status frame **[C]**

The five-byte frame

```
18 18 1E 15 <addr>          CAN CAN RS NAK <addr>
```

is **real and in the code** — `0x2D45` sends `18`, falls into the sequence at
`0x2D52` that sends `18 1E 15` and then `0xA07D`. Two variants were recorded,
`0x60 | address` and `0xA0 | address`, differing only in bits 7:6 of the last
byte, so those are flags.

> **Retracted 2026-08-08: it is not emitted continuously, and not
> "independently of either handler".** It belongs to **handler C**, and handler C
> is entirely reactive — every one of its transmit paths runs from the
> received-byte dispatch and is preceded by `line_turnaround`.
>
> Re-checked in the simulator across `port A` = `0x45`/`0xFF` and both settings of
> port C bit 5, two million instructions each: **zero bytes transmitted, and
> `isr_rst55` never entered at all** without fed input. The original note recorded
> this frame as something the firmware "transmits continuously"; nothing in the
> current model reproduces that, and the code says a poll must arrive first.
>
> This matters beyond bookkeeping: [TODO.md](../TODO.md) offered the frame as a
> **known-answer test** for baud, polarity and framing on a powered unit. It is
> still a known answer — but the unit has to be *asked*, and the transitions seen
> on a scope with nothing driving the link remain unexplained.

Either way this is a slave answering on a shared bus, not a master polling. Worth
weighing against the MAESTRA label: this unit may be the master *of its display
chain* while still being a slave *on the serial bus*.

## Handler C — a third protocol **[C]**

Selected when **DIP switch port C bit 5 is clear**, at a hardwired **9600 8E1**.
930 bytes at `0x2C80`, and *not* a variant of the command set above: it is binary,
polled, and shares nothing with handlers A and B but the UART.

What is established:

* **Entirely reactive.** Every transmit path runs from the received-byte
  dispatch at `0x2D06`, and each is preceded by `line_turnaround` (RTS) and
  followed by `reply_cleanup`. Nothing is sent unprompted.
* **It reads the UART properly** — `IN 0x81 / ANI 0x78` separates a clean byte
  from a parity/overrun/framing error, and latches the error into
  `status_flags_a`.
* **Command bytes** dispatch on `uart_rx_byte`: `0x00`, `0x01`, `0x02` (STX),
  `0x03` (ETX), `0x0C`, `0x0F`, `0x18` (CAN), `0x1D` (GS), `0x1E` (RS). `0x0C`
  jumps straight to `fatal_or_init` — a **reset command**.
* **Replies** are short binary frames: `0x0F <addr>` to command `0x01`,
  `0x1D <byte>`, `0x02 <addr> …`, and the `18 18 1E 15 <addr>` status frame.
* **`<addr>` is `0xA07D`**, whose low 5 bits come from the port A switches — the
  same address the other handlers use.
* **Message delivery is length-prefixed.** At `0x2F14` the handler takes the
  received byte as a **count**, stores it in `rx_c_stage_len`, and stages the
  following bytes at `0xA039`. `main_loop` (`0x03C7`) then replays exactly that
  many bytes through `rx_a_store_byte` into the message buffer, using `0x61` as
  the start delimiter and `0x03` as the end delimiter.

  That closes an old loose end: the "0x61-iteration render loop" recorded in
  earlier drafts was a misreading. `0x61` is a **delimiter constant** passed in
  `B`, not an iteration count; the count is `rx_c_stage_len`.

Not established: the frame layout, the checksum (if any), and which command
begins a transaction. Nobody has driven handler C in the simulator successfully —
single bytes produce silence, so it wants a sequence.

**Why it matters.** An installed unit may well have been running this, not the
ASCII protocol. Read port C bit 5 before assuming anything.

## Still unknown

* The sticker reads **MAESTRA** ("master"), and DIP switch port A is largely
  unattributed. A master/slave addressing scheme on the serial bus is therefore
  likely, meaning frames probably carry a **display address** taken from those
  switches. **[L]**
* **Handler C's frame layout** — see above. This is now the largest undecoded
  region of the ROM and needs no hardware.
* **What produced the transitions seen on the DB37 pins** with nothing driving
  the link. The status frame was the standing explanation and it no longer fits.

## Protocol scratchpad

Record every hypothesis and every experiment result here, including the ones
that failed — a negative result is data.

| Date | Frame sent (hex) | Expected | Observed | Verdict |
| --- | --- | --- | --- | --- |
| | | | | |

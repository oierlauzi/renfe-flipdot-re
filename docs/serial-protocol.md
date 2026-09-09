# Serial protocol

How to send bytes into the DB37 connector and get arbitrary text onto the
16-character panel, with no hardware modification. Everything here has been
exercised against the unit itself.

## Physical layer

| Property | Value |
| --- | --- |
| Connector | DB37 female, 4 conductors populated |
| Signalling | RS-422 differential, 4-wire full duplex, opto-isolated |
| Transceivers | 2 x TI SN75176BP on daughter 1, one per pair |
| Isolation | 3 x HP 6N139 on daughter 1 — TX data, RX data, driver enable |
| Receive pair | **green = `A`, yellow = `B`** — the pair the display listens on |
| Transmit pair | **blue = `A`, red = `B`** — the pair the display replies on |
| Shield | grey overall, chassis / drain |
| DB37 pin numbers | not traced |

Each cable pair lands on the `A`/`B` pins of a different SN75176BP. On the
transceiver carrying green/yellow, `/RE`, `DE` and `D` are strapped to ground —
receiver permanently enabled, driver permanently tri-stated — so that package can
only receive.

A single SN75176B is a 2-wire half-duplex device, its driver and receiver sharing
one pair, so two packages used half each is the 4-wire idiom. Two independent
half-duplex channels are excluded by there being one USART in the unit and only
three optocouplers crossing the isolation barrier: enough for one TX, one RX and
one control line.

The firmware asserts RTS before every reply, which would be redundant on a
point-to-point full-duplex link. On a 4-wire multidrop bus the slaves share the
return pair and each must tri-state its driver until addressed, which is what the
third optocoupler and the `DE` line are for. Whether `DE` on the transmit package
is actually driven from RTS or tied high has not been traced.

A host therefore needs a **4-wire RS-422 adapter**, not a 2-wire RS-485 dongle.
Wiring table in [controller.md](controller.md#wiring).

## Link layer

The 8251A is initialised twice over, and DIP switch port C bit 5 chooses which
path runs:

* **Bit 5 clear** — a hardwired configuration at `0x0443`: mode word `0x7E`,
  timer preload `0x4014`, giving **9600 8E1**. This also selects
  [handler C](#handler-c).
* **Bit 5 set** — the mode word is built from port B at `0x0533` and the timer
  preload chosen at `0x0474`, so baud rate, data bits, parity and stop bits all
  come from the switches. See [dip-switches.md](dip-switches.md).

**This unit runs at 1200 baud, 7 data bits, even parity, 1 stop.** A different
installation may be set differently; there is no way to know without reading the
switches or sweeping the line settings, and `tools/bringup.py --stage line` does
the latter automatically.

A 7-bit link strips bit 7 in hardware. That is visible in replies — the
configuration byte at ROM `0x007A` is `0xFF` but reads back as `0x7F` — and it is
why the ROM carries duplicate `ñ` and `Ñ` glyphs at `0x24`/`0x25`, inside the
7-bit range. See [font.md](font.md).

## Receive architecture

RST5.5 is the serial receive interrupt. The handler at `0x0260`:

1. `MVI A,0x0B / SIM` masks RST5.5 and RST6.5 but leaves RST7.5 unmasked, then
   `EI`, so the timer tick can preempt the receive path.
2. `IN 0xA3 / ANI 0x20` tests DIP switch port C bit 5. Clear routes the whole
   interrupt to `rx_handler_c` at `0x2C80` and the steps below never run.
3. Writes token `0x4E` to the mailbox byte `0xA012`.
4. Sets `0xA05A = 1` for the duration, so the banked memory accessors save and
   restore their page (see [io-map.md](io-map.md#0xe7--expansion-memory-bank-register)).
5. Dispatches on `0xA00C` to handler A (`0x08B7`) or handler B (`0x0ACB`).
   `0xA00C` is set at `0x0689` from either the expansion board or port A bit 6.

There are three receive handlers, chosen by two independent switches:

| port C bit 5 | `0xA00C` | Handler | Protocol |
| --- | --- | --- | --- |
| set | `!= 1` | **A** (`0x08B7`) | delimited byte stream into the buffer |
| set | `== 1` | **B** (`0x0ACB`) | the ASCII command set, most of this document |
| clear | — | **C** (`0x2C80`) | a binary polled protocol |

RST7.5 (`0x02A9`) is a periodic tick, not part of the receive path. It decrements
the watchdog at `0xA01E`, samples the status input `0xDE`, debounces the clock
board's keypad, and drives the 8085's SOD pin as a GPIO output — possibly a
heartbeat or an external watchdog strobe.

### The mailbox at 0xA012

`0x03A0` in the main loop performs an atomic swap on `0xA012` under `DI`/`EI`:
write `0x4E`, and branch on whether the old value was `0x4A`. The ISR writes
`0x4E`, so `0x4A` and `0x4E` are ISR/main-loop handshake tokens rather than
framing bytes.

## Handler A — delimited text

Handler A runs when `0xA00C = 0`, meaning no expansion board and port A bit 6
clear. It is a byte-at-a-time state machine driven by the RST5.5 ISR.

| Element | Source | Notes |
| --- | --- | --- |
| Start-of-frame | byte at `0xA006` | computed at `0x055A` as `0x60 \| (port A & 0x1F)` — the display address |
| Alternate start | `0xA006 + 1` | selects a second buffer at `0xE03C` |
| End-of-frame | byte at `0xA007` | configurable; on match, posts `0x4A` to the mailbox |
| Buffer | `0xE000`, or `0xE03C` | write pointer kept in `0xA00E` |

On the start byte the handler resets the write pointer to `0xE000` and stores the
byte. On the end byte it posts `0x4A` to the mailbox, which is what the main loop
at `0x03A0` waits for.

Per-character filtering, in the order the handler tests it:

| Condition | Action |
| --- | --- |
| `== (0xA006)` | start of frame — reset buffer to `0xE000` |
| `== 0x7D` `}` | set duration `0xA016` to 80 or 2000 per port C bit 5, `0xA014 \|= 0x02` |
| `== 0x7C` `\|` | `0xA014 = (0xA014 & 0x7F) \| 0x05` |
| `== 0x7F` DEL | `0xA014 = (0xA014 & 0xFA) \| 0x80` |
| `== (0xA007)` | end of frame — post `0x4A` |
| bit 7 set | not stored; write pointer still advances |
| `0x00-0x1F` | ignored entirely |
| `0x60-0x7F` | state change only, not stored |
| `== (0xA00B)` | stored as `0x20`, a blank substitute character |
| otherwise | stored, pointer advances |

In an extended sub-mode (`0xA008 = 1`, entered at `0x097B`) further codes are
recognised: `0x11`, `0x12`, and `0x64`/`0x65`/`0x66`, which set the duration words
at `0xA016`/`0xA018`. `0xA014` is the display attribute byte, so these in-band
control characters select display effects and their timings.

### Error handling

The handler reads the 8251A status and masks `0x78` — PE, OE, FE, SYNDET:

| Status & `0x78` | Action |
| --- | --- |
| `0x00` | normal: read and process the byte |
| `0x08`, parity error only | read and discard the byte, re-issue command `0x17`, and **substitute `0x23` `#`** into the message |
| anything else | full 8251A re-init: reset `0x40`, restore mode from `0xA005`, command `0x17`, post `0x4E` or `0x4A` |

A parity error therefore puts a `#` on the display, which is a distinctive and
externally visible symptom of a mis-set link.

## Handler B — the command set

Handler B runs when `0xA00C = 1`, meaning an expansion board is fitted or port A
bit 6 is set. It is a command-oriented protocol driven by single-byte ASCII
control codes, with several state variables (`0xA03F`, `0xA040`, `0xA053`,
`0xA055`, `0xA05D`, `0xA039`, `0xA03E`, `0xA04C`) that divert incoming bytes to
different continuations, so a command can consume following bytes as parameters.

### Addressing: the unit must be selected first

Handler B opens with a guard at `0x0ACB`: while `0xA040`, the selected flag, is
zero, **only `EOT`, `VT` and `GS` are accepted**. Everything else falls through to
`RET`, and the dispatch table at `0x0B76` is unreachable.

`GS` (`0x1D`) does the selecting. Its parameter handler at `0x105E`:

```
0x105E  IN   0x80          ; the address byte
0x1060  ANI  0x7F
0x1065  CPI  0x7F          ; 0x7F -> broadcast, select unconditionally
0x1078  IN   0xA1          ; else build this unit's address from port A
        ...0x30 | bits3:0, or 0x40 | bits3:0 when bit 4 is set...
0x108D  CMP  B             ; compare with the received byte
0x108E  JZ   0x1098        ; match -> select
0x1091  STA  0xA03F = 0    ; no match -> stay deselected
0x1098  MVI  A,1
0x109A  STA  0xA040        ; selected
```

* `0x7F` is the broadcast address; every unit on the bus selects.
* A non-matching address leaves the unit as it was, so several displays can share
  one line.
* `RS` (`0x1E`) clears `0xA040` and deselects.

Every transaction therefore looks like:

```
GS <addr>  <commands...>   RS
```

Start with `RS`. A unit left mid-command by an earlier session is waiting for a
parameter byte and will consume the next byte as one, which puts the text that
follows at a stale write pointer and produces a spurious overflow `NAK`.

### The repaint interlock: RS is not optional

The repaint task reaches this at `0x130B`:

```
0x130B  LDA  0xA040        ; the selected flag
0x130E  CPI  0x00
0x1310  JNZ  0x130B        ; spin, for as long as this unit is selected
0x1313  MVI  A,0x01
0x1315  STA  0xA045
0x1318  JMP  0x124E        ; ...and only then render
```

**A selected unit never repaints.** Text sent between `GS` and `RS` is stored and
acknowledged, and the dots do not move. Deselecting releases the spin.

Measured in emulation, feeding `GS '5' STX A B ETX` over 800 000 instructions:

| Sent | Spin hits at `0x130B` | Render passes | Characters drawn |
| --- | --- | --- | --- |
| without `RS` | 259 016 | 0 | 0 |
| with `RS` | 131 | 131 | 2 — exactly the two that changed |

This stops the panel repainting halfway through an incoming message: the buffer
and its shadow make the update atomic, and the selection defines the transaction
boundary.

For a host, `ACK` means *stored*, not *displayed*. Send `RS` to commit.

### Command dispatch, from 0x0B76

| Code | Name | Target | Notes |
| --- | --- | --- | --- |
| `0x00` | NUL | `0x0C70` | ignored |
| `0x02` | STX | `0x0F17` | start of text; sets `0xA042 = 1`, buffer to `0xE001` |
| `0x03` | ETX | `0x0F29` | end of text; validates and replies `ACK`/`NAK` |
| `0x04` | EOT | `0x110D` | block write, or the literal-byte escape |
| `0x05` | ENQ | `0x1035` | takes a parameter byte |
| `0x06` | ACK | `0x1057` | clears `0xA03D`, then clears the display |
| `0x07` | BEL | `0x10C8` | takes a parameter byte: display attribute |
| `0x08` | BS | `0x10E1` | status query |
| `0x09` | TAB | `0x0DF5` | reports the cursor |
| `0x0A` | LF | `0x0D31` | line feed |
| `0x0B` | VT | `0x0CA7` | position cursor; takes two parameter bytes |
| `0x0C` | FF | `0x0FE9` | fill the buffer with `0x23` |
| `0x0D` | CR | `0x0D1B` | carriage return |
| `0x0E` | SO | `0x0E1A` | read the character at the cursor |
| `0x0F` | SI | `0x0E3C` | read back what is displayed |
| `0x10` | DLE | `0x034B` | **resets the unit** |
| `0x11` | DC1 | `0x0D54` | cursor up |
| `0x12` | DC2 | `0x0C1B` | cursor right |
| `0x13` | DC3 | `0x0D6D` | cursor left |
| `0x14` | DC4 | `0x0D94` | cursor down |
| `0x17` | ETB | `0x0DAA` | erase to end of line |
| `0x18` | CAN | `0x101E` | clear the display |
| `0x19` | EM | `0x0DC9` | erase to end of display |
| `0x1A` | SUB | `0x0E69` | identify |
| `0x1B` | ESC | `0x0E8C` | parameter sequence; disabled in this build |
| `0x1C` | FS | `0x0C70` | ignored |
| `0x1E` | RS | `0x10A3` | deselect |
| `0x1F` | US | `0x0C70` | ignored |

Bytes outside this table with high nibble `0x0` or `0x1` are ignored (`0x0C03`);
everything else is message text. `0x01` SOH, `0x15` NAK, `0x16` SYN and `0x1D` GS
have no handler — SOH, NAK and SYN are transmit-side only, and GS is consumed as
a parameter-taking command before the table is reached.

### The message buffer

Every path converges on the bounds check at `0x0C41`:

```
0x0C41  LXI  HL,0xE001     ; buffer base
0x0C44  LXI  DE,0x40       ; length
0x0C47  DAD  HL,DE         ; limit = 0xE041
        ...compare the write pointer at 0xA03B against the limit...
0x0C59  XCHG               ; clamp the pointer to the limit
0x0C5B  STA  0xA044 = 0
0x0C60  STA  0xA043 = 1    ; overflow flag
0x0C6B  MVI  A,0x30
0x0C6D  STA  0xA03A        ; error code '0' -> this message will be NAKed
```

The message buffer is `0xE001-0xE040`, exactly **64 bytes**, and a second buffer
lives at `0xE084`. Overflow is detected rather than ignored: the pointer is
clamped and error code `0x30` latched, so an over-long message is answered with
`NAK`. Messages of 1 to 64 characters have all been confirmed to `ACK` on the
unit; 65 is refused.

Sixty-four characters for a 16-character panel is the same 64 as the panel's
address space — see
[flipdot-interface.md](flipdot-interface.md#what-the-rest-of-the-character-bus-reaches).

### The buffer is a cursor-addressable grid

The buffer is addressed as rows of 64 columns, with the cursor in two variables:
`0xA043` row, `0xA044` column, and `0xA03B` the write pointer they describe.

| Code | | Effect |
| --- | --- | --- |
| `0x0B` VT | position cursor: `VT <row+0x30> <col+0x30>`; recomputes the pointer as `0xE001 + row*64 + col` |
| `0x0D` CR | carriage return to column 0 of the current row |
| `0x0A` LF | line feed: advance to column 64, then row++, column := 0 |
| `0x11` DC1 | cursor up: pointer -= 64, row-- |
| `0x14` DC4 | cursor down: pointer += 64, row++ |
| `0x13` DC3 | cursor left; wraps to the previous row at column 63 |
| `0x12` DC2 | cursor right; wraps to the next row at column 64 |
| `0x17` ETB | erase from the cursor to column 64 |
| `0x19` EM | erase from the cursor to the end of the display |
| `0x18` CAN | fill the whole buffer with spaces |
| `0x0C` FF | fill the whole buffer with `0x23`, the solid block |

Cursor moves that run off the buffer jump to `0x0C66`, which latches error `0x30`
— the same overflow error a too-long message produces. `check_buffer_bounds`
clamps at `0xE041`, so in this build only row 0 holds text and the multi-row
arithmetic is generic code for wider members of the family.

### ETX replies ACK or NAK

On `ETX`, at `0x0F48`:

```
0x0F48  LDA  0xA03A        ; error flag accumulated over the message
0x0F4B  CPI  0x00
0x0F4D  MVI  C,0x06        ; ACK
0x0F4F  JZ   0x0F54
0x0F52  MVI  C,0x15        ; NAK
0x0F54  MOV  A,C
0x0F55  CALL 0x1A0C        ; transmit it
```

The full reply frame:

```
<ACK|NAK>  <address>  [errcode  [param1 param2]]
```

| Field | Source | Condition |
| --- | --- | --- |
| `ACK` `0x06` / `NAK` `0x15` | `0xA03A == 0` ? | always |
| address | port A bits 4:0, ASCII-encoded | only if ROM byte `0x0088` bit 1 is set, and it is |
| errcode | `0xA03A` | only when non-zero |
| `param1`, `param2` | `0xA050`, `0xA051` | only when errcode is neither `0x30` nor `0x32` |

Error codes are printable ASCII. `0x30` is buffer overflow and `0x32` some other
self-describing failure; both are reported bare, while any other code carries two
extra bytes of detail. An observed overflow reply is `15 31 30` — `NAK`,
address `'1'`, error `'0'`.

`0xA03A` is cleared after every reply, `ACK` or `NAK` alike, at `0x0F8C`, along
with `0xA04F`-`0xA052` and `0xA054`. An error therefore never persists into the
next message.

Before replying, `0x1A7F` performs line turnaround: it re-issues the 8251A
command word to drive RTS — `0x37` asserted, `0x17` deasserted — then optionally
sends four `SYN` bytes or waits a fixed delay, under ROM byte `0x008A`. In this
build that byte is `0x05`: turnaround enabled, no `SYN` preamble, delay of 40
units.

### Query commands

Five commands make the display answer with data. Each wraps its reply in a
ROM-defined prefix and suffix from the tables at `0x00D2` and `0x00DA`, mostly
empty in this build except a **four-`SYN` preamble for `SUB`**.

| Code | | Reply |
| --- | --- | --- |
| `0x1A` SUB | identify: 45 bytes from ROM `0x0064`, the version string then the whole configuration block |
| `0x09` TAB | cursor position |
| `0x0E` SO | the character at the cursor, one byte |
| `0x0F` SI | what is currently displayed |
| `0x08` BS | one status byte |

**Reply lengths are not fixed.** The `SUB` prefix, the conditional address byte
and the conditional error detail all vary, so a reader that counts bytes will
leave the remainder buffered and fall a reply behind. Drain until the line goes
quiet instead; the unit sends each reply as one burst.

`SUB` is the best bring-up command: it needs no prior state, works under
broadcast addressing, and returns a known-answer string. The observed reply is 49
bytes — four `SYN` then 45 — beginning `16 16 16 16 41 4B 30 30 35 32 2E …`,
which is `AK0052.` and onward.

#### TAB reports the cursor as offset bytes

The reply is `0x30 + row`, `0x30 + column`. That looks like two ASCII digits only
below 10: walking the cursor along the buffer gives `':'` at column 10, `'@'` at
16, `'P'` at 32 and `'o'` at 63, and column 64 wraps to row 1 column 0.

#### SI reads the shadow, not the message buffer

`0x0E45` computes the source as `write_ptr + 0x42`, which for a cursor at `0xE001`
is `0xE043` — the shadow copy, i.e. what the panel is actually showing, rather
than what was last written into the message buffer. That makes `SI` the way to
confirm an update committed:

```
select, message("CHAMARTIN"), goto(0,0), SI
  -> '                    '     nothing displayed yet: still selected
deselect, select, goto(0,0), SI
  -> 'CHAMARTIN           '     the repaint has happened
```

`SI` reads **from the cursor onward**, so home the cursor first or the reply
starts partway along the line.

One side effect: the firmware plants a `0xFF` sentinel at `0xE083` to bound the
reply, clobbering the shadow's last byte, so character 63 is repainted on the next
pass.

#### BS status byte

```
0x10E1  CALL 0x1A7F        ; line turnaround
0x10E4  MVI  A,0x08        ; echo BS itself
0x10E6  CALL uart_send
0x10E9  LDA  0x008B        ; a build-time selector
0x10EE  JNZ  0x1101
0x10F1  LDA  0xA024        ; ...this build: the latched status byte
0x10F4  CALL uart_send
0x10FA  SUB  A
0x10FB  STA  0xA024        ; read-and-clear
0x1101  IN   0xDE          ; ...other builds: the hardware input, inverted
0x1103  CMA
```

ROM `0x008B` is `0x00` here, so the reply is `08 <0xA024>` and reading clears the
byte. The observed reply is `08 00`.

`0xA024` is a sticky accumulator of I/O port `0xDE`, sampled and OR-ed in on every
RST7.5 tick at `0x19FC`, with the port complemented because it is active low. The
two variants are the same input read two ways:

| ROM `0x008B` | `BS` reports |
| --- | --- |
| `0x00`, this build | `0xA024`: any bit active since the last query, then cleared |
| non-zero | `IN 0xDE` complemented: the instantaneous state |

So a host gets edge-triggered semantics: a fault that came and went between two
queries is still reported once, and polling repeatedly reads `0` even while the
condition persists. Each reply means "since you last asked". What the individual
bits of `0xDE` mean is not known.

### EOT is a block write

`EOT` (`0x04`) sets a state byte, and the next byte is an ASCII digit selecting a
sub-mode:

| Sub-mode | Form | Effect |
| --- | --- | --- |
| `'1'` | `EOT '1' <64 bytes>` | write exactly 64 bytes to `0xE084` |
| `'2'` | `EOT '2' <ofs-hi> <ofs-lo> <cnt-hi> <cnt-lo> <data…>` | write `cnt` bytes at `0xE084 + ofs` |
| `'3'` | `EOT '3' <byte>` | store one byte verbatim, bypassing the dispatcher |
| other | | latches error `'0'` and behaves like `STX` |

`EOT '1'` with `00 01 … 3F` lands byte for byte at `0xE084`; `EOT '2'` with offset
4 and count 6 writes exactly `0xE088-0xE08D`. Both offset and count are 16-bit, so
this is not bounded by the 64-byte region and can reach into paged expansion
memory. Nothing in the render path reads `0xE084` in this build, so writes there
are invisible.

#### EOT '3' reaches the control-code glyphs

The dispatcher consumes `0x00-0x1F` as commands, so a bare control byte is
silently eaten — it never reaches the buffer and no error is raised:

```
STX 'A' 0x01 'B' ETX            -> buffer: 41 42        the 0x01 vanished
STX 'A' EOT '3' 0x01 'B' ETX    -> buffer: 41 01 42     stored
STX EOT '3' 0x00 'A' ETX        -> buffer: 00 41        even 0x00 works
```

CP437 puts real glyphs at `0x01-0x1F` — smileys, card suits, arrows, note symbols
([font.md](font.md)) — and this escape is the only way to display them. It works
mid-message and needs no state beyond being selected.
[../tools/pit100.py](../tools/pit100.py) applies it automatically in `message()`.

### Commands that take a parameter byte

These set a state variable, and the next byte received is consumed as the
parameter rather than dispatched as a command.

| Command | State var | Parameter handler | Effect |
| --- | --- | --- | --- |
| `0x07` BEL | `0xA055` | `0x10D0` | low 3 bits become the display attribute `0xA014` |
| `0x05` ENQ | `0xA03E` | `0x103D` | parameter stored at `0xA041`; a `0x12` DC2 parameter becomes `0x23`; sets `0xA03D = 1` |
| `0x1D` GS | `0xA03F` | `0x105E` | an address; `0x7F` broadcasts |
| `0x04` EOT | `0xA05D` | `0x1115` | ASCII digit selecting a block-write sub-mode |
| `0x0B` VT | `0xA039` | `0x0CB7`, `0x0CE9` | two parameters, row then column |
| `0x1B` ESC | `0xA04C` | — | collects bytes at `0xA046`; disabled here, since `0x0E8C` requires ROM byte `0x0077` to be 1 and it is `0x00` |

`BEL` plus one byte is handler B's display-effect selector, the counterpart of
handler A's in-band `0x7C`/`0x7D`/`0x7F` characters.

## Build-time configuration

Handler B reads its behaviour out of ROM constants rather than hard-coding it, so
the same firmware could be customised per installation by patching these bytes.

| Address | Value | Meaning |
| --- | --- | --- |
| `0x0072` | `0x01` | protocol enable; zero short-circuits the ETX reply |
| `0x007A` | `0xFF` | address mode: `0xFF` takes the address from port A, `0x00` is unaddressed |
| `0x007B` | `0x40` | message buffer length, 64 bytes |
| `0x007E` | `0x40` | a second length, also 64 |
| `0x0088` | `0x02` | feature bits; bit 1 appends the address to the reply |
| `0x008A` | `0x05` | turnaround: bit 0 enable, bit 1 four-`SYN` preamble, bit 2 delay, bit 3 RTS polarity |

Anyone building a controller should read these out of their own ROM; a different
unit may be configured differently. `SUB` returns the whole block.

Customisation also lives in the code itself, as three constant tests — a value
loaded and immediately compared with itself, which is what a compiled-out option
looks like in the image:

| Site | Test | Effect |
| --- | --- | --- |
| `0x07E9` | `MVI A,0x01 / CPI 0x01 / JZ` | skips setting `0xA057`, disabling the second display |
| `0x1A0D` | `MVI A,0x00 / CPI 0x00 / JZ` | selects the turnaround-capable transmit path, orphaning `0x1A46` |
| `0x088A` | `MVI A,0x00 / CPI 0x00 / JZ` | skips a reply that would send the empty ROM string at `0x00DE` |

Scanning the whole image for `MVI A,k / CPI k` followed by a conditional finds
exactly these three, all always taken. Only the first changes anything
substantial. So behaviour differences between units are not all discoverable from
the configuration block: another installation's ROM could have any of these
constants flipped.

## Timing

With the clock chain known — 6.144 MHz crystal, 3.072 MHz CPU, 325.5 ns per cycle
— the firmware's delay loops convert to real numbers:

| Quantity | Cycles | Time |
| --- | --- | --- |
| One delay unit, `0x1B4F` | 277 | 90 µs |
| Line turnaround before a reply, `0x1ABD`, 40 units | 12 017 | **3.9 ms** |
| Short turnaround variant, `0x1AD0`, 2 units | ~600 | ~0.2 ms |

Inter-character delay is zero in this build: the transmit routine at `0x1A1A` has
two configurable gap loops, both loaded with `MVI C,0x00`, so characters go out
back to back.

Frame times, for a host timeout budget:

| Baud | Bit time | 5-byte frame, 11 bits per character |
| --- | --- | --- |
| 9600 | 104 µs | 5.7 ms |
| 4800 | 208 µs | 11.5 ms |
| 2400 | 417 µs | 22.9 ms |
| 1200 | 833 µs | 45.8 ms |
| 600 | 1.67 ms | 91.7 ms |
| 300 | 3.33 ms | 183 ms |

A host should not expect the first reply byte for at least 4 ms, plus the frame
time. At 300 baud a `SUB` reply takes about 1.8 seconds.

## The unit answers; it never announces

**The firmware transmits nothing unprompted.** Listening on the line with nothing
sent produces silence, and in the simulator `isr_rst55` is never even entered
without received data. Every transmit path in every handler runs from a
received-byte dispatch and is preceded by line turnaround.

Handler C emits a five-byte frame:

```
18 18 1E 15 <addr>          CAN CAN RS NAK <addr>
```

`0x2D45` sends `18` and falls into the sequence at `0x2D52` that sends `18 1E 15`
and then `0xA07D`. Two variants exist, `0x60 | address` and `0xA0 | address`,
differing only in bits 7:6 of the last byte, so those are flags.

This is a slave answering on a shared bus. The `MAESTRA` label is worth weighing
against it: the unit may be master of its display chain while remaining a slave on
the serial bus.

## Handler C

Selected when DIP switch port C bit 5 is clear, at a hardwired 9600 8E1. It is
930 bytes at `0x2C80`, and not a variant of the command set above: binary, polled,
and sharing nothing with handlers A and B but the UART.

* **Entirely reactive.** Every transmit path runs from the received-byte dispatch
  at `0x2D06`, preceded by line turnaround and followed by `reply_cleanup`.
* **It reads the UART properly.** `IN 0x81 / ANI 0x78` separates a clean byte from
  a parity, overrun or framing error, and latches the error into `status_flags_a`.
* **Command bytes** dispatch on the received byte: `0x00`, `0x01`, `0x02` (STX),
  `0x03` (ETX), `0x0C`, `0x0F`, `0x18` (CAN), `0x1D` (GS), `0x1E` (RS). `0x0C`
  jumps to `fatal_or_init`, so it is a reset command.
* **Replies** are short binary frames: `0x0F <addr>` to command `0x01`,
  `0x1D <byte>`, `0x02 <addr> …`, and the status frame above.
* **The address** is `0xA07D`, whose low 5 bits come from the port A switches —
  the same address the other handlers use.
* **Message delivery is length-prefixed.** At `0x2F14` the handler takes the
  received byte as a count, stores it in `rx_c_stage_len`, and stages the
  following bytes at `0xA039`. The main loop at `0x03C7` then replays exactly that
  many bytes through `rx_a_store_byte` into the message buffer, using `0x61` as
  the start delimiter and `0x03` as the end delimiter.

The frame layout, any checksum, and which command begins a transaction are not
established. Single bytes fed to it in the simulator produce silence, so it wants
a sequence. This is the largest undecoded region of the ROM, and it needs no
hardware to work on.

An installed unit could be running handler C rather than the command set, so port
C bit 5 is worth reading before assuming which protocol applies.

## Reference implementation

[../tools/pit100.py](../tools/pit100.py) implements all of the above and runs
against either the emulator or a serial port:

```sh
PYTHONPATH=tools python3 tools/pit100.py --sim --address 5 --identify --text HOLA
```

[../tools/say.py](../tools/say.py) is the everyday front end, and
[../tools/bringup.py](../tools/bringup.py) walks a first power-up stage by stage.
See [controller.md](controller.md).

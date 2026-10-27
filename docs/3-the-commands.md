# 3. The commands

The reference for the protocol this unit speaks. Everything here was exercised
against the hardware.

## First, a warning about switches

The firmware carries three different receive protocols and two DIP switch bits
choose between them.

| Port C bit 5 | Port A bit 6 | Protocol |
| --- | --- | --- |
| set | set | **The command set**, which is this document |
| set | clear | A simple delimited text stream |
| clear | any | A binary polled protocol, at a hardwired 9600 8E1 |

This unit is set to the command set. Another unit might not be, so if a sign
answers nothing at any baud rate, that is the first thing to check. The binary
protocol was never decoded.

## The shape of a transaction

```
GS <addr>   <commands and text...>   RS
```

`GS` (`0x1D`) selects a unit. `RS` (`0x1E`) deselects it. Until a unit is
selected it accepts only `EOT`, `VT` and `GS` and ignores everything else.

* The address byte is the unit's own address as printable ASCII, so address 1 is
  `0x31`. There are 32 addresses.
* `0x7F` is a broadcast. Every unit on the line selects.
* A non-matching address leaves a unit as it was, so several signs can share one
  line.

**`RS` is not optional.** A selected unit never repaints. Text sent between `GS`
and `RS` is stored and acknowledged, and the dots do not move until you
deselect. This is deliberate: it stops the panel repainting halfway through an
incoming message.

Measured in emulation, sending `GS '5' STX A B ETX` over 800,000 instructions:

| | Render passes | Characters drawn |
| --- | --- | --- |
| without `RS` | 0 | 0 |
| with `RS` | 131 | 2, exactly the two that changed |

So for a host, **`ACK` means stored, not displayed.**

## The command table

| Code | | Effect |
| --- | --- | --- |
| `0x02` | STX | Start of text. Resets the write pointer |
| `0x03` | ETX | End of text. Validates and replies `ACK` or `NAK` |
| `0x04` | EOT | Block write, or the escape for a literal byte |
| `0x05` | ENQ | Takes a parameter byte: the pad character |
| `0x06` | ACK | Clears the display |
| `0x07` | BEL | Takes a parameter byte: display effect |
| `0x08` | BS | Status query |
| `0x09` | TAB | Reports the cursor position |
| `0x0A` | LF | Line feed |
| `0x0B` | VT | Position the cursor. Takes two parameter bytes |
| `0x0C` | FF | Fill the buffer with the solid block |
| `0x0D` | CR | Carriage return |
| `0x0E` | SO | Read the character at the cursor |
| `0x0F` | SI | Read back what is on the panel |
| `0x10` | DLE | **Resets the unit** |
| `0x11` | DC1 | Cursor up |
| `0x12` | DC2 | Cursor right |
| `0x13` | DC3 | Cursor left |
| `0x14` | DC4 | Cursor down |
| `0x17` | ETB | Erase to the end of the line |
| `0x18` | CAN | Clear the display |
| `0x19` | EM | Erase to the end of the display |
| `0x1A` | SUB | Identify |
| `0x1D` | GS | Select. Takes an address byte |
| `0x1E` | RS | Deselect |

Everything else below `0x20` is ignored. Everything from `0x20` up is message
text.

`0x1B` ESC exists in the ROM but is disabled in this build.

## The message buffer

The buffer is 64 bytes. Messages of 1 to 64 characters all acknowledge on the
unit, and 65 is refused. Overflow is detected rather than ignored: the write
pointer is clamped and an error code is latched, so an over-long message comes
back with a `NAK` instead of being quietly cut.

The panel is 16 characters, so the other 48 buffer positions go somewhere else
on the panel bus. Where was never traced. See
[5-the-panel.md](5-the-panel.md#the-other-48-positions).

The buffer is addressed as a grid of rows 64 columns wide, but this build clamps
at the end of row 0, so in practice there is one row. The row arithmetic is
generic code carried for wider members of the family.

| Code | | Effect |
| --- | --- | --- |
| VT | | `VT <row+0x30> <col+0x30>` positions the cursor |
| CR | | Back to column 0 |
| LF | | Advance a row, back to column 0 |
| DC1 / DC4 | | Cursor up and down one row |
| DC3 / DC2 | | Cursor left and right, wrapping at the row ends |
| ETB | | Erase from the cursor to the end of the row |
| EM | | Erase from the cursor to the end of the buffer |
| CAN | | Fill the whole buffer with spaces |
| FF | | Fill the whole buffer with `0x23`, the solid block |

A cursor move that runs off the buffer latches the same error code an over-long
message does.

## Replies

`ETX` ends a message and answers:

```
<ACK|NAK>  <address>  [errcode  [param1 param2]]
```

`ACK` is `0x06` and `NAK` is `0x15`. The address byte is this unit's address,
encoded the same way as in `GS`. The error code is printable ASCII and is only
present when something went wrong. `0x30` means buffer overflow. A real overflow
reply from this unit reads `15 31 30`, which is `NAK`, address `'1'`, error `'0'`.

Error state is cleared after every reply, so an error never persists into the
next message.

Before replying, the unit drives RTS and waits about 3.9 ms. That is the line
turnaround, and it is why a host must not expect an instant answer.

### Queries

| Code | | Reply |
| --- | --- | --- |
| SUB | identify | 45 bytes: the version string, then the whole configuration block |
| TAB | cursor | Row and column, each offset by `0x30` |
| SO | character | The one byte at the cursor |
| SI | screen | What is actually on the panel |
| BS | status | One status byte |

**`SUB` is the bring-up command.** It needs no prior state, works under
broadcast, and returns a known answer. The observed reply is 49 bytes: four
`SYN` bytes of preamble, then 45 beginning `41 4B 30 30 35 32 2E`, which is
`AK0052.`.

**Reply lengths are not fixed.** The preamble, the conditional address byte and
the conditional error detail all vary. Drain until the line goes quiet instead
of counting bytes.

`TAB` reports position as `0x30 + row` and `0x30 + column`, which only looks
like two digits below 10. Column 16 reads `'@'` and column 63 reads `'o'`.

`SI` reads what is **displayed**, not what is in the message buffer, which makes
it the way to confirm that an update committed:

```
select, message("CHAMARTIN"), home cursor, SI
  -> '                    '    nothing displayed yet, still selected
deselect, select, home cursor, SI
  -> 'CHAMARTIN           '    the repaint has happened
```

`SI` reads from the cursor onward, so home the cursor first.

`BS` returns one byte of hardware status, taken from an input port that is
sampled on every timer tick. Reading it clears it, so the answer means "anything
that happened since you last asked". What the individual bits mean is not known.

## Block writes, and sending control codes

`EOT` takes an ASCII digit and then behaves three ways:

| Form | Effect |
| --- | --- |
| `EOT '1' <64 bytes>` | Write 64 bytes to the second buffer |
| `EOT '2' <ofs-hi> <ofs-lo> <cnt-hi> <cnt-lo> <data...>` | Write `cnt` bytes at an offset |
| `EOT '3' <byte>` | Store one byte verbatim, bypassing the command dispatcher |

Nothing in this build renders the second buffer, so the first two forms are
invisible on this unit.

`EOT '3'` is useful, though. The dispatcher eats every byte below `0x20` as a
command, so a bare control code never reaches the buffer and raises no error:

```
STX 'A' 0x01 'B' ETX            -> buffer: 41 42        the 0x01 vanished
STX 'A' EOT '3' 0x01 'B' ETX    -> buffer: 41 01 42     stored
STX EOT '3' 0x00 'A' ETX        -> buffer: 00 41        even 0x00 works
```

CP437 has real glyphs down there (smileys, card suits, arrows, musical notes),
and this escape is the only way to display them. `tools/pit100.py` applies it
automatically.

## Commands that take a parameter

After one of these, the next byte received is swallowed as a parameter instead
of being treated as a command.

| Command | Parameter |
| --- | --- |
| GS | The address. `0x7F` broadcasts |
| BEL | The display effect, low 3 bits |
| ENQ | The pad character |
| EOT | An ASCII digit choosing a block-write mode |
| VT | Two of them: row, then column |

This is why a session that ends mid-command poisons the next one, and why rule 1
in [2-talking-to-it.md](2-talking-to-it.md#five-rules-for-anything-you-write)
says to start with `RS`.

## Build-time configuration

The firmware reads its own behaviour out of a block of ROM constants rather than
hard-coding it, so the same image can be customised per installation by patching
bytes. `SUB` returns the whole block.

| Address | This build | Meaning |
| --- | --- | --- |
| `0x0072` | `0x01` | Protocol enabled |
| `0x007A` | `0xFF` | Take the address from the switches |
| `0x007B` | `0x40` | Message buffer length, 64 |
| `0x0088` | `0x02` | Bit 1 appends the address to replies |
| `0x008A` | `0x05` | Turnaround enabled, no preamble, 40 delay units |

Three more options are frozen into the code itself as tests of a constant
against itself, which is what a compiled-out option looks like in a binary. Only
one of them matters: it disables a second display the firmware otherwise
supports.

Anyone building a controller for a different unit should read these out of their
own ROM, because another installation could have any of them set differently.

## Timing

The CPU runs at 3.072 MHz, which fixes the delay loops:

| | Time |
| --- | --- |
| Line turnaround before a reply | 3.9 ms |
| 5-byte frame at 9600 baud | 5.7 ms |
| 5-byte frame at 1200 baud | 45.8 ms |
| 5-byte frame at 300 baud | 183 ms |

There is no gap between transmitted characters in this build.

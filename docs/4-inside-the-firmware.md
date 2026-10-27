# 4. Inside the firmware

The ROM is 32 KiB of which 12.3 KiB is used. It is a generic build: 13 panel
drivers behind 32 switch settings, three serial protocols, and an optional
clock board. This unit uses one driver, one protocol and probably no clock
board.

The annotated listing is [../src/firmware.asm](../src/firmware.asm), which
reassembles to a file byte-identical to the ROM. Names live in
[../ghidra/symbols.csv](../ghidra/symbols.csv).

## The machine

| | |
| --- | --- |
| CPU | OKI MSM80C85AH, an Intel 8085 clone |
| Crystal | 6.144 MHz, halved internally, so 3.072 MHz and 325.5 ns per cycle |
| RAM | 256 bytes, inside the M81C55-5 |
| ROM | 32 KiB M27256 EPROM |
| Serial | M82C51A-2 USART, clocked by the M81C55-5's timer |

The clock chain is what fixes every baud rate:

```
6.144 MHz crystal -> 8085, divided by 2 -> CLK OUT 3.072 MHz
  -> 81C55 TIMER IN, divided by the switch-selected 20...640
  -> 81C55 TIMER OUT -> 8251A TxC and RxC, divided by 16
  -> 300 ... 9600 baud
```

Those dividers give all six standard rates exactly, with no error, which is
strong evidence the chain is as described. The last two links were never traced
with a meter.

## Memory

| Range | Size | Contents |
| --- | --- | --- |
| `0x0000-0x7FFF` | 32 KiB | EPROM. Only `0x0000-0x3047` is programmed |
| `0x8000-0x9FFF` | 8 KiB | Unmapped |
| `0xA000-0xA0FF` | 256 B | RAM, the only RAM in the machine |
| `0xA100-0xDFFF` | | Unmapped, or aliases of that RAM |
| `0xE000-0xFFFF` | 8 KiB | A paged window onto an expansion board |

### ROM layout

| Range | Contents |
| --- | --- |
| `0x0000-0x003F` | 8085 hardware vectors, each a jump into the table below |
| `0x0040-0x0063` | Second vector table. All but two entries go to the reset path |
| `0x0064-0x006F` | Version string `AK0052.139U1` |
| `0x0070-0x008B` | Build-time configuration block, reported by `SUB` |
| `0x0092-0x00D1` | A 64-entry halving table |
| `0x00D2-0x00E5` | Reply prefixes and suffixes |
| `0x0260-0x1C0C` | Code |
| `0x1C0D-0x210C` | The 5x7 glyph table, 256 entries |
| `0x210D-0x3047` | Code and more tables |
| `0x3048-0x7FFF` | Erased |

### Only two interrupts are used

Every 8085 vector jumps into a second table, and all but two entries there lead
to the reset path.

| Vector | Handler | Job |
| --- | --- | --- |
| RST5.5 | `0x0260` | Serial receive |
| RST7.5 | `0x02A9` | Periodic tick |

The tick does a lot: it decrements a watchdog and resets the machine when it
expires, samples the hardware status input, debounces the clock board's keypad,
ages the display effects, counts down the delay timers, and drives the 8085's
SOD pin as an output.

### The window at 0xE000

The message buffer lives at `0xE001`, in a window onto an expansion board that
this unit does not have. Writing I/O port `0xE7` selects the page. Reading it
returns `0xFF` when the board is absent, and the firmware then bypasses the
window and accesses local memory directly.

Every access goes through three small routines that do exactly this test, so
absence is handled by design rather than by accident. Nothing in this build ever
changes page, so the paging is a latent capability a host never has to think
about.

Text does render on the real unit, so something answers at `0xE001` here: either
the expansion latch reports present, or the address decoding is incomplete and
the window lands on real RAM. Which one was never determined. The emulator needs
`--xram board` for this reason.

What the window holds:

| Range | Size | Contents |
| --- | --- | --- |
| `0xE001-0xE040` | 64 | **The message buffer**, what a host writes |
| `0xE043-0xE082` | 64 | **The shadow**, what the panel currently shows |
| `0xE084-0xE105` | | A second buffer and shadow, disabled in this build |

The buffer and its shadow are the whole repaint mechanism. The render pass walks
the two in step and calls the panel driver only where they differ, so only
characters that actually changed are redrawn.

### RAM variables worth knowing

| Address | Meaning |
| --- | --- |
| `0xA003`/`0xA004` | Baud divider taken from the switches |
| `0xA009` | Panel family, selects the per-character driver |
| `0xA00A` | Character position within the line |
| `0xA00C` | Chooses receive handler A or B |
| `0xA012` | Mailbox between the receive interrupt and the main loop |
| `0xA01E` | Watchdog counter |
| `0xA024` | Sticky copy of the hardware status input, reported by `BS` |

The stack starts at `0xA0F3` and grows down.

## I/O ports

The 8085 keeps I/O in its own 8-bit space, so none of this is in the memory map.

| Ports | Device |
| --- | --- |
| `0x80-0x81` | USART, the external serial link |
| `0xA0-0xA5` | M81C55-5: DIP switch inputs and the baud timer |
| `0xC0-0xCF` | Clock board, if fitted |
| `0xDE` | Hardware status input, active low |
| `0xE0-0xE5` | The flip-dot interface |
| `0xE7` | Expansion board latch |
| `0xE8-0xEF` | A second display group, initialised but never painted here |

U9 is a CD74HC138E decoder, and the whole map falls out if it decodes address
bits 6:4 with bit 7 as an enable. That gives eight 16-port blocks starting at
`0x80`, each holding exactly one device, and the firmware never touches a port
below `0x80` anywhere in the ROM. The decoder's pins were not traced.

### Serial, 0x80 and 0x81

A standard 8251A pair: data at `0x80`, status and control at `0x81`. Mode and
command words are built either from a hardwired constant or from the switches,
depending on port C bit 5.

### Switches and timer, 0xA0 to 0xA5

| Port | Access | Use |
| --- | --- | --- |
| `0xA0` | write | Command register. Set so all three ports are inputs |
| `0xA1` | read | Port A: address, protocol select, render options |
| `0xA2` | read | Port B: baud rate and character format |
| `0xA3` | read | Port C, 6 bits: protocol family and panel family |
| `0xA4`/`0xA5` | write | Timer preload, which sets the baud rate |

The firmware never writes ports A, B or C, so all 22 bits are switch inputs.

### The clock board, 0xC0 to 0xCF

An optional board that holds the wall clock. Four ports carry the `HH:MM`
digits, validated 0-2, 0-9, 0-5, 0-9. The firmware reads them, makes them ASCII,
and writes them into the first four characters of the message buffer. Another
port carries a three-key keypad for setting the time, debounced over 201 ticks.

Whether this unit has the board is unknown. The routine returns immediately when
the port reads `0xFF`, so an unconnected bus disables the feature silently.

This is the only arithmetic the firmware does on anything that reaches the
panel, and it never touches text a host sends.

## The configuration switches

Three banks, 22 bits, all read-only to the firmware.

### Port A

| Bits | Use |
| --- | --- |
| 4:0 | **Display address**, 0 to 31. Address 0 is special and ignores `RS` |
| 5 | Blank placeholder character: `?` or space |
| 6 | **Receive protocol**: set chooses the command set, clear the text stream |
| 7 | On reset, blank the panel or leave the last message showing |

The address is encoded as printable ASCII, `0x30` to `0x3F` or `0x40` to `0x4F`.
A host selects a unit with `GS <encoded address>` and the unit puts the same
encoding into its replies, which is how this unit's address was read back as 1.

Bit 7 exists because flip-dots hold their image with the power off, so a sign
that reboots can either blank itself or carry on showing its last message.

### Port B

| Bits | Use |
| --- | --- |
| 7:5 | **Baud rate**: 300, 600, 1200, 2400, 4800, 9600 |
| 4 | Data bits: set is 7, clear is 8 |
| 3 | Parity enable |
| 2 | Parity sense: even or odd |
| 1 | Stop bits: clear is 1, set is 2 |
| 0 | Pad with solid blocks instead of spaces, and pick the render pass |

Bit 0 must be clear for legible text, and on this unit it is.

### Port C

| Bits | Use |
| --- | --- |
| 5 | **Protocol family**: set is the ASCII command set with switch-selected line settings, clear is the binary protocol at a hardwired 9600 8E1 |
| 4:0 | **Panel family**, 32 values selecting 13 drivers |

This unit's port C bits 4:0 select the serial shift-register driver described in
[5-the-panel.md](5-the-panel.md).

Port C is not on a DIP bank. There are 19 physical switches against 22 input
bits, so the two banks of eight beside the chip are ports A and B, and port C
comes from one lone switch plus hardwired links. That matches what port C does:
panel family and protocol family are factory decisions, while the address, baud
rate and character format are things an installer sets.

### This unit's settings

Established by talking to the unit rather than by reading switches. It answers
at 1200 baud, 7 data bits, even parity, 1 stop, at address 1, speaking the
command protocol, through the serial shift-register panel driver.

Running those facts back through the tables above fixes the port values:

| Port | Value |
| --- | --- |
| A | `?1?0 0001` |
| B | `0101 1100` |
| C | `10 0011` |

**How the physical switch positions map onto those values is not established.**
Neither polarity of the switch sense reproduces the measured configuration, and
a single sample cannot settle it. Resolving it needs a meter and the board. See
[../TODO.md](../TODO.md).

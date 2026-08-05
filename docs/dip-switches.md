# DIP switch configuration

The firmware only ever **reads** ports A, B and C of the 81C55 (`0xA1`, `0xA2`,
`0xA3`) and writes command word `0xC0` (all ports input). Those 22 input bits
are the three DIP switch banks visible in
[switches1.jpg](../hardware/photos/switches1.jpg),
[switches2.jpg](../hardware/photos/switches2.jpg) and
[switches3.jpg](../hardware/photos/switches3.jpg).

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Port B bits 7:5 — line rate **[C]**

At `0x0474` the firmware reads port B, masks `0xE0` and selects an 81C55 timer
preload for each of six values. The preload is stored at `0xA003`/`0xA004` and
later written to `0xA4`/`0xA5`.

| `PB & 0xE0` | Branch | Preload | Divider | Baud (assuming 3.072 MHz TIMER IN) |
| --- | --- | --- | --- | --- |
| `0x00` | `0x04D2` | *TODO* | | |
| `0x20` | `0x04C5` | *TODO* | | |
| `0x40` | `0x04B8` | *TODO* | | |
| `0x60` | `0x04AB` | *TODO* | | |
| `0x80` | `0x049E` | *TODO* | | |
| other | `0x0491` (default) | `0x4014` → count 20 | 20 | **9600 [L]** |

Filling in the five `TODO` rows from the listing is a five-minute job and gives
the complete baud table — do that before touching the hardware.

## Port C bit 5 — alternate startup path **[C]**

`0x0430` reads port C, tests bit 5, and branches to two very different
initialisations:

* **bit 5 = 0** (`0x0437`): normal path — program the 81C55 timer with the
  fixed `0x4014` preload, initialise the 8251 with mode `0x7E` (8E1), clear
  `0xF0` bytes of RAM, jump to `0x055A`.
* **bit 5 = 1** (`0x0468`): clear only `0x80` bytes of RAM, then read port B to
  select the baud rate, and later build the 8251 mode word from a register at
  `0x0533`. **[L]**

So bit 5 looks like "use hardwired defaults" vs "use the switch-selected
configuration" — possibly a test/service mode. Confirming which is which is a
high-value experiment because it decides whether the switches matter at all.

## Unmapped bits

Port A (`0xA1`, 8 bits, 10 reads) and the remaining port B / port C bits are not
yet attributed. Given the **MAESTRA** sticker and the master/slave hints in
[serial-protocol.md](serial-protocol.md), a **display address** is the most
likely occupant. **[L]**

## Recording the physical state — do this first

Before flipping anything, photograph each bank and write the as-found positions
down. This is the one piece of information that is destroyed by curiosity and
cannot be recovered from the ROM.

| Bank | Port | As-found positions (1 = closed/on) | Notes |
| --- | --- | --- | --- |
| SW1 | *TODO* | *TODO* | |
| SW2 | *TODO* | *TODO* | |
| SW3 | *TODO* | *TODO* | |

Also record, for each bank, whether a closed switch reads as 0 or 1 — pull-ups
to the 81C55 inputs mean **closed usually reads 0**, which inverts every table
above.

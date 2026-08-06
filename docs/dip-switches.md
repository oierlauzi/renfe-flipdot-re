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

| `PB & 0xE0` | Branch | Preload (MSB,LSB) | Divider | Baud if TIMER IN = 3.072 MHz |
| --- | --- | --- | --- | --- |
| `0x00` | `0x04D2` | `0x42`,`0x80` | 640 | 300 |
| `0x20` | `0x04C5` | `0x41`,`0x40` | 320 | 600 |
| `0x40` | `0x04B8` | `0x40`,`0xA0` | 160 | 1200 |
| `0x60` | `0x04AB` | `0x40`,`0x50` | 80 | 2400 |
| `0x80` | `0x049E` | `0x40`,`0x28` | 40 | 4800 |
| `0xA0`, `0xC0`, `0xE0` | `0x0491` (default) | `0x40`,`0x14` | 20 | 9600 |

The top two bits of the MSB are the 81C55 timer mode (`01` = continuous square
wave in every case); the remaining 14 bits are the count.

**The dividers are 20, 40, 80, 160, 320, 640 — an exact 1:2:4:8:16:32 ladder.**
That is independent corroboration that this is a standard baud-rate generator,
and it constrains the absolute rates hard: `baud = TIMER_IN / (divider x 16)`, so
the whole ladder is fixed by one measurement. **[C]**

Two candidate crystals both give a fully standard ladder:

| TIMER IN | divider 20 | ... | divider 640 |
| --- | --- | --- | --- |
| 3.072 MHz | 9600 | | 300 |
| 6.144 MHz | 19200 | | 600 |

3.072 MHz is the more likely of the two, because a 6.144 MHz crystal on the 8085
is halved internally and appears on CLK OUT as 3.072 MHz, which is the usual
source for the 81C55 TIMER IN. **[L]** Measuring CLK OUT settles it outright.

## Port B bits 4:2 — character format **[C]**

`0x04EA` builds the 8251A mode word in register `B`, starting from `0x02`
(baud factor x16) and OR-ing in bits taken from port B:

| Test | Bits set | Meaning |
| --- | --- | --- |
| `B` initial `0x02` | 1:0 = `10` | x16 baud factor |
| `PB` bit 4 set (`0x04EC`) | `0x08` → 3:2 = `10` | **7 data bits** |
| `PB` bit 4 clear | `0x0C` → 3:2 = `11` | **8 data bits** |
| `PB` bit 3 clear (`0x04FE`) | none | **parity disabled** |
| `PB` bits 3:2 == `0b10` (`0x0505`) | via `0x0515` | odd parity |
| otherwise | `0x30` → 5:2 = `11` | **even parity** |

So port B is now fully accounted for: bits 7:5 baud, bit 4 word length, bits 3:2
parity. The `0x7E` literal used on the hardwired path (8E1) is what these
switches produce in their common setting.

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

## Port A bits 4:0 — display address **[C]**

**Confirmed twice over**, from the listing and by experiment.

From the listing, at `0x0F69` inside the ETX handler, the firmware builds its
reply address:

```
0x0F69  IN   0xA1          ; DIP switch port A
0x0F6B  ANI  0x1F          ; keep bits 4:0
0x0F6D  MOV  C,A
0x0F6E  ANI  0x10          ; bit 4 selects the encoding
0x0F70  JZ   0x0F7C
0x0F73  MOV  A,C
0x0F74  ANI  0x0F
0x0F76  ORI  0x40          ; bit 4 set   -> 0x40-0x4F  '@'..'O'
0x0F78  MOV  C,A
0x0F7C  MOV  A,C
0x0F7D  ORI  0x30          ; bit 4 clear -> 0x30-0x3F  '0'..'?'
```

So **32 addresses, encoded as printable ASCII**.

Independently, the status frame the unit transmits carries the same five bits.
Sweeping port A in the simulator moves the last byte in lockstep:

| Port A | bits 4:0 | transmitted frame |
| --- | --- | --- |
| `0x40` | `0x00` | `18 18 1E 15 60` / `18 18 1E 15 A0` |
| `0x41` | `0x01` | `18 18 1E 15 61` / `18 18 1E 15 A1` |
| `0x45` | `0x05` | `18 18 1E 15 65` / `18 18 1E 15 A5` |
| `0x50` | `0x10` | `18 18 1E 15 70` / `18 18 1E 15 B0` |
| `0x5F` | `0x1F` | `18 18 1E 15 7F` / `18 18 1E 15 BF` |

The last byte is exactly `0x60 | (port A & 0x1F)`, with a variant at
`0xA0 | (port A & 0x1F)`. Reproduce with:

```sh
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xDF \
    --xram board --send '02 48 4F 4C 41 03'
```

## Port A bit 6 — receive protocol select **[C]**

At `0x0675`, when no expansion board is present, bit 6 sets `0xA00C` and
therefore which receive handler the RST5.5 ISR dispatches to:

| Bit 6 | `0xA00C` | Handler |
| --- | --- | --- |
| clear | 0 | `0x08B7` — simple delimited text |
| set | 1 | `0x0ACB` — the full command protocol |

A fitted expansion board forces `0xA00C = 1` regardless.

## Still unmapped

Port A bits 7 and 5, and the remaining port B / port C bits. **[?]**

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

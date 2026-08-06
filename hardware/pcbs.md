# PCBs

Three boards. Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Motherboard

Largest of the three. Carries the CPU and its peripherals: U1 MSM80C85AH with its
6.144 MHz crystal, U2 M81C55-5, U3 M82C51A-2, U4 M27256 EPROM, U5 MC14584BCP,
U6 7555, U7 6N139, and the unidentified U8/U9/U10. See [chips.md](chips.md).

## Daughter 1 — serial interface

Carries the screw terminal blocks that the four DB37 conductors land on, and an
**isolated DC-DC converter module** (marked `970593-960119980401`, with
`VI+ / VI- / VO+ / VO-` silkscreen).

### Component inventory

| Count | Part | Notes |
| --- | --- | --- |
| 1 | DC-DC converter module | Isolated, soldered down |
| 1 | U11 MC14584BCP | Hex Schmitt trigger |
| 6 | U12-U17, DIP8 | U14-U17 have yellow bodies; **almost certainly optocouplers** |
| 4 | Diodes | |
| 2 | Electrolytic capacitors | |
| 8 | Film capacitors | |
| 3 | **560 Ω** resistors | |
| 3 | **~1 kΩ** resistors | Bands read as brown(wide) / brown / black / red / brown, direction uncertain — see below |

### Reading the ambiguous resistors

Five bands with one wider than the rest means a 1% part, and the wide band is the
tolerance. Depending on which end is read first the candidates are:

| Reading | Value | E96 standard? |
| --- | --- | --- |
| brown, red, black, brown | 120 × 10 | **1.2 kΩ** | yes |
| brown, black, red, brown | 102 × 10 | **1.02 kΩ** | yes |
| brown, black, red (4-band) | 10 × 100 | **1.0 kΩ** | yes |

**All three land within 20 % of each other, so the exact reading barely matters:
these are ~1 kΩ, 1 %.** A multimeter across one settles it in seconds and is worth
doing, because the precise value pins the design current.

## Daughter 2 — flip-dot interface

Large ribbon connector to the flip-dot panel. Carries U18a/U18b (two ULN2003A
Darlington arrays), U19 (DIP16, ST) and U20 (DIP40). U20 is the strong 81C55
candidate on which the whole `0xE0` interpretation rests — see
[../docs/io-map.md](../docs/io-map.md).

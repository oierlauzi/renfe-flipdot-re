# PCBs

Three boards, one function each.

| Board | Function |
| --- | --- |
| **Mother** | The logic: CPU, RAM, EPROM, USART, DIP switches, and its own regulators |
| **Daughter 1** | Media conversion: the USART's logic-level signals onto RS-422 |
| **Daughter 2** | Display drive: U20's ports and the Darlington sinks, out to the panel ribbon |

This is a modular product. The same motherboard serves any physical layer by
swapping daughter 1, and any panel family by swapping daughter 2, which is what
the firmware's 13 panel drivers and switch-selected line settings are for.

No communication with the station controller passes through daughter 2. Anything
reached down the daughter-2 ribbon is a display; the host link enters at the DB37
and stops at daughter 1.

One thing does not fit the division: `U7`, a 6N139, is recorded as being on the
**motherboard**, while daughter 1 carries three 6N139s of its own — a complete
isolation barrier for three signals. A fourth on the logic board would be
redundant and on the wrong side of the split. The likeliest explanation is that
the board attribution was misrecorded; the `U7` silkscreen is worth checking
against the board.

## Telenorma part numbers

The motherboard carries the Telenorma logo, which together with the EPROM's
`TELENORMA 18 JUN 96` label makes the electronics a Telenorma design and build
rather than Telenorma firmware in a PAGE Ibérica board — see
[../docs/device.md](../docs/device.md#telenorma-and-page-ibérica).

The silkscreen numbering is German-industrial in style, and these are the most
searchable identifiers on the unit. Transcribed from
[photos/Mother%20%28top%29.jpg](photos/Mother%20%28top%29.jpg) and
[photos/Daughter%201%20%28top%29.jpg](photos/Daughter%201%20%28top%29.jpg), so check each
against the board before quoting it elsewhere:

| Board | Silkscreen | Rev |
| --- | --- | --- |
| Mother | `300-48.0216.5586` | `A.2` |
| Mother | `324-48.0216.5586` | `A.2` |
| Mother | `41.0222.0026`, `L1` | `A.1` |
| Daughter 1 | `300-40.0219.9078` | `A.3` |
| Daughter 1 | `324-40.0219.9078` | `A.3` |
| Daughter 1 | `11.0238.4030`, `L1` | `A.3` |
| Daughter 1 | `37623.6`, `BR1` / `L1` | `A.3` |
| Flip-dot carrier | `37624.0 DV2 A.1` / `S7D5-2` | |
| DC-DC module | `970593-960119980401` | |

The paired `300-` and `324-` prefixes on the same number probably distinguish the
bare board from the populated assembly, as is usual in German practice, which
would make `48.0216.5586` and `40.0219.9078` the real design numbers.

Other silkscreen markings: connector designators `STA`, `STB`, `STC`, `STD`;
`ASTU1`; `S17` beside a two-pin blue component; adhesive labels `A 965` and
`BAR ?065`.

## Motherboard

Largest of the three. Carries the CPU and its peripherals: U1 MSM80C85AH with its
6.144 MHz crystal, U2 M81C55-5, U3 M82C51A-2, U4 M27256F1 EPROM, U5 MC14584BCP,
U6 7555, U7 6N139, U9 CD74HC138E, U10 CD74HC373E, and the unidentified U8. See
[chips.md](chips.md).

Two 8-position blue DIP switch banks sit immediately beside the M81C55-5, which
are the ports A and B configuration switches
([../docs/dip-switches.md](../docs/dip-switches.md)). The lone separate switch is
designated `S17`. **Two TO-220 regulators on gold heatsinks supply the logic
rails** from the 24 V input.

The motherboard also carries an unconnected four-way terminal block labelled
`RP RN TP TN` — receive and transmit, positive and negative. Those are
differential pair names rather than a TTL USART header, but nothing identified on
the board can drive a differential pair: U9 and U10 are glue logic, leaving U8 as
the only candidate. What the block connects to has not been traced. See
[connectors.md](connectors.md#the-motherboard-terminal-block).

## Daughter 1 — serial interface

Carries the screw terminal blocks the four DB37 conductors land on, the RS-422
transceivers and optocouplers described in
[connectors.md](connectors.md#the-physical-layer), and an isolated DC-DC converter
module.

### The DC-DC converter

| Property | Value |
| --- | --- |
| Type | `24 IMP 3-05-7`, Melcher IMP series |
| Input | 10-36 V DC |
| Output | 5 V, 500 mA nominal |
| Isolation | input to output, 1500 V DC test voltage |
| Pins | `VI+`, `VI-`, `VO+`, `VO-`, each doubled into two connected through-holes |
| PCB marking | `970593-960119980401` |

The type key decodes off the
[IMP series selection chart](https://datasheet.octopart.com/48IMP6-1212-7-Melcher-datasheet-180627968.pdf):

| Field | Meaning |
| --- | --- |
| `24` | input voltage class — 10-36 V DC, i.e. 24 V nominal |
| `IMP` | the series, 1-12 W DC-DC converters |
| `3` | power class, about 3 W |
| `05` | output 5 V |
| `7` | version |

The chart's 5 V / 500 mA row lists exactly `24 IMP 3-05-7` under the 10-36 V input
column. The manufacturer is Melcher, Switzerland, later part of Power-One; the
silkscreen was read as "Mercher", so the module is worth a second look.

**The output is the isolated line-side supply, not the logic rail.** Daughter 1's
job is media conversion and the motherboard has its own regulators, so an isolated
5 V here powers what sits on the far side of the isolation barrier. Four nets with
no shared ground confirm the isolation itself: a buck or linear regulator shares a
common return and would expose three pins, not four.

The doubling of each pin into two through-holes is ordinary brick-converter
practice, for current capacity and mechanical anchoring.

A handwritten QA sticker, reading something like `30-01-9?` with `OK`, covers part
of the module. It is the only marking on this board and is destroyed by peeling,
so photograph it legibly first.

### Component inventory

| Count | Part | Notes |
| --- | --- | --- |
| 1 | Melcher `24 IMP 3-05-7` | Isolated DC-DC converter, soldered down |
| 1 | U11 MC14584BCP | Hex Schmitt trigger |
| 3 | HP 6N139, yellow DIP8, unlabelled | Optocouplers: the isolation barrier |
| 2 | TI SN75176BP, black DIP8, unlabelled | RS-422 differential transceivers |
| 4 | Diodes | |
| 2 | Electrolytic capacitors | |
| 8 | Film capacitors | |
| 3 | 560 Ω resistors | 6N139 LED drive from 5 V, about 6.5 mA |
| 3 | ~1 kΩ resistors | 6N139 output pull-ups |

The ~1 kΩ parts have five bands with one wider than the rest, meaning a 1% part
with the wide band as tolerance. Depending on which end is read first the value is
1.2 kΩ, 1.02 kΩ or 1.0 kΩ — all within 20% of each other and all E96 standard, so
the exact reading barely matters. A multimeter settles it.

## Daughter 2 — flip-dot interface

Large ribbon connector to the flip-dot panel: 26 conductors, continuing out of the
unit.

| Count | Part | Notes |
| --- | --- | --- |
| 3 | ULN2003A — U17 and U18 TI, U19 ST | 21 Darlington sinks |
| 1 | U20 OKI M81C55-5 | The `0xE0` register group |
| 9 | Transistors, each with a diode and a 22 kΩ resistor | Near the ribbon |
| 2 | Switches | Both open as found; designators not transcribed |

### The 9 discrete transistor stages

A 22 kΩ base resistor means low base current, so these are small-signal switches
rather than power drivers. Two readings fit, and they are not mutually exclusive:

* **Bus drivers** for the address, clock and data lines. The ribbon is 26
  conductors spanning two displays and leaving the enclosure, so it is a long,
  capacitive bus a Darlington sink alone may not drive cleanly. Port B's 8 address
  bits plus clock and data is 10 signals, and 9 is close to that.
* **The missing high side.** ULN2003As can only sink, yet a flip-dot coil needs
  current both ways. If these 9 are PNP or emitter-follower source drivers, with a
  Darlington sink pulling each base through its 22 kΩ, that gap is closed.

The diodes fit either reading: flyback clamping for an inductive load, or reverse
protection on a long external bus. Deciding needs the transistors' part numbers
and a look at which side of the load they sit on.

The two switches on this board cannot feed U20's port C, which is configured
entirely as outputs, so they reach either the 81C55 on the motherboard or
something else.

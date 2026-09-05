# PCBs

Three boards. Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Architecture: one function per board **[L]**

The three boards divide the job cleanly, and stating that up front is useful
because it **rules hypotheses out**:

| Board | Function |
| --- | --- |
| **Mother** | The logic. CPU, RAM, EPROM, USART, DIP switches, and its own regulators. |
| **Daughter 1** | **Media conversion only.** Takes the USART's logic-level signals from the motherboard and puts them on whatever physical layer the installation uses. |
| **Daughter 2** | **Display drive only.** U20's ports and the Darlington sinks, out to the panel ribbon. |

This is a modular product: the same motherboard serves any physical layer by
swapping daughter 1, and any panel family by swapping daughter 2 — which is
exactly what the firmware's 13 panel drivers and switch-selected line settings
imply.

**The constraint that does the work: no communication with the station controller
passes through daughter 2.** Anything reached down the daughter-2 ribbon is a
*display* — the flip-dot line, the LED panel, the `0xE7` expansion that backs the
message buffer. The host link enters at the DB37 and stops at daughter 1.

Two things this immediately settles, and one it strains:

* **The logic supply is the motherboard's own.** Two TO-220 regulators on gold
  heatsinks sit on it (below). Daughter 1's isolated converter is therefore *not*
  the logic supply — a media-conversion board has no business powering the CPU.
* **A "second channel for the LED matrix" on daughter 1 is ruled out.** That was
  offered in [connectors.md](connectors.md) to explain three signals in each
  direction. The LED panel is a display, so it belongs behind daughter 2.
* **Strain: `U7`, a 6N139, is recorded as being on the *motherboard*.** Daughter 1
  turns out to carry **three 6N139s of its own** — a complete isolation barrier
  for three signals — so a fourth on the logic board is redundant and sits on the
  wrong side of this model. The likeliest resolution is that the board attribution
  was misrecorded; **check which board the `U7` silkscreen is on**
  ([chips.md](chips.md#u7--high-speed-optocoupler)).

## Telenorma part numbers **[C] — but verify the digits against the board**

**The motherboard carries the Telenorma logo.** Combined with the EPROM's
`TELENORMA 18 JUN 96` label, that makes the electronics a **Telenorma design and
build**, not merely Telenorma firmware in a PAGE Ibérica board — see
[../docs/device.md](../docs/device.md#dating-and-the-telenorma-connection-c-for-the-label).

The silkscreen numbering is consequently German-industrial in style, and these are
**the most searchable identifiers on the whole unit**. Transcribed from
[photos/mother_pcb_top.jpg](photos/mother_pcb_top.jpg) and
[photos/daughter_pcb1_top.jpg](photos/daughter_pcb1_top.jpg) — read off
photographs, so **check each against the board before quoting it anywhere**:

| Board | Silkscreen | Rev |
| --- | --- | --- |
| Mother | `300-48.0216.5586` | `A.2` |
| Mother | `324-48.0216.5586` | `A.2` |
| Mother | `41.0222.0026`, `L1` | `A.1` |
| Daughter 1 | `300-40.0219.9078` | `A.3` |
| Daughter 1 | `324-40.0219.9078` | `A.3` |
| Daughter 1 | `11.0238.4030`, `L1` | `A.3` |
| Daughter 1 | `37623.6`, `BR1` / `L1` | `A.3` |
| DC-DC module | `970593-960119980401` | |

The paired `300-` and `324-` prefixes on the *same* number are worth noting: in
German practice that usually distinguishes the **bare board** from the **populated
assembly**. If so, `48.0216.5586` and `40.0219.9078` are the real design numbers.
**[L]**

Other silkscreen markings: connector designators `STA`, `STB`, `STC`, `STD`;
`ASTU1`; `S17` beside a two-pin blue component; adhesive labels `A 965` and
`BAR ?065`.

## Motherboard

Largest of the three. Carries the CPU and its peripherals: U1 MSM80C85AH with its
6.144 MHz crystal, U2 M81C55-5, U3 M82C51A-2, U4 M27256 EPROM, U5 MC14584BCP,
U6 7555, U7 6N139, and the unidentified U8/U9/U10. See [chips.md](chips.md).

Two 8-position blue DIP switch banks sit immediately beside the M81C55-5,
confirming they are the ports A and B configuration switches
([../docs/dip-switches.md](../docs/dip-switches.md)). The lone separate switch is
designated **`S17`** on the silkscreen. Two TO-220 regulators on gold heatsinks
supply the logic rails.

## Daughter 1 — serial interface

Carries the screw terminal blocks that the four DB37 conductors land on, and an
**isolated DC-DC converter module** (marked `970593-960119980401`, with
`VI+ / VI- / VO+ / VO-` silkscreen).

### The DC-DC converter

| Property | Value | Conf. |
| --- | --- | --- |
| Type | **`24 IMP 3-05-7`**, Melcher IMP series | **[C]** observed |
| Input | **10-36 V DC** (datasheet); module printing read as 9-36 V | **[C]** |
| Output | **5 V, 500 mA nominal** | **[C]** datasheet |
| Isolation | **Genuine**, input to output; 1500 V DC test voltage | **[C]** |
| Pins | `VI+`, `VI-`, `VO+`, `VO-`, each doubled into two connected through-holes | **[C]** |

The type key decodes straight off the
[IMP series selection chart](https://datasheet.octopart.com/48IMP6-1212-7-Melcher-datasheet-180627968.pdf):

| Field | Meaning |
| --- | --- |
| `24` | input voltage class — **10-36 V DC**, i.e. 24 V nominal |
| `IMP` | the series (1-12 W DC-DC converters) |
| `3` | power class, ~3 W |
| `05` | **output 5 V** |
| `7` | version |

The chart's `5 V / 500 mA` row lists exactly `24 IMP 3-05-7` under the 10-36 V
input column. Manufacturer **Melcher** (Switzerland, later Power-One) — the
silkscreen was read as "Mercher", but the type key and the datasheet are Melcher's,
so the module is worth a second look.

A 24 V nominal input corroborates the 24 V rail recorded in
[psu.md](psu.md#24-vdc-rail) independently of the chassis label.

**The output is the isolated line-side supply, not the logic rail.** Daughter 1's
job is media conversion, and the motherboard has its own TO-220 regulators for the
logic — so an isolated 5 V on the comms board is there to power whatever sits on
the far side of the isolation barrier. That is what an isolated brick on a line
interface is normally for.

> **This conclusion was retracted and then restored, wrongly in between.** For one
> revision the section above claimed the *four-pin count* proved the output floats
> against the display's 0 V, which does not follow — four pins prove
> input-to-output isolation and say nothing about which side the display 0 V is
> on. When the output turned out to be **5 V / 500 mA**, that looked like a logic
> rail and the whole conclusion was reversed.
>
> **The reversal was wrong**, and the disproof was already in this file: *"Two
> TO-220 regulators on gold heatsinks supply the logic rails"* — on the
> motherboard, in the Motherboard section. The original conclusion stands; only
> its justification was bad.
>
> What *does* change with the 5 V figure: this rail cannot source 20 mA at current
> loop voltages, so an **active** loop powered from it is implausible. An isolated
> 5 V supply feeding optocouplers and a line driver fits either a **passive**
> current loop or an isolated differential link.

**Four nets and no shared ground** does establish one thing: a non-isolated
converter — a buck or a linear regulator — shares a common return between input
and output and would expose three pins, not four. So the module is genuinely
isolated, input to output. What it does **not** establish is which side the
display's 0 V sits on; see the retraction above.

The doubling of each pin into two through-holes is ordinary brick-converter
practice — current capacity and mechanical anchoring. It carries no further
information.

**Before removing the sticker:** photograph it legibly first. It appears to be a
handwritten QA mark reading something like `30-01-9?` with `OK`. The EPROM's
`TELENORMA 18 JUN 96` label is now the unit's primary date evidence, so this one
is corroboration rather than the only record — but it is still the only marking
on *this* board and it is destroyed by peeling. Then photograph what is
underneath: the printed part number should give the rated output voltage, current
and isolation rating, corroborating the measurement without needing to power
anything.

### Component inventory

| Count | Part | Notes |
| --- | --- | --- |
| 1 | DC-DC converter module | Isolated, soldered down |
| 1 | U11 MC14584BCP | Hex Schmitt trigger |
| 5 | DIP8, **unlabelled** | 3 have yellow bodies; **almost certainly optocouplers** |
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

Large ribbon connector to the flip-dot panel — **26 conductors**, continuing out of
the unit to the (absent) LED panel.

| Count | Part | Notes |
| --- | --- | --- |
| 3 | ULN2003A (U17, U18 TI; U19 ST) | **21 Darlington sinks** |
| 1 | U20, DIP40 | Strong 81C55 candidate; the whole `0xE0` reading rests on it |
| **9** | **Transistors, each with a diode and a 22 kΩ resistor** | Near the ribbon — see below |

### The 9 discrete transistor stages

Each transistor is paired with a diode and a **22 kΩ** resistor. A 22 kΩ base
resistor means **low base current**, so these are small-signal switches rather
than power drivers.

Two readings, not mutually exclusive:

* **Bus drivers** for the address, clock and data lines. The ribbon is 26
  conductors spanning two displays and leaving the enclosure, so it is a long,
  capacitive bus that a Darlington sink alone may not drive cleanly. Note the
  count: port B's 8 address bits plus clock plus data is 10 signals, and 9 is
  suggestively close.
* **The missing high side.** ULN2003As can only sink, yet a flip-dot coil needs
  current both ways — an open question in
  [../docs/flipdot-interface.md](../docs/flipdot-interface.md). If these 9 are
  PNP or emitter-follower **source** drivers, with a Darlington sink pulling each
  base through its 22 kΩ, that gap is closed.

The diodes are consistent with either: flyback clamping for an inductive load, or
reverse protection on a long external bus. Deciding between the readings needs the
transistors' part numbers and a look at which side of the load they sit on. **[?]**

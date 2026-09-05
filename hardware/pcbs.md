# PCBs

Three boards. Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

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
| Marking | **`Mercher 24 IMP 3-50-7`** | **[C]** observed |
| Pins | `VI+`, `VI-`, `VO+`, `VO-`, each doubled into two connected through-holes | **[C]** |
| Isolation | **Genuine** — see below | **[C]** |
| Input voltage | **9-36 VDC**, printed on the module | **[C]** observed |
| Output voltage | **not printed anywhere on the module** — still the highest-value measurement on the board | |

A **9-36 VDC** input is the standard 4:1 wide range for a **24 V nominal**
system, so it corroborates the 24 VDC rail recorded in
[psu.md](psu.md#24-vdc-rail) independently of the chassis label. Whether the `24`
in the part number also refers to that nominal is a guess. **[?]**

> **A stated expectation, falsified.** [TODO.md](../TODO.md) reasoned that the
> printed part number under the QA sticker "should give the rated output voltage
> without powering anything". It does not — the module states its *input* range
> and nothing about its output. Measuring `VO+` to `VO-` is unavoidable.

**Four nets and no shared ground is the important detail.** A non-isolated
converter — a buck or a linear regulator — necessarily shares a common return
between input and output, and would expose three pins, not four. Two independent
pairs means the output is **galvanically floating with respect to the display's
internal 0 V**. That confirms the module exists to power an isolated line side,
which is the premise the whole current-loop reading rests on
([connectors.md](connectors.md#what-daughter-1s-parts-imply-l)).

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

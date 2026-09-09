# Device identification

From the chassis sticker ([hardware/photos/sticker.jpg](../hardware/photos/sticker.jpg)):

| Field | Value |
| --- | --- |
| Manufacturer | **PAGE Ibérica, S.A.** |
| Address | Av. de la Industria 25, 28760 Tres Cantos (Madrid), España |
| Model | **PI-T100-M** |
| Serial | `985412?` (last digit obscured by the torn label) |
| Role | **MAESTRA** — *master* |
| Description | "1 línea de texto. Módulos de PLACA MÓVIL de 100 mm" |

The firmware version string, at ROM offset `0x0064`, is **`AK0052.139U1`**. The
unit returns it in response to the `SUB` command.

## Dating

The EPROM carries a sticker reading **`TELENORMA 18 JUN 96`** — 18 June 1996 —
which dates the firmware. The chassis plate gives a serial but no date.

| Source | Reading |
| --- | --- |
| EPROM label | `TELENORMA 18 JUN 96` |
| DC-DC converter | `970593-960119980401` on the PCB; the module itself is a Melcher `24 IMP 3-05-7`. The `9601` is possibly a 1996 date code |
| Converter QA sticker | handwritten, `30-01-9?` |
| Chassis plate serial | `985412?`, no date |

Everything is consistent with a mid-1990s build.

## Telenorma and PAGE Ibérica

**Telenorma** descends from Telefonbau und Normalzeit of Frankfurt — literally
"telephone construction and standard time". Alongside telephone systems the firm
built public and station clock and time-distribution installations, and later
became part of Bosch Telecom.

Two marks show the electronics are Telenorma's, not just the firmware: the
`TELENORMA 18 JUN 96` sticker on the EPROM, and the **Telenorma logo on the
motherboard silkscreen**. All three boards carry German-industrial part numbering
(`300-48.0216.5586`, `324-40.0219.9078`, `11.0238.4030` and others, listed in
[../hardware/pcbs.md](../hardware/pcbs.md#telenorma-part-numbers)).

So the division of labour is:

* **Telenorma** designed and manufactured the control electronics and wrote the
  firmware.
* **PAGE Ibérica** supplied the sign — enclosure, flip-dot panel, fluorescent
  lighting — and put its plate on the outside. The plate reads `FABRICADO`, so
  either manufacture under licence or assembly around bought-in Telenorma boards.
* `MAESTRA` is PAGE Ibérica's Spanish terminology layered over a Telenorma
  product.

This is therefore not a RENFE one-off. Telenorma-designed hardware implies a
product family deployed wherever Telenorma sold, so identical or near-identical
boards may survive under Telenorma part numbers — a much wider search than
Spanish rail sources. Documentation is worth hunting under Telenorma and Bosch
Telecom as well as PAGE Ibérica; see [references.md](references.md).

The protocol supports that reading. It is a generic intelligent display terminal:
ASCII control codes, `GS`-based addressing, `STX`/`ETX` framing, `ACK`/`NAK`, an
identify command and cursor positioning.

## What the label means

* **MAESTRA (master).** This unit is the master of a set. The firmware probes for
  a second display board (`IN 0xE7` at `0x040F`) and the serial protocol carries a
  display address taken from the DIP switches — see
  [dip-switches.md](dip-switches.md) and [serial-protocol.md](serial-protocol.md).
* **PLACA MÓVIL de 100 mm** — *moving plate*, i.e. flip-dot, with 100 mm
  character height.
* **1 línea de texto** — a single 16-character text line.
* **PI-T100** is the product family; the `-M` suffix matches *Maestra*.

## Display geometry

| Property | Value |
| --- | --- |
| Characters | 16 |
| Character cell | 5 wide x 7 high dots |
| Total dots | 80 x 7 = 560 |
| Character height | 100 mm |

## Operation in service

The unit is the upper element of a platform sign assembly:

| Element | Content |
| --- | --- |
| **Flip-dot line** (this unit) | The train's final destination, and the ETA to this station in minutes |
| **Vinyl sticker**, right of the panel | The fixed word `min`, so the last one or two characters of the line are the countdown |
| **LED matrix** below (absent here) | Scrolls the list of upcoming stations, and service notices |
| **Fluorescent tube**, lower edge | Illuminates the flip-dot panel at night |

The `min` being a sticker rather than rendered text means the 16 characters are
laid out as destination text on the left and a numeric countdown right-aligned at
the end. That layout is a convention of whatever drives the sign, not something
the display knows about.

## What this unit does and does not do

The device performs no arithmetic on message content. Nothing formats a number
into characters for the text line, there is no minute-scale timer, and nothing
decrements a countdown. **The countdown is pushed from upstream**, one text
update at a time; something re-sends the line every minute.

The ROM's only ASCII digit arithmetic belongs to the wall clock rather than the
message. `clock_read_time` ORs `0x30` into four digits to make ASCII, and
`clock_inc_minute` / `clock_inc_hour` do mod-60 and mod-24 carries — see
[io-map.md](io-map.md#0xc0-0xcf--clock-board-and-keypad). Those digits come from
and return to the clock board and land in display positions 0-3. They never touch
the text a host sends.

What is local to this unit:

* Glyph rendering and the flip-dot drive.
* The display attributes selected by `BEL`, with their durations.
* Serial addressing, framing, and `ACK`/`NAK` error reporting.

A replacement controller therefore sends the whole 16-character line, digits
included, and re-sends it as the ETA changes.

## Deployment

RENFE Cercanías station platform signage. Three assemblies using this generation
of display have been seen in service:

| Variant | Composition | Siting | Photo |
| --- | --- | --- | --- |
| **Single-sided platform** | 1 flip-dot row + 1 LED matrix row | Shows trains calling at that platform | <https://maps.app.goo.gl/2x7p8cGFKuDc8iAQ8> |
| **Double-sided platform** | Two of the above, back to back | Readable from both sides | <https://maps.app.goo.gl/L88dpQK7dEkBPZzP8> |
| **Station-wide** | 2-4 stacked flip-dot rows of **20 characters** + 1 LED row | Concourse; all platforms at once | <https://maps.app.goo.gl/Nm12rbAVFkfoDoA66> |

The station-wide variant's 20-character rows show that **row width is a
per-variant property, not a constant of the design**. That fits how thoroughly
the firmware parameterises the panel — 32 switch settings selecting 13 distinct
drivers — and the fact that `char_pos` is a flat counter which no code bounds
against a width.

Judging by the station it came from, this specimen probably belonged to a
double-sided assembly, but no second unit was recovered. What the character bus
reaches after it leaves the enclosure is discussed in
[flipdot-interface.md](flipdot-interface.md#what-the-rest-of-the-character-bus-reaches).

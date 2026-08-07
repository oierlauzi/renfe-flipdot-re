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

Firmware version string, at ROM offset `0x0064`: **`AK0052.139U1`**

## Dating, and the TELENORMA connection **[C] for the label**

The EPROM carries a sticker reading **`TELENORMA 18 JUN 96`**. That is the first
hard date found anywhere on the unit — the chassis plate gives a serial but no
date — and it dates the *firmware*, which is the date that matters most here.

| Evidence | Reading |
| --- | --- |
| EPROM label | `TELENORMA 18 JUN 96` → **18 June 1996** |
| DC-DC converter marking | `970593-960119980401` — the `9601` may be a 1996 date code **[?]** |
| Converter QA sticker | handwritten, `30-01-9?` **[?]** |
| Chassis plate serial | `985412?`, no date |

Everything is consistent with a **mid-1990s build**.

### Why a German company's marks on a Spanish sign

**Telenorma** descends from **Telefonbau und Normalzeit** of Frankfurt — literally
"telephone construction and standard time". Alongside telephone systems the firm
built **public and station clock and time-distribution installations**; it later
became part of Bosch Telecom. **[L]**, and worth verifying properly.

**The electronics are Telenorma's, not just the firmware.** Two independent marks:

| Evidence | What it covers |
| --- | --- |
| `TELENORMA 18 JUN 96` sticker on the EPROM | the **firmware** |
| **Telenorma logo on the motherboard silkscreen** | the **hardware design and build** |

Together with German-industrial part numbering across all three boards
(`300-48.0216.5586`, `324-40.0219.9078`, `11.0238.4030` …, see
[../hardware/pcbs.md](../hardware/pcbs.md#telenorma-part-numbers-c--but-verify-the-digits-against-the-board))
that settles the division of labour much more sharply than the EPROM label alone
did:

* **Telenorma designed and manufactured the control electronics** and wrote the
  firmware.
* **PAGE Ibérica supplied the sign** — the enclosure, the flip-dot panel, the
  fluorescent lighting, the Spanish market — and put its plate on the outside.
  The plate does say `FABRICADO`, so manufacture under licence, or assembly
  around bought-in Telenorma boards, are both plausible. **[?]**
* `MAESTRA` is PAGE Ibérica's Spanish terminology layered over a Telenorma
  product.

Three consequences worth carrying:

1. **This is not a RENFE one-off.** Telenorma-designed hardware means a product
   family with a documented bus, deployed wherever Telenorma sold — most likely
   including German stations. Identical or near-identical boards may survive under
   Telenorma part numbers, which is a far wider search than Spanish rail sources.
2. **Documentation should be hunted under Telenorma and Bosch Telecom too**, not
   only PAGE Ibérica — see [references.md](references.md).
3. **It quietly supports the current-loop theory.** German time-distribution and
   clock-bus installations of that era commonly ran on **20 mA current loops**,
   which is independently where the component analysis on daughter 1 already
   points ([../hardware/connectors.md](../hardware/connectors.md#what-daughter-1s-parts-imply-l)). **[L]**

The decoded protocol also reads like a *generic intelligent display terminal*
rather than something bespoke: ASCII control codes, `GS`-based addressing,
`STX`/`ETX` framing, `ACK`/`NAK`, an identify command and cursor positioning. That
is the shape of a documented product bus, which makes finding the original
specification a realistic prospect rather than a fantasy.

## What the label tells us

* **"MAESTRA" (master).** This unit is the master of a set. Combined with the
  second-display-board probe in the firmware (`IN 0xE7` at `0x040F`), it implies
  a master/slave arrangement — so the serial protocol very likely carries a
  **display address**, and the DIP switches very likely set it. See
  [dip-switches.md](dip-switches.md) and [serial-protocol.md](serial-protocol.md).
* **"PLACA MÓVIL de 100 mm"** — *moving plate*, i.e. flip-dot / flip-disc, with
  100 mm character height.
* **"1 línea de texto"** — a single 16-character text line, consistent with the
  panel geometry and with the 5-byte-per-character glyph table.
* **`PI-T100`** is the product family; the `-M` suffix matches *Maestra*. A
  slave variant almost certainly exists under a different suffix.

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
| **Flip-dot line** (this unit) | Train's final destination, and the ETA to this station in minutes, counting down |
| **Vinyl sticker**, right of the panel | The fixed word `min`, so the last 1-2 characters of the line are the countdown |
| **LED matrix** below (absent here) | Scrolls the list of upcoming stations, and service notices |
| **Fluorescent tube**, lower edge | Illuminates the flip-dot panel at night |

The `min` being a *sticker* rather than rendered text is a useful detail: it means
the 16 characters are laid out as destination text on the left and a numeric
countdown right-aligned at the end, and that this layout is a convention of
whatever drives the sign, not something the display knows about.

## What this unit does and does not do **[C]**

Worth stating precisely, because it determines what a replacement controller has
to implement.

**This device does not compute anything.** It has no arithmetic on the message
content at all:

* Nothing formats **message content** into digits. No routine turns a number
  into characters for the text line.
* There is **no minute-scale timer** and nothing decrements a countdown.
* So the **countdown is pushed from upstream**, one text update at a time. The
  sign does not tick down on its own; something re-sends the line every minute.

The one piece of ASCII digit arithmetic in the ROM belongs to the **wall clock,
not the message**: `clock_read_time` ORs `0x30` into four digits to make ASCII,
and `clock_inc_minute`/`clock_inc_hour` do mod-60 and mod-24 carries — see
[io-map.md](io-map.md#0xc0-0xcf--a-clock-with-keys-c). Those digits come from and
go back to the clock board, land in display positions 0-3, and never touch the
text a host sends. Nothing local computes an ETA.

What *is* local to this unit:

* Glyph rendering and the flip-dot drive.
* The display effects — 8 modes selected by `BEL`, with durations
  (see [serial-protocol.md](serial-protocol.md)). Scrolling and blinking timing
  is handled here, even though the content is not.
* Serial addressing, framing, and the ACK/NAK error reporting.

**Consequence for a replacement controller:** send the whole 16-character line,
including the digits, and re-send it as the ETA changes.

## Deployment

RENFE Cercanías station platform signage. The station-list scrolling and notice
text belong to the LED panel or to the upstream controller, not to this board.

I/O `0xC0-0xCF` is not the LED matrix interface — it is a clock board, four
`HHMM` digit registers plus a time-setting keypad
([io-map.md](io-map.md#0xc0-0xcf--a-clock-with-keys-c)). The candidates for the
LED panel are the `0xE8-0xEF` port group and the upper half of the character bus;
see
[flipdot-interface.md](flipdot-interface.md#the-led-panel-is-probably-characters-16-63-of-the-same-buffer-l).

## Variants of this generation **[C] observed in service, by the owner**

Three distinct assemblies using this generation of display have been seen in
service. They matter here because they constrain what the ribbon leaving this
enclosure can plausibly reach.

| Variant | Composition | Siting | Photo |
| --- | --- | --- | --- |
| **Single-sided platform** | 1 flip-dot row + 1 LED matrix row | Wall-mounted on a platform or at its entrance; shows trains calling at that platform | <https://maps.app.goo.gl/2x7p8cGFKuDc8iAQ8> |
| **Double-sided platform** | Two of the above, back to back | Readable from both sides | <https://maps.app.goo.gl/L88dpQK7dEkBPZzP8> |
| **Station-wide** | **2-4 stacked flip-dot rows** of **20 characters** + 1 LED row | Concourse; all platforms at once | <https://maps.app.goo.gl/Nm12rbAVFkfoDoA66> |

The station-wide variant's **20-character rows** are the useful detail: this
specimen's row is **16**, so row width is a per-variant property and not a
constant of the design. It is consistent with how thoroughly the firmware
parameterises the panel — 32 switch settings selecting 13 distinct drivers — and
with `char_pos` being a flat counter that no code ever bounds against a width.

### Provenance of this specimen **[?]**

Deduced from the station it came from, the owner expects a **double-sided**
assembly — but **no second unit was recovered**. Either it was taken first, or
the deduction is wrong and this was a single-sided device. Unresolved, and it
matters: it is one of three competing guesses about what the character bus reaches
once it leaves the enclosure, in
[flipdot-interface.md](flipdot-interface.md#but-we-are-guessing-three-readings-of-positions-16-63-).

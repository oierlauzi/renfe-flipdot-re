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

* There is **no minutes-to-ASCII conversion** anywhere in the ROM. The only two
  `ADI 0x30` instructions in the entire firmware are in the `TAB` cursor-position
  report. Nothing formats a number into digits for display.
* There is **no minute-scale timer** and no decrement of an ASCII digit.
* So the **countdown is pushed from upstream**, one text update at a time. The
  sign does not tick down on its own; something re-sends the line every minute.

What *is* local to this unit:

* Glyph rendering and the flip-dot drive.
* The display effects — 8 modes selected by `BEL`, with durations
  (see [serial-protocol.md](serial-protocol.md)). Scrolling and blinking timing
  is handled here, even though the content is not.
* Serial addressing, framing, and the ACK/NAK error reporting.

**Consequence for a replacement controller:** send the whole 16-character line,
including the digits, and re-send it as the ETA changes.

## Deployment

RENFE Cercanías station platform signage. The unit also has an interface for an
additional LED matrix panel (I/O `0xC0` and/or `0xE8` groups) which is not
present in this specimen. The station-list scrolling and notice text belong to
that panel or to the upstream controller, not to this board.

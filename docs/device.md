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

## Deployment

RENFE Cercanías station platform signage. The unit also has an interface for an
additional LED matrix panel (I/O `0xC0` and/or `0xE8` groups) which is not
present in this specimen.

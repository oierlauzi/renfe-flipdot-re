<img src=".github/title.svg" alt="renfe-flipdot-re, rendered as a flip-dot destination sign">

<img src=".github/renfe-galleta-glyph.png" width="90" align="right" alt="RENFE's 'galleta' logo, hand-drawn as a single 5x7 flip-dot glyph. A fan mockup, not ROM data">

Reverse engineering of a **PAGE Ibérica PI-T100-M** flip-dot platform display,
the kind that hung over RENFE Cercanías platforms from the mid-1990s and told
you which train was coming and how many minutes away it was. They are being
taken down as stations are refitted.

The goal was to put arbitrary text on one from a modern computer, over its
original serial link, with no modification to the hardware or the firmware.

**It works.** Text goes onto a real panel over the original link. There is also
an [ESP32 controller](controller/) that replaces the sign's upstream controller
outright, and [a web replica](web-replica/) for anyone without a panel in their
living room.

## Español

Ingeniería inversa de un teleindicador **PAGE Ibérica PI-T100-M**, de los que se
instalaron en las estaciones de **RENFE Cercanías** a mediados de los 90. El
proyecto tiene el fin de preservar, experimentar y divulgar.

El objetivo era enviar texto arbitrario al panel desde un ordenador moderno, a
través del enlace serie existente, **sin modificar el hardware ni el software**.
Funciona. También hay un [controlador ESP32](controller/) que sustituye al
equipo que gobernaba el cartel, y [una réplica web](web-replica/).

Este proyecto se documenta en **inglés**: es el idioma en el que estoy
acostumbrado a programar, y hacerlo en otros me resulta inconsistente.

## The manufacturer, 1997

<img src=".github/page-iberica-1997.jpg" width="640" alt="PAGE Ibérica advertisement from Vía Libre, March 1997, showing a platform display reading 'Vía 1, ATOCH-T.CANT, 6 min'">

PAGE Ibérica's own advertisement in *Vía Libre*, March 1997. The display in the
photograph is not this model, but the layout is the one this unit was built for:
the destination on the left, the minutes pushed to the right-hand end, and the
word `min` printed beside them rather than drawn by the sign. The Tres Cantos
street address is the one on this unit's chassis plate, down to a different
house number.

## The font

All 256 glyphs, extracted from the ROM and rendered flip-dot style. It is IBM
CP437, with `ñ` and `Ñ` quietly duplicated into the lower half so they survive a
7-bit serial link.

![The 256-glyph font, 5x7 dots each](docs/font-map.png)

## Putting text on it

```sh
pip install pyserial
tools/say.py CHAMARTIN            # one line onto the panel
tools/say.py                      # or type lines interactively
tools/say.py --fill               # every dot on
```

That needs a 4-wire RS-422 adapter and four wires.
[docs/2-talking-to-it.md](docs/2-talking-to-it.md) has the wiring and the
first-power-up procedure.

## Where to start

[**docs/**](docs/) is written to be read in order, from what the device is to
how it drives its dots. The first three files are enough to put text on a panel.

| | |
| --- | --- |
| [docs/](docs/) | The documentation, in reading order |
| [hardware/](hardware/) | Parts, boards, connectors, photographs |
| [controller/](controller/) | The ESP32 firmware that replaces the original controller |
| [web-replica/](web-replica/) | A browser replica of the display |
| [tools/](tools/) | Scripts: drive a panel, emulate the firmware, read the ROM |
| [original/](original/) | The ROM dump. Ground truth, never edited |
| [src/](src/) | Annotated assembly that rebuilds the ROM byte for byte |
| [ghidra/](ghidra/) | The symbol table and the script that applies it |
| [TODO.md](TODO.md) | The questions that stayed open |

## Safety

Flip-dot coils expect short pulses. Leaving one energised will overheat it and
can destroy it. This matters for any experiment that drives the panel directly
instead of through the original firmware. There is also mains inside the
enclosure in two places, because the fluorescent tube's ballast lives in there.

## Licence

[MIT](LICENSE) for the documentation, the tools and the code. The original
firmware in [original/](original/) belongs to its authors and is included only
for interoperability research and preservation.

This is an unofficial fan project, not affiliated with, endorsed by or sponsored
by RENFE, PAGE Ibérica or Telenorma. Trademarks and logos shown belong to their
owners and are used only to identify the hardware being documented.

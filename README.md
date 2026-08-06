# renfe-flipdot-re

<img src=".github/renfe-galleta-glyph.png" width="90" align="right" alt="RENFE's 'galleta' logo, hand-drawn as a single 5x7 flip-dot glyph -- a fan mockup, not ROM data">

Reverse engineering of a **PAGE Ibérica PI-T100-M** flip-dot platform display,
as used in RENFE Cercanías stations, for preservation and play.

**Goal:** drive arbitrary text onto the panel from a modern host, over the
existing DB37 serial link, with **no hardware modification**.

Nothing is soldered, nothing is patched, and [original/](original/) stays
byte-for-byte as it came off the EPROM.

I document this project in **English**: it's the language I'm used to
developing software in, and working in other languages has proven
inconsistent for me.

## Español

Ingeniería inversa de un teleindicador modelo **PAGE Ibérica PI-T100-M**. Este
modelo de pantalla se instaló en las estaciones de **RENFE Cercanías** a
mediados de los 90. El proyecto tiene el fin de preservar, experimentar y 
divulgar.

**Objetivo:** enviar texto arbitrario al panel desde un ordenador moderno, a
través del enlace serie DB37 existente, **sin modificar el hardware**.

No se suelda nada, no se parchea nada, y el código [original/](original/) se 
mantiene idéntico, byte a byte, tal como salió de la EPROM.

Este proyecto se desarrolla en **inglés**: es el idioma en el que estoy
acostumbrado a programar, y desarrollar en otros idiomas me resulta
inconsistente.

## The device

| | |
| --- | --- |
| Model | PAGE Ibérica PI-T100-M ("MAESTRA" — master unit) |
| Electronics | **Telenorma** (Telefonbau und Normalzeit, Frankfurt) — logo on the motherboard, EPROM labelled `TELENORMA 18 JUN 96` |
| Display | 16 characters, 5x7 dots each (80x7 = 560 dots), 100 mm high |
| CPU | OKI MSM80C85AH (Intel 8085 clone) |
| Firmware | 32 KiB M27256 EPROM, 12.3 KiB used, version `AK0052.139U1` |
| Peripherals | M81C55-5 (256 B RAM + I/O + timer), M82C51A-2 USART |
| Interface | DB37, 4 conductors, opto-isolated (6N139) |
| Power | 24 VDC |

Full details in [docs/device.md](docs/device.md).

## Spotted in the wild

These units were installed across Cercanías Madrid starting in the **mid-1990s**
and are being **phased out during the 2020s** as stations are refitted with
modern LED/LCD signage. Mounting varies by station: some are **hung from the
platform marquee/ceiling**, others are **mounted on top of a pole**. Photos
below (Google Maps contributions) show the "Vía" next-arrival displays in
service at several stations:

| | | |
| --- | --- | --- |
| [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWnuVKiA1CfmwZmsqmBYouK0si_PNXC_1U0SrNglIIoP5KZG6Z1GexpvDqZtwOC9uT73btR2eoEZ2aWrIWKo9QxQPs-N16P0gDpi3cy41LDFZbNhbBk1PE-Yz6G6Nhmb5ObGMoPb=w480" width="260" alt="Vía display at Tres Cantos">](https://maps.app.goo.gl/SgD5UcXEt2scQfJP6) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWmoqYe0ipeDueShLFtyYjQjVWBriaWA0QRaPspo7n_r3Fz-qXHp38SmIWHZOgZ1MsfU3ut1e7HdkN6ZduYmL3LE5ZlVxx3j8Qsh8-ux2M7qVNr8HGspjrl_SVDUodYu_lAfnsVZLit2JfOq=w480" width="260" alt="Vía display at Pitis">](https://maps.app.goo.gl/4AjHZ317e6zEZWLX6) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWk4_tU2sE1bVB9q-qaFMDxpqdA9U9lIxhcIlpl-EgX4x--BPeS4z7z2sRtWZYjxWSkJsR0KsMC2LQUVfB8H8rL1Oyx6-gWMZ2_s3ort6AxnIwKep2PW3Nb7GRVJhsQlt8kzIRg=w480" width="260" alt="Vía display at Galapagar-La Navata">](https://maps.app.goo.gl/Z3yJGkftK173W16y9) |
| Tres Cantos | Pitis | Galapagar-La Navata |
| [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWk4tJf7YDnld6bDf3idtBx8zkhKSAuLuR9X5OKPpXxEIN6QJ6NmxOUSVi68oVhZzKOaNPMTqaWcdhC8w0gkpDABvV2MXC6QVSUTpgXJDViyaysEAjE9uInq-6oN6jEsYC9HU1FE=w480" width="260" alt="Vía display at Delicias">](https://maps.app.goo.gl/wM6Yq13YrQxSNZAG7) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWn8hKNYQuLIZa8VCoKam4xc6gy935AlYsH69e-by3c4ZkdvUhzFSQn45frfmGF6cQSo0SKHmEScAodaDCHz1cgGm9BaNnHH5aEMidmlVOjYCf--HT3oZyubCppA8_vkdR3dqbm3=w480" width="260" alt="Vía display at Cantoblanco Universidad">](https://maps.app.goo.gl/gkyVJaivqFrVrPYh8) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWksNsfaIbK51xFdfq2uRg9pGEUd41QC-cA2u6JiGHyDItdHZ6m-IvAlsGf-zE9AxOgDBnmAAhmwR8MbYs1D6YBGs0G0SPIZtCADX8jCFW0WC08OkxbCOTYv5HKZjAT_IzpjSh5iRg=w480" width="260" alt="Vía display at Collado Mediano">](https://maps.app.goo.gl/yveuFPEbi5sjoSSv5) |
| Delicias | Cantoblanco Universidad | Collado Mediano |

## Character font

All 256 glyphs of the panel's 5x7 font, extracted straight from the ROM and
rendered flip-dot style. See [docs/font.md](docs/font.md) for the derivation
and [tools/extract_font.py](tools/extract_font.py) (`--png`) to regenerate it.

![256-glyph flip-dot font map, 5x7 dots each, arranged 16 per row](docs/font-map.png)

## Status

| Area | State |
| --- | --- |
| ROM dump | done — **two independent reads, identical SHA-256**, provenance recorded |
| Memory map | done — [docs/memory-map.md](docs/memory-map.md) |
| I/O port map | done — all ports enumerated, most attributed — [docs/io-map.md](docs/io-map.md) |
| Character font | done — [docs/font.md](docs/font.md) |
| Serial link layer | done — **9600 8E1** default; full 300-9600 ladder mapped |
| DIP switches | port B fully attributed; port C bit 5 known; port A mostly open |
| Receive architecture | done — RST5.5 ISR, two handlers — [docs/serial-protocol.md](docs/serial-protocol.md) |
| Serial framing | both handlers mapped; per-command semantics open |
| DB37 pinout | not measured |
| Flip-dot drive | **serial shift register decoded** — clock, data, 16 selects, 74 bits/char — [docs/flipdot-interface.md](docs/flipdot-interface.md) |

The working queue is [docs/open-questions.md](docs/open-questions.md).

## Key findings so far

* The external link is a **9600 baud, 8 data bits, even parity, 1 stop bit**
  asynchronous serial line, opto-isolated, almost certainly RS-422/485 or a
  20 mA current loop. Both the frame format and the baud rate are DIP-switch
  selectable.
* The **font is fully decoded** and the code page identified as **IBM CP437**:
  256 glyphs of 5x7 at ROM `0x1C0D`, indexed directly by character code, one byte
  per column, bit 0 = top row. So `Ñ` is `0xA5` and `text.encode("cp437")` works.
  Two exceptions: `0x24` and `0x25` are accented capitals, not `$` and `%`.
* **The whole baud ladder is mapped**: dividers 20/40/80/160/320/640, an exact
  1:2:4:8:16:32 sequence giving 9600 down to 300. Port B also selects word length
  (bit 4) and parity (bits 3:2), so port B is fully attributed.
* **RST5.5 is the serial receive interrupt**, gated by DIP switch port C bit 5,
  dispatching to one of *two* receive protocols selected by port A bit 6:
  `0x08B7` simple delimited text, or `0x0ACB` a **28-command protocol** built on
  ASCII control codes with `STX`/`ETX` framing.
* **The unit replies `ACK` (0x06) or `NAK` (0x15) on ETX**, followed by its
  address and, on failure, a printable **error code** — so a rejection says why.
  Once the link is wired up this makes the rest of the protocol discoverable by
  experiment.
* Messages are **64 bytes at `0xE001`**; overflow is detected and NAKed rather
  than silently truncated. `CAN` clears the display, `DLE` resets the unit, and
  `BEL` + one byte selects one of 8 display effects.
* **DIP switch port A bits 4:0 are the display address**, and **port C bit 5 is
  the serial enable** — with it clear the unit runs standalone and never receives
  anything at all.
* **The command protocol is fully mapped and validated in emulation.** A unit is
  selected with `GS <addr>` (`0x7F` broadcasts), then accepts a small
  cursor-addressable terminal command set, and answers queries with real data:

```
GS '5' SUB           -> AK0052.139U1 + the unit's configuration block
GS '5' TAB           -> '00'                     (cursor row, column)
GS '5' STX HOLA ETX  -> ACK '5'                  (acknowledged, with address)
GS '9' SUB           -> nothing                  (wrong address, stays silent)
```
* `0x4A`/`0x4E` at `0x03A0` are an **ISR/main-loop mailbox**, not framing bytes.
* **The panel is driven serially, not as a matrix.** Each of the 16 characters is
  an independent module on a shared 26-conductor bus with its own select line;
  port C bit 4 is the shift clock and bit 5 the data, 74 bits per character.
  Validated by reconstructing the glyphs from the emulated bit stream.
* The firmware is **designed to run standalone**. Expansion-board memory is
  reached through a paged window at `0xE000` selected by I/O port `0xE7`, and all
  46 banked accesses funnel through three accessors that check for the board and
  fall back to local RAM. No site waits, retries or errors out; one even
  substitutes a DIP switch for a value the board would have supplied.
* The "MAESTRA" (master) label plus a switch-selectable configuration suggests
  the *serial* protocol carries a display address — a separate question from the
  expansion board, and still open.

## Layout

```
original/    Pristine captures from the device. Never edit. Checksummed.
docs/        Reverse-engineered understanding - the real product
hardware/    Chips, PCBs, connectors, PSU, photos
ghidra/      symbols.csv (source of truth for names) + scripts + frozen exports
src/         Annotated 8085 assembly, aiming at a byte-exact rebuild
tools/       Standalone scripts (font extraction, ROM inspection)
notes.md     Dated lab notebook - every experiment, including the failures
```

## Quickstart

```sh
make                # list targets
make check          # verify the ROM dump and the symbol table
make font           # render all 256 glyphs
make font-map       # regenerate docs/font-map.png
make verify         # reassemble src/ and prove it matches the ROM (once src/ exists)
```

To browse the firmware without installing Ghidra, grep the frozen exports in
[ghidra/export/](ghidra/export/). To do real work on it, see
[ghidra/README.md](ghidra/README.md) — the import settings matter, because every
address in the docs depends on them.

## Conventions

* Every factual claim in `docs/` carries a confidence mark: **[C]** confirmed,
  **[L]** likely, **[?]** guess. See [docs/README.md](docs/README.md).
* Firmware locations are cited by **address**, not by Ghidra's autogenerated
  names, so the documentation survives a re-import.
* Memory addresses and I/O ports are different spaces on the 8085 and are always
  labelled as such.

## Safety

Flip-dot coils expect **short pulses**. Leaving one energised will overheat and
can destroy it — relevant to any experiment that drives the panel directly
instead of through this firmware. See
[docs/flipdot-interface.md](docs/flipdot-interface.md).

## Licence

[MIT](LICENSE) for the documentation, tools and any code here. The original
firmware in [original/](original/) is the property of its authors and is included
solely for interoperability research and preservation.

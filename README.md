# renfe-flipdot-re

Reverse engineering of a **PAGE Ibérica PI-T100-M** flip-dot platform display,
as used in RENFE Cercanías stations, for preservation and play.

**Goal:** drive arbitrary text onto the panel from a modern host, over the
existing DB37 serial link, with **no hardware modification**.

Nothing is soldered, nothing is patched, and [original/](original/) stays
byte-for-byte as it came off the EPROM.

## The device

| | |
| --- | --- |
| Model | PAGE Ibérica PI-T100-M ("MAESTRA" — master unit) |
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
modern LED/LCD signage. Photos below (Google Maps contributions) show the
"Vía" next-arrival displays in service at three stations:

| | | |
| --- | --- | --- |
| [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWmK28mfNc-ZA2jDGY3_7ZhZ-RhqbmH_yfOD1WHwSFxA_tJVUUlU_ls6Ep7V4JCouZTkuWMBHcqoRIkwaohcH3whRLMUvcglht9Su8y0h8GQKbfWPJvw-DDx9LkxyOrs6nd_wTJF=w480" width="260" alt="Vía display at Pitis">](https://maps.app.goo.gl/f3oppjUooByw9qFM7) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWmoqYe0ipeDueShLFtyYjQjVWBriaWA0QRaPspo7n_r3Fz-qXHp38SmIWHZOgZ1MsfU3ut1e7HdkN6ZduYmL3LE5ZlVxx3j8Qsh8-ux2M7qVNr8HGspjrl_SVDUodYu_lAfnsVZLit2JfOq=w480" width="260" alt="Vía display at Pitis">](https://maps.app.goo.gl/4AjHZ317e6zEZWLX6) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWnFynRZ11MLTaY8K9TzJk2z0aSJ831N8SqPhOjEIxtrfgf-Mfz92unTC7gmGX9oP4FgJZ8GNllLy0VE3X7fYrkc4ECUq4kAXdlF6TG5g-Rf43uSnL-XJpBw6PDVIfL4qK2YqBuochHzyeh2=w480" width="260" alt="Vía display at Pitis">](https://maps.app.goo.gl/j4Zr16FniYCeHfKF9) |
| Pitis | Pitis | Pitis |
| [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWkM3lz6eDJbIT53HVNaMjq8jxknHYzRm0EqoFSJeOIwr2WMZQfsT3Pc9DfD0-6UH28eMF2UHKK1BKV_AsXw_i575gAJhUaQozz5o0WyDWDGYJE2ylwdfJYVeaBayZMwZXN5qSS6=w480" width="260" alt="Vía display at Cantoblanco Universidad">](https://maps.app.goo.gl/bC8dspu8YQ9TEN98A) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWn8hKNYQuLIZa8VCoKam4xc6gy935AlYsH69e-by3c4ZkdvUhzFSQn45frfmGF6cQSo0SKHmEScAodaDCHz1cgGm9BaNnHH5aEMidmlVOjYCf--HT3oZyubCppA8_vkdR3dqbm3=w480" width="260" alt="Vía display at Cantoblanco Universidad">](https://maps.app.goo.gl/gkyVJaivqFrVrPYh8) | [<img src="https://lh3.googleusercontent.com/gps-cs-s/AHRPTWksNsfaIbK51xFdfq2uRg9pGEUd41QC-cA2u6JiGHyDItdHZ6m-IvAlsGf-zE9AxOgDBnmAAhmwR8MbYs1D6YBGs0G0SPIZtCADX8jCFW0WC08OkxbCOTYv5HKZjAT_IzpjSh5iRg=w480" width="260" alt="Vía display at Collado Mediano">](https://maps.app.goo.gl/yveuFPEbi5sjoSSv5) |
| Cantoblanco Universidad | Cantoblanco Universidad | Collado Mediano |

## Status

| Area | State |
| --- | --- |
| ROM dump | done — verified, checksummed, provenance recorded |
| Memory map | done — [docs/memory-map.md](docs/memory-map.md) |
| I/O port map | done — all ports enumerated, most attributed — [docs/io-map.md](docs/io-map.md) |
| Character font | done — [docs/font.md](docs/font.md) |
| Serial link layer | done — **9600 8E1** default; full 300-9600 ladder mapped |
| DIP switches | port B fully attributed; port C bit 5 known; port A mostly open |
| Receive architecture | done — RST5.5 ISR, two handlers — [docs/serial-protocol.md](docs/serial-protocol.md) |
| Serial framing | **not started — this is the blocker** |
| DB37 pinout | not measured |
| Flip-dot drive | partially traced |

The working queue is [docs/open-questions.md](docs/open-questions.md).

## Key findings so far

* The external link is a **9600 baud, 8 data bits, even parity, 1 stop bit**
  asynchronous serial line, opto-isolated, almost certainly RS-422/485 or a
  20 mA current loop. Both the frame format and the baud rate are DIP-switch
  selectable.
* The **font is fully decoded**: 256 glyphs of 5x7 at ROM `0x1C0D`, indexed
  directly by character code, one byte per column, bit 0 = top row.
* **The whole baud ladder is mapped**: dividers 20/40/80/160/320/640, an exact
  1:2:4:8:16:32 sequence giving 9600 down to 300. Port B also selects word length
  (bit 4) and parity (bits 3:2), so port B is fully attributed.
* **RST5.5 is the serial receive interrupt**, gated by DIP switch port C bit 5,
  dispatching to one of *two* receive handlers (`0x08B7`, `0x0ACB`) — the unit
  implements two protocols or two link roles. RST7.5 is a tick and watchdog.
* `0x4A`/`0x4E` at `0x03A0` are an **ISR/main-loop mailbox**, not framing bytes.
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

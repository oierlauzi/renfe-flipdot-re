# Lab notebook

Chronological, newest first. One entry per session. Record what was *tried*, not
just what worked — a hypothesis that failed is the most useful thing in the file
six months later, because it stops you re-running it.

Settled conclusions belong in [docs/](docs/); this file is the trail that leads
to them.

Entry template:

```
## YYYY-MM-DD — short title

**Did:**
**Found:**
**Concluded:** (with a [C]/[L]/[?] mark)
**Next:**
```

---

## 2026-08-05 — Project structure, and a first pass over the ROM

**Did:** Reorganised the repository (see [README.md](README.md#layout)) and read
the Ghidra export looking for peripheral accesses.

**Found:**

* 322 `IN`/`OUT` instructions across ports `0x80-0x81`, `0xA0-0xA5`,
  `0xC0-0xCF`, `0xDE`, `0xE0-0xED`. Everything is in 8085 I/O space; nothing is
  memory-mapped except the 81C55's RAM at `0xA000`.
* `0x0443` is a textbook 8251A wake-up: 3 dummy writes, reset `0x40`, mode
  `0x7E`, command `0x37`. Mode `0x7E` decodes to 8 data bits / even parity /
  1 stop / x16 clock.
* `0x03F2` initialises **two** more 81C55-shaped register groups at `0xE0` and
  `0xE8`, and `IN 0xE7` at `0x040F` is a presence check that skips the second
  group when it reads `0xFF`.
* The 81C55's ports A/B/C are only ever *read* → all three are the DIP switch
  banks. Port B bits 7:5 index a 6-way baud table at `0x0474`; port C bit 5
  picks between two startup paths at `0x0430`.
* Only `0x0000-0x3047` of the 32 KiB EPROM is programmed; the rest is `0xFF`.
* Region `0x1C0D-0x210C` is a 5x7 font: 256 entries x 5 bytes, column-major,
  bit 0 = top row. Verified by rendering — `'0'`, `'A'`, `'B'` come out right,
  and bit 7 is clear in all 1280 bytes.
* Chassis sticker: **PAGE Ibérica S.A., PI-T100-M, MAESTRA**, "1 línea de texto,
  módulos de PLACA MÓVIL de 100 mm".

**Concluded:**

* Link layer is **9600 8E1** **[L on the 9600, C on the 8E1]** — the 9600
  depends on the 81C55 TIMER IN being 3.072 MHz, which has not been measured.
* Font format **[C]**.
* Master/slave with a switch-set display address **[L]** — inferred from
  "MAESTRA" plus the second-board probe, not yet from the protocol code.

**Next:** the application-layer framing. Follow the `IN 0x80` sites back to
their buffer and forward to the state machine at `0x03A0`, which compares
against the constants `0x4A`/`0x4E`. Also fill in the five missing rows of the
baud table — pure listing reading, no hardware needed.

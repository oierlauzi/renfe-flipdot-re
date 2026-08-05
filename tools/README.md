# Tools

Standalone scripts. No dependencies beyond the Python standard library — this
project should still work in ten years.

Run them from the repository root; they default to `original/firmware.bin`.

| Script | Purpose |
| --- | --- |
| [rom_info.py](rom_info.py) | Integrity check, used/erased extent, region cross-check, printable strings |
| [extract_font.py](extract_font.py) | Render or export the 5x7 glyph table |

```sh
make check                          # rom_info.py + symbol table validation
make font                           # every glyph
python3 tools/extract_font.py --ascii --range 0x41-0x5A
python3 tools/extract_font.py --json > font.json
```

Both scripts **re-derive their assumptions from the ROM and fail loudly** if the
image or the documented layout has changed. That is deliberate: they double as
regression tests on the claims in [../docs/](../docs/). If `rom_info.py` reports
a region whose contents no longer match, either the dump changed or a document
is wrong.

## Wanted

Ideas in rough order of value to the project goal:

* **`send_frame.py`** — build and transmit a candidate protocol frame over a
  USB-serial adapter, once [../docs/serial-protocol.md](../docs/serial-protocol.md)
  has a framing hypothesis. Should log every attempt in a form that can be
  pasted into the protocol scratchpad table.
* **An 8085 emulator harness** — run `firmware.bin` with stub 8251/81C55
  devices, feed candidate frames into the emulated UART, and watch the writes to
  I/O `0xE1`/`0xE2`/`0xE3`. This is the highest-value tool in the list: it makes
  protocol discovery a fast software loop instead of a slow hardware one, and it
  cannot damage the panel. A few hundred lines is enough — the 8085 has 246
  opcodes and the firmware uses a fraction of them.
* **`render.py`** — turn a string into the 80x7 bitmap the panel would show,
  using the extracted font. Useful for predicting what a frame *should* produce
  before sending it.

# Tools

Standalone scripts. No dependencies beyond the Python standard library — this
project should still work in ten years.

Run them from the repository root; they default to `original/firmware.bin`.

| Script | Purpose |
| --- | --- |
| [rom_info.py](rom_info.py) | Integrity check, used/erased extent, region cross-check, printable strings |
| [extract_font.py](extract_font.py) | Render or export the 5x7 glyph table |
| [emu8085.py](emu8085.py) | Intel 8085 CPU core; machine-agnostic, memory and I/O via callbacks |
| [pit100_sim.py](pit100_sim.py) | Runs the real firmware against stub peripherals |
| [pit100.py](pit100.py) | **Host-side driver**: the command protocol, ready for a serial port |

## pit100.py — the driver

The deliverable the project is aiming at, minus the physical layer. It implements
the protocol from [../docs/serial-protocol.md](../docs/serial-protocol.md) and is
tested against the emulator, so it can be developed and proven with no hardware:

```sh
PYTHONPATH=tools python3 tools/pit100.py --sim --address 5 --identify --text HOLA
```

```
version : AK0052.139U1
config  : 00 00 01 00 00 2f 30 00 01 00 ff 40 01 00 40 ...
message : 'HOLA' -> ACK
```

Swap `SimTransport` for `SerialTransport` and the same code drives the real unit.

It handles three things a naive implementation gets wrong:

* **Filters the status frame.** The display transmits `CAN CAN RS NAK <addr>`
  unprompted, forever, so replies arrive interleaved with it.
* **Guards the font.** Refuses `$` and `%` (ROM `0x24`/`0x25` hold accented
  capitals), and refuses uppercase `Á Í Ó Ú` which CP437 lacks — with `fold=True`
  to drop those accents instead.
* **Guards the buffer.** Rejects messages over 64 bytes rather than letting the
  unit NAK with error `0x30`.

## pit100_sim.py

Executes `original/firmware.bin` with emulated 8251A and 81C55 devices, so
protocol hypotheses can be tested in a second with no hardware and no risk to
the panel.

```sh
# boot and report where the firmware settles
PYTHONPATH=tools python3 tools/pit100_sim.py --boot

# identify: address 5 on port A, command protocol (bit 6), serial enabled
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xFF \
    --xram board --send '1D 35 1A'
```

**Port C bit 5 must be set or the unit is deaf.** The RST5.5 handler returns
immediately when it is clear, so no command produces any reply — see
[../docs/dip-switches.md](../docs/dip-switches.md). Keep `--port-c 0xFF` unless
you are deliberately testing standalone mode.

Handler B also requires the unit to be **addressed first**: prefix commands with
`GS <0x30|address>`, or `1D 7F` to broadcast.

The `--xram` flag picks what answers at `0xE000`: `none`, a fitted expansion
`board`, or `alias` (the 81C55 reachable there through incomplete address
decoding). That is an open hardware question, so the model offers all three
rather than guessing — and reports any write that lands in unbacked memory.

It has already reproduced the documented 8251A init (mode `0x7E`, command
`0x37`) from a cold boot, which is the main evidence that the model is faithful.

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

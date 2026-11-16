# Tools

Standalone scripts. Run them from the repository root. They default to
`original/firmware.bin`. Only the ones that open a serial port need anything
installed, and that is `pyserial`.

| Script | Purpose |
| --- | --- |
| [say.py](say.py) | **Put text on the panel.** The everyday one |
| [bringup.py](bringup.py) | Staged first power-up of an unknown unit |
| [pit100.py](pit100.py) | The host driver: the whole command set |
| [pit100_sim.py](pit100_sim.py) | Runs the real firmware against emulated peripherals |
| [emu8085.py](emu8085.py) | The 8085 CPU core, with memory and I/O as callbacks |
| [panel_view.py](panel_view.py) | Decodes the flip-dot bit stream and draws the panel |
| [disasm.py](disasm.py) | Disassembler, cross-references, and the rebuildable listing |
| [extract_font.py](extract_font.py) | Render or export the glyph tables |
| [rom_info.py](rom_info.py) | Integrity check, used extent, strings |

`say.py` and `bringup.py` are covered in
[../docs/2-talking-to-it.md](../docs/2-talking-to-it.md). The rest are below.

## pit100.py

Implements the protocol from [../docs/3-the-commands.md](../docs/3-the-commands.md)
against either a serial port or the emulator, so it can be developed with no
hardware:

```sh
PYTHONPATH=tools python3 tools/pit100.py --sim --address 5 --identify --text HOLA
```

Three things it handles that a naive implementation gets wrong:

* **Drains replies until the line goes quiet** rather than counting bytes, because
  reply lengths vary.
* **Guards the font.** Refuses `$` and `%`, which are `ñ` and `Ñ` on this panel,
  and refuses the uppercase accents CP437 lacks. `fold=True` drops them instead.
* **Guards the buffer.** Rejects messages over 64 bytes rather than letting the
  unit refuse them.

## pit100_sim.py

Executes the real ROM with emulated serial and I/O chips, so a protocol idea can
be tested in a second with no risk to the panel.

```sh
PYTHONPATH=tools python3 tools/pit100_sim.py --boot
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xFF \
    --xram board --send '1D 35 1A'
```

Two switch settings matter. **Port C bit 5 must be set** or reception goes to
the undecoded binary protocol. And the unit must be **addressed first**, so
prefix with `GS <0x30|address>` or `1D 7F` to broadcast.

`--xram` picks what answers at `0xE000`: `none`, a fitted expansion `board`, or
`alias`. The real unit renders text, so its window is backed, but which of the
two it is was never determined.

## panel_view.py

Watches the emulated firmware drive the display ports, recovers the clocked
data, and reconstructs the glyphs:

```sh
PYTHONPATH=tools python3 tools/panel_view.py --text "HOLA MUNDO"
PYTHONPATH=tools python3 tools/panel_view.py --fill
```

Two settings matter: `--port-b` needs bit 0 clear or the pad character is a
solid block, and the command stream must end in `RS` or a selected unit never
repaints.

## disasm.py

```sh
python3 tools/disasm.py 0x1440-0x1470     # disassemble a range
python3 tools/disasm.py --xref 0xA00A     # who touches this address or port
python3 tools/disasm.py --calls 0x136A    # who calls this routine
make disasm                               # rewrite src/firmware.asm
```

The cross-reference scan disassembles linearly from `0x0000`, so it can lose
sync inside the font table. It checks itself against a raw byte search and says
so when the two disagree.

## extract_font.py and rom_info.py

```sh
make check                                      # rom_info.py plus symbol checks
make font                                       # every glyph
python3 tools/extract_font.py --ascii --range 0x41-0x5A
python3 tools/extract_font.py --json > font.json
python3 tools/extract_font.py --font 4x7        # or --alt for the 3x5 table
```

Both re-derive their assumptions from the ROM and fail loudly if the image or
the documented layout has changed, so they double as regression tests on
[../docs/](../docs/).

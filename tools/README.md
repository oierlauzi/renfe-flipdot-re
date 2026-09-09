# Tools

Standalone scripts. Only `pyserial` is needed, and only for the ones that open a
serial port; everything else is the Python standard library.

Run them from the repository root; they default to `original/firmware.bin`.

| Script | Purpose |
| --- | --- |
| [say.py](say.py) | **Put text on the panel.** The everyday front end |
| [bringup.py](bringup.py) | Staged first power-up: adapter, quiet line, line settings, address, demo |
| [pit100.py](pit100.py) | Host-side driver: the whole command protocol |
| [pit100_sim.py](pit100_sim.py) | Runs the real firmware against emulated peripherals |
| [emu8085.py](emu8085.py) | Intel 8085 CPU core; machine-agnostic, memory and I/O via callbacks |
| [panel_view.py](panel_view.py) | Decodes the flip-dot serial stream and renders the panel |
| [disasm.py](disasm.py) | 8085 disassembler with symbol annotation, cross-references, and the matching listing |
| [extract_font.py](extract_font.py) | Render or export the glyph tables |
| [rom_info.py](rom_info.py) | Integrity check, used and erased extent, region cross-check, strings |

## say.py — text on the panel

```sh
tools/say.py CHAMARTIN            # one line and exit
tools/say.py                      # type lines; each appears on the panel
echo ATOCHA | tools/say.py        # or from a pipe
tools/say.py --fill               # every dot on
tools/say.py --clear              # every dot off
tools/say.py --cycle 3            # fill and clear repeatedly, to find stuck dots
tools/say.py --identify           # version and config, as a health check
```

Defaults are this unit's configuration — `/dev/ttyUSB0`, 1200 7E1, address 1 —
overridable with `--port/--baud/--bits/--parity/--address`, or `--sim` to drive
the emulator instead.

It deselects before it starts and after every line. The first matters because a
unit left mid-command consumes the next byte as a parameter, putting the text
that follows at a stale write pointer; the second because the repaint is held off
while a unit is selected. It also folds the accents CP437 lacks on uppercase
vowels, maps `ñ`/`Ñ` onto the `0x24`/`0x25` duplicates a 7-bit link needs, and
refuses text longer than the 64-byte buffer.

## bringup.py — first power-up

```sh
python3 tools/bringup.py --port /dev/ttyUSB0          # every stage, in order
python3 tools/bringup.py --port /dev/ttyUSB0 --stage line
make bringup-selftest                                 # against the emulator
```

Each stage prints what a failure means rather than just failing. The `line` stage
sweeps baud and character format sending a broadcast `SUB`, looking for the known
answer `AK0052.139U1`, which is what makes a unit with unknown switch settings
tractable. See [../docs/controller.md](../docs/controller.md#the-tools).

## pit100.py — the driver

Implements the protocol from
[../docs/serial-protocol.md](../docs/serial-protocol.md) against either a serial
port or the emulator, so it can be developed and proven with no hardware:

```sh
PYTHONPATH=tools python3 tools/pit100.py --sim --address 5 --identify --text HOLA
```

```
version : AK0052.139U1
config  : 00 00 01 00 00 2f 30 00 01 00 ff 40 01 00 40 ...
message : 'HOLA' -> ACK
```

Three things it handles that a naive implementation gets wrong:

* **Drains replies to an idle gap** rather than reading a fixed count. Reply
  lengths vary — the `SUB` preamble, the conditional address byte and the
  conditional error detail all move — so counting leaves the remainder buffered
  and every later read falls one reply behind.
* **Guards the font.** Refuses `$` and `%`, which are `ñ` and `Ñ` on this panel,
  and refuses uppercase `Á Í Ó Ú` which CP437 lacks, with `fold=True` to drop
  those accents instead.
* **Guards the buffer.** Rejects messages over 64 bytes rather than letting the
  unit `NAK`.

## pit100_sim.py

Executes `original/firmware.bin` with emulated 8251A and 81C55 devices, so
protocol hypotheses can be tested in a second with no risk to the panel.

```sh
# boot and report where the firmware settles
PYTHONPATH=tools python3 tools/pit100_sim.py --boot

# identify: address 5 on port A, command protocol on bit 6
PYTHONPATH=tools python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xFF \
    --xram board --send '1D 35 1A'
```

Two switch settings matter. **Port C bit 5 must be set** for the ASCII command
protocol; clear routes reception to handler C, which speaks something else
entirely and will not answer these commands. And handler B requires the unit to be
**addressed first**: prefix with `GS <0x30|address>`, or `1D 7F` to broadcast.

`--xram` picks what answers at `0xE000`: `none`, a fitted expansion `board`, or
`alias`, the 81C55 reachable there through incomplete address decoding. The real
unit renders text, so its window is backed — which of the latter two it is has not
been determined. The model reports any write that lands in unbacked memory.

## panel_view.py — a virtual flip-dot panel

Watches the emulated firmware drive port `0xE3`, recovers the clocked serial data,
and reconstructs the 5x7 glyphs:

```sh
PYTHONPATH=tools python3 tools/panel_view.py --fill
PYTHONPATH=tools python3 tools/panel_view.py --text "HOLA MUNDO"
```

```
#...#  .###.  #....  .###.  #...#  #...#  #...#  ####.  .###.
#...#  #...#  #....  #...#  ##.##  #...#  #...#  #...#  #...#
#...#  #...#  #....  #...#  #.#.#  #...#  ##..#  #...#  #...#
#####  #...#  #....  #####  #.#.#  #...#  #.#.#  #...#  #...#
#...#  #...#  #....  #...#  #...#  #...#  #..##  #...#  #...#
#...#  #...#  #....  #...#  #...#  #...#  #...#  #...#  #...#
#...#  .###.  #####  #...#  #...#  .###.  #...#  ####.  .###.
```

Nine glyphs for ten characters: the space produces no frame because the position
already held a space, which is the differential update doing its job.

Two settings matter. `--port-b` must have bit 0 clear or the pad character is a
solid block, and the command stream must end in `RS` or a selected unit never
repaints — see
[../docs/serial-protocol.md](../docs/serial-protocol.md#the-repaint-interlock-rs-is-not-optional).

## disasm.py — reading the ROM

Four modes, all annotating with the names in
[../ghidra/symbols.csv](../ghidra/symbols.csv):

```sh
python3 tools/disasm.py 0x1440-0x1470     # disassemble a range
python3 tools/disasm.py --xref 0xA00A     # who touches this address or port
python3 tools/disasm.py --calls 0x136A    # who calls this routine
python3 tools/disasm.py --asm             # the full matching listing
```

`make disasm` writes the last of those to [../src/firmware.asm](../src/firmware.asm),
which reassembles byte-identically to the ROM.

The xref scan disassembles linearly from `0x0000`, so it can lose sync inside the
font table and other data. It cross-checks against a raw byte search and says so
when the two disagree, rather than quietly under-reporting.

## extract_font.py and rom_info.py

```sh
make check                          # rom_info.py + symbol table validation
make font                           # every glyph
python3 tools/extract_font.py --ascii --range 0x41-0x5A
python3 tools/extract_font.py --json > font.json
python3 tools/extract_font.py --font 4x7        # or 3x5
```

Both re-derive their assumptions from the ROM and fail loudly if the image or the
documented layout has changed, so they double as regression tests on the claims in
[../docs/](../docs/).

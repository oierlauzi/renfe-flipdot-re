# Ghidra workspace

## Why the project directory is not committed

A Ghidra project (`*.rep/`, `*.gzf`) is a binary blob: it cannot be diffed,
reviewed or merged, and it silently becomes the only copy of your analysis. So
it is gitignored, and the analysis is kept in reviewable text instead:

| File | Role |
| --- | --- |
| [symbols.csv](symbols.csv) | **Source of truth** for every name and comment |
| [scripts/apply_symbols.py](scripts/apply_symbols.py) | Pushes `symbols.csv` into an open program (idempotent) |
| [export/](export/) | Frozen listing + decompiler output, for grepping |

Workflow: name things in Ghidra → write the good names into `symbols.csv` →
commit the CSV. If the project is ever lost, re-import and re-run the script.

## Import settings

These must match or every address in the docs shifts:

| Setting | Value |
| --- | --- |
| File | `original/firmware.bin` |
| Format | Raw Binary |
| Language | `8085:LE:16:default` (Intel 8085) |
| Base address | `0x0000` |
| Block name | anything; the exports use `ram` |

Ghidra names the single flat block `ram`, which is why exported lines read
`rom:ram:0043`. Listing addresses are plain ROM offsets — see
[../docs/memory-map.md](../docs/memory-map.md).

Do **not** let auto-analysis run over `0x3048-0x7FFF`; it is erased `0xFF` and
disassembles into thousands of bogus `RST 7` instructions.

## Applying the symbols

From the GUI: Window → Script Manager → add `ghidra/scripts` to the script
path → run `apply_symbols.py`.

Headless:

```sh
analyzeHeadless /tmp/ghidra-proj flipdot -process firmware.bin \
    -scriptPath ghidra/scripts -postScript apply_symbols.py
```

## Refreshing the exports

The files in [export/](export/) are snapshots, regenerable at any time. They are
committed only so the firmware can be grepped and diffed without launching
Ghidra.

* `firmware.lst` — File → Export Program → Format: ASCII, with bytes + comments
* `firmware.c`   — decompiler output for all functions

Regenerate both after any significant naming pass, and commit them in a separate
commit from the `symbols.csv` change so the diff stays readable.

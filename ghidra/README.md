# Ghidra workspace

A Ghidra project is a binary blob. It cannot be diffed, reviewed or merged, and
it quietly becomes the only copy of your analysis. So it is gitignored, and the
analysis lives in text instead:

| File | Role |
| --- | --- |
| [symbols.csv](symbols.csv) | **The source of truth** for every name and comment |
| [scripts/apply_symbols.py](scripts/apply_symbols.py) | Pushes it into an open program. Safe to re-run |

The loop is: name things in Ghidra, write the good names into `symbols.csv`,
commit the CSV. If the project is ever lost, re-import and re-run the script.
`make symbols-check` validates the file.

## Import settings

These must match, or every address in the documentation shifts.

| Setting | Value |
| --- | --- |
| File | `original/firmware.bin` |
| Format | Raw Binary |
| Language | `8085:LE:16:default` |
| Base address | `0x0000` |

Listing addresses are then plain ROM offsets.

Do **not** let auto-analysis run over `0x3048-0x7FFF`. It is erased and
disassembles into thousands of bogus instructions.

## Applying the symbols

From the GUI: Window, Script Manager, add `ghidra/scripts` to the script path,
run `apply_symbols.py`.

Headless:

```sh
analyzeHeadless /tmp/ghidra-proj flipdot -process firmware.bin \
    -scriptPath ghidra/scripts -postScript apply_symbols.py
```

To read the firmware without Ghidra at all, use
[../src/firmware.asm](../src/firmware.asm) or `tools/disasm.py`.

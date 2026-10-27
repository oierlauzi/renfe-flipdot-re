# Documentation

Read in order. Each file assumes the ones before it.

| | File | Contents |
| --- | --- | --- |
| 1 | [1-the-device.md](1-the-device.md) | What the unit is, what it does and what it is made of |
| 2 | [2-talking-to-it.md](2-talking-to-it.md) | Wiring a modern host to it, and the scripts that drive it |
| 3 | [3-the-commands.md](3-the-commands.md) | The command set, as a reference |
| 4 | [4-inside-the-firmware.md](4-inside-the-firmware.md) | Memory, I/O ports and the configuration switches |
| 5 | [5-the-panel.md](5-the-panel.md) | How a character becomes moving dots, and the font |

Files 1 to 3 are enough to put text on the panel. Files 4 and 5 are for working
on the firmware itself.

Photographs and the parts list are in [../hardware/](../hardware/). The ESP32
firmware that replaces the sign's original controller is
[../controller/](../controller/).

## Conventions

* Memory addresses are written `0x1C0D`. I/O ports are always written as
  "I/O `0xE1`", because the 8085 keeps the two in separate spaces.
* Bit 0 is the least significant bit.
* Firmware locations are given by address as well as by name, so the text still
  works after a fresh Ghidra import. The names come from
  [../ghidra/symbols.csv](../ghidra/symbols.csv) and appear in
  [../src/firmware.asm](../src/firmware.asm).
* Everything stated here was either measured on the unit or read out of the ROM.
  The few things that are still guesses are collected in [../TODO.md](../TODO.md)
  and nowhere else.

# Documentation index

What the device is and how it works. The disassembly in [../src/](../src/) is the
evidence; this is the result.

## Documents

| File | Contents |
| --- | --- |
| [device.md](device.md) | What the unit is: PAGE Ibérica PI-T100-M, its labels, geometry and place in a station |
| [serial-protocol.md](serial-protocol.md) | The external link: physical layer, framing, and the full command set |
| [controller.md](controller.md) | Driving it from a PC: wiring, USB-RS-422 adapters, and the tools in this repository |
| [dip-switches.md](dip-switches.md) | The three switch banks and what each bit configures |
| [io-map.md](io-map.md) | Every 8085 I/O port and the device behind it |
| [memory-map.md](memory-map.md) | Address space, ROM layout, interrupt vectors, RAM variables |
| [font.md](font.md) | The three glyph tables and the code page |
| [flipdot-interface.md](flipdot-interface.md) | How a character in the buffer becomes moving dots |
| [references.md](references.md) | Datasheets and external material |

Physical description of the boards lives in [../hardware/](../hardware/). The
ESP32 firmware that replaces the sign's original upstream controller is
[../controller/](../controller/), documented in its own README. The chronological
record of how all this was worked out, including the readings that turned out to
be wrong, is [../notes.md](../notes.md).

## Conventions

* Addresses are hex with an explicit prefix: `0x1C0D` for memory, and I/O ports
  are always written as "I/O `0xE1`" to keep the two spaces apart.
* Firmware locations are cited by address as well as by name, so the text
  survives a re-import. Names come from [../ghidra/symbols.csv](../ghidra/symbols.csv)
  and appear in [../src/firmware.asm](../src/firmware.asm).
* Bit numbering is LSB = bit 0 throughout, matching the datasheets.
* Statements are made plainly when they are established. Where something is
  inferred rather than observed, the text says so in words.

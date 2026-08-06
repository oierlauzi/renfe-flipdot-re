# Documentation index

Reverse-engineered *understanding* of the device. This is the real product of
the project — the disassembly is only the evidence.

Every document uses the same confidence key, and every claim carries one:

| Mark | Meaning |
| --- | --- |
| **[C]** | Confirmed — traced in the firmware and cross-checked against a datasheet, or measured on the hardware |
| **[L]** | Likely — consistent with the evidence, one step of inference away |
| **[?]** | Guess — recorded so it can be tested, not relied upon |

Never silently upgrade a mark. When something moves from **[L]** to **[C]**, say
in [../notes.md](../notes.md) what the evidence was.

## Documents

| File | Contents |
| --- | --- |
| [device.md](device.md) | What the unit is: PAGE Ibérica PI-T100-M, labels, geometry |
| [memory-map.md](memory-map.md) | Address space, ROM layout, vectors, RAM variables |
| [io-map.md](io-map.md) | Every 8085 I/O port and the device behind it |
| [serial-protocol.md](serial-protocol.md) | **The goal.** Physical, link and application layer |
| [controller.md](controller.md) | Planning the host side: interface hardware, sourcing, gotchas |
| [dip-switches.md](dip-switches.md) | The three switch banks and what they configure |
| [font.md](font.md) | The 5x7 glyph table format and code point coverage |
| [flipdot-interface.md](flipdot-interface.md) | How dots actually get flipped |
| [open-questions.md](open-questions.md) | The working queue |
| [references.md](references.md) | Datasheets and external material |

## Conventions

* Addresses are hex with an explicit prefix: `0x1C0D` for memory, and I/O ports
  are always written as "I/O `0xE1`" to keep the two spaces apart.
* Firmware locations are cited by address (`0x03F2`), not by Ghidra's
  auto-generated `FUN_ram_03f2`, so the docs survive a re-import. Names come
  from [../ghidra/symbols.csv](../ghidra/symbols.csv).
* Bit numbering is LSB = bit 0 throughout, matching the datasheets.

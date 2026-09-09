# Memory map

The MSM80C85AH drives a 16-bit address space. Peripheral registers live in the
separate 8085 I/O space — see [io-map.md](io-map.md).

## Address space

| Range | Size | Contents |
| --- | --- | --- |
| `0x0000-0x7FFF` | 32 KiB | U4 M27256F1 EPROM; only `0x0000-0x3047` is programmed |
| `0x8000-0x9FFF` | 8 KiB | unmapped |
| `0xA000-0xA0FF` | 256 B | U2 M81C55-5 static RAM — the machine's only RAM |
| `0xA100-0xDFFF` | — | unmapped, or aliases of the RAM above |
| `0xE000-0xFFFF` | 8 KiB | paged window into expansion-board memory |

### The paged window at 0xE000

The `0xE000-0xFFFF` window lives on the expansion board, which is absent from
this unit. The page visible through it is selected by writing I/O port `0xE7`.
Reading `0xE7` returns `0xFF` when the board is missing, and the firmware then
bypasses the window and accesses local RAM directly — see
[io-map.md](io-map.md#0xe7--expansion-memory-bank-register).

This is why the Ghidra decompilation refers to `DAT_ram_e000`, `DAT_ram_e03b`,
`DAT_ram_e07d` and similar: they are banked addresses, not real locations in this
machine. Ghidra imports the image at `0x0000`, so listing addresses are ROM
offsets throughout, and it labels the single flat block `ram:`, which is why the
exported listing reads `rom:ram:0000`.

## ROM layout

| Range | Contents |
| --- | --- |
| `0x0000-0x003F` | 8085 hardware vectors; each is one `JMP` into the table below |
| `0x0040-0x0063` | Second-stage vector table, twelve `JMP`s; all but two go to `0x034B` |
| `0x0064-0x006F` | Version string `AK0052.139U1` |
| `0x0070-0x008B` | Build-time configuration block, reported by `SUB` |
| `0x0092-0x00D1` | `tbl_halve`, a 64-entry *i* → *i*/2 lookup |
| `0x00D2-0x00E5` | Reply prefixes and suffixes, `0xFF`-terminated |
| `0x00E6-0x025F` | 377 bytes of zero fill, reserved space ahead of a hand-placed `ORG` |
| `0x0260-0x1C0C` | Code |
| `0x1C0D-0x210C` | 5x7 glyph table, 256 entries — see [font.md](font.md) |
| `0x210D-0x3047` | Code and tables |
| `0x3048-0x7FFF` | Erased, `0xFF` fill |

### Interrupt vectors

Every 8085 vector jumps into a second, densely packed table at `0x0040`, and only
that table names the real handler. The second table lists the handlers in
`RST0-7 / TRAP / 5.5 / 6.5 / 7.5` order rather than the scattered order of the
hardware addresses.

| Vector | Address | Trampoline | Handler |
| --- | --- | --- | --- |
| RESET | `0x0000` | `DI` / `JMP 0x0040` | `0x034B` |
| RST1 | `0x0008` | `JMP 0x0043` | `0x034B` |
| RST2 | `0x0010` | `JMP 0x0046` | `0x034B` |
| RST3 | `0x0018` | `JMP 0x0049` | `0x034B` |
| RST4 | `0x0020` | `JMP 0x004C` | `0x034B` |
| TRAP | `0x0024` | `JMP 0x0058` | `0x034B` |
| RST5 | `0x0028` | `JMP 0x004F` | `0x034B` |
| RST5.5 | `0x002C` | `JMP 0x005B` | **`0x0260`** |
| RST6 | `0x0030` | `JMP 0x0052` | `0x034B` |
| RST6.5 | `0x0034` | `JMP 0x005E` | `0x034B` |
| RST7 | `0x0038` | `JMP 0x0055` | `0x034B` |
| RST7.5 | `0x003C` | `JMP 0x0061` | **`0x02A9`** |

Only two hardware interrupts are used; `0x034B` is the catch-all reset path.

* **RST5.5 → `0x0260`** is the serial receive interrupt. DIP switch port C bit 5
  chooses between the ASCII command handlers and a third binary handler, and
  `0xA00C` then selects handler A or handler B. See
  [serial-protocol.md](serial-protocol.md#receive-architecture).
* **RST7.5 → `0x02A9`** is the periodic tick. It decrements the watchdog at
  `0xA01E` and jumps to `fatal_or_init` when it expires, samples `0xDE` into a
  sticky status accumulator, debounces the clock board's keypad on `0xCD` over
  201 ticks, ages the display attributes, counts down `wait_ticks`, and drives the
  8085 SOD pin.

## RAM layout, `0xA000-0xA0FF`

Only the low 256 bytes exist. Startup clears either `0xF0` bytes from `0xA000`
(the normal path, `0x0459`) or `0x80` bytes (the alternate path, `0x0469`).

The stack pointer is set to `0xA0F3` at `0x034C` and grows down. The clearing
loop stops at `0xA0EF`, leaving the stack region untouched, which is also why
Ghidra shows phantom symbols at `0xA0ED`, `0xA0EF` and `0xA0F1` — those are stack
slots rather than variables.

Frequently referenced variables:

| Address | Meaning |
| --- | --- |
| `0xA003` / `0xA004` | 81C55 timer preload, LSB and MSB — the baud divider taken from the switches |
| `0xA005` | Cached 8251A mode word |
| `0xA009` | Panel family, from port C bits 4:0; selects the per-character driver |
| `0xA00A` | Character position within the line |
| `0xA00C` | Selects receive handler A or B; set at `0x0689` |
| `0xA012` | Mailbox between the RST5.5 ISR and the main loop; tokens `0x4A` and `0x4E`, swapped atomically at `0x03A0` |
| `0xA01B` / `0xA01C` | Tick-wait request flag and 16-bit countdown |
| `0xA01E` | Watchdog counter; reaching 1 jumps to `fatal_or_init` |
| `0xA024` | Sticky accumulator of the `0xDE` status input, reported by `BS` |
| `0xA039` | Handler C's staging buffer, replayed by the main loop |
| `0xA059` | Saved expansion memory page, restored after a banked access |
| `0xA05A` | Set inside the ISRs so the banked accessors save and restore the page |
| `0xA07C` / `0xA07E` | Handler C event flags and state; both gate the main loop |
| `0xA07F` | Byte count handler C staged at `0xA039` |

[../ghidra/symbols.csv](../ghidra/symbols.csv) is the complete and current list.

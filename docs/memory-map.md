# Memory map

The MSM80C85AH drives a 16-bit address space. Peripheral registers live in the
separate 8085 I/O space — see [io-map.md](io-map.md).

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Address space

| Range | Size | Contents | Conf. |
| --- | --- | --- | --- |
| `0x0000-0x7FFF` | 32 KiB | U4 M27256 EPROM (only `0x0000-0x3047` used) | **[C]** |
| `0x8000-0x9FFF` | 8 KiB | unmapped | **[L]** |
| `0xA000-0xA0FF` | 256 B | U2 M81C55-5 static RAM | **[C]** |
| `0xA100-0xDFFF` | — | unmapped / aliases of the above | **[?]** |
| `0xE000-0xFFFF` | 8 KiB | **Paged window into expansion-board memory** | **[C]** |

### The paged window at 0xE000

The `0xE000-0xFFFF` window is not present in this unit: it lives on the absent
expansion board, and the page visible through it is selected by writing I/O port
`0xE7`. Reading `0xE7` returns `0xFF` when the board is missing, and the firmware
then bypasses the window entirely and accesses local RAM directly. See
[io-map.md](io-map.md#0xe7--expansion-memory-bank-register-c).

This is why the Ghidra decompilation contains references to `DAT_ram_e000`,
`DAT_ram_e03b`, `DAT_ram_e07d` and friends: they are banked addresses, not real
locations in this machine.

Ghidra imports the image at `0x0000`, so listing addresses are ROM offsets
throughout. Ghidra labels the single flat block `ram:`, which is why the
exported listing reads `rom:ram:0000` — ignore the `ram` part.

## ROM layout

| Range | Contents | Conf. |
| --- | --- | --- |
| `0x0000-0x003F` | 8085 RST/TRAP vector table (8-byte slots) | **[C]** |
| `0x0040-0x0063` | Vector trampolines; all but three jump to `0x034B` | **[C]** |
| `0x0064-0x006F` | Version/ID string `AK0052.139U1` | **[C]** |
| `0x0070-0x00xx` | Small constant tables and default parameters | **[L]** |
| `0x00xx-0x1C0C` | Code | **[C]** |
| `0x1C0D-0x210C` | 5x7 glyph table, 256 entries — see [font.md](font.md) | **[C]** |
| `0x210D-0x3047` | Code and tables | **[C]** |
| `0x3048-0x7FFF` | Erased (`0xFF` fill) | **[C]** |

### Vector table

| Vector | Address | Trampoline | Target |
| --- | --- | --- | --- |
| RESET | `0x0000` | `DI` / `JMP 0x034B` | `0x034B` |
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

Only two hardware interrupts are actually used. `0x034B` is the catch-all
(reset/hang) path. **[C]**

- **RST5.5 → `0x0260`** — the handler begins `IN 0xA3`, so it is fed by the
  81C55 port C / timer domain. **[L]**
- **RST7.5 → `0x02A9`** — the handler begins `IN 0xCD`, tying it to the
  `0xC0` device. **[L]**

## RAM layout (`0xA000-0xA0FF`)

Only the low 256 bytes exist. Two clearing loops appear at startup: `0xF0`
bytes from `0xA000` on the normal path (`0x0459`) and `0x80` bytes on the
alternate path (`0x0469`), so `0xA0F0-0xA0FF` is likely stack.

Variables identified so far:

| Address | Meaning | Conf. |
| --- | --- | --- |
| `0xA003` | 81C55 timer preload LSB (baud divider, from switches) | **[C]** |
| `0xA004` | 81C55 timer preload MSB | **[C]** |
| `0xA005` | Cached 8251 mode word | **[C]** |
| `0xA012` | Interrupt-guarded state byte; polled against `0x4A`/`0x4E` at `0x03A0` | **[L]** |
| `0xA07C` | Status flags; bit 0 polled in the main loop | **[L]** |
| `0xA07E` | Status flags; bits 7:6 polled for `0b01` in the main loop | **[L]** |
| `0xA07F` | Loop/repeat count for the 0x61-iteration render loop at `0x03CE` | **[L]** |
| `0xA039-` | Buffer walked by that render loop | **[L]** |
| `0xA059` | Saved expansion memory page, for restore after a banked access | **[C]** |
| `0xA05A` | Re-entrancy flag; set to 1 inside the ISRs (`0x0271`, `0x0296`) so banked accessors save and restore the page | **[C]** |

The stack pointer initialisation and the full variable map are still to be
recovered; see [open-questions.md](open-questions.md).

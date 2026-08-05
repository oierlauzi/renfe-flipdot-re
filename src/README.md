# Annotated source

Intended home of a **matching disassembly**: hand-written 8085 assembly that
reassembles to a file byte-identical to
[../original/firmware.bin](../original/firmware.bin).

`make verify` proves it, by comparing SHA-256. Until the reassembly exists the
target simply reports that it has nothing to check.

## Why bother

A matching build is the standard proof-of-understanding in firmware RE: it makes
every claim falsifiable, because a wrong `equ`, a misread operand or a
mis-sized table shows up as a checksum mismatch instead of quietly living on in
a document. It also makes the ROM *modifiable* later, if the project ever goes
that way.

It is, however, **optional for the stated goal**. Controlling the display from
outside only needs
[../docs/serial-protocol.md](../docs/serial-protocol.md) to be right. Treat the
reassembly as a long-running background task, not a prerequisite: do it for the
regions that matter (the RX path, the render loop) and leave the rest as
`db` blocks.

## Current contents

| File | Status |
| --- | --- |
| [hardware.inc](hardware.inc) | Usable — I/O and memory equates |
| `firmware.asm` | Not written yet |

## Toolchain

[Macro Assembler AS](http://john.ccac.rwth-aachen.de:8000/as/) (`asl`) — it
supports the 8085 with Intel mnemonics, which is what Ghidra emits, so listings
transcribe with minimal editing.

```sh
# Debian/Ubuntu
sudo apt install asl
# or build from source; the binary is called `asl`
```

Alternatives if `asl` is unavailable: `naken_asm`, or SDCC's `sdas8085` (Zilog
mnemonics — avoid, it forces a translation pass on every line).

## Suggested skeleton

```asm
        cpu     8085
        page    0
        include "hardware.inc"

        org     ROM_BASE

; ---------------------------------------------------------------------------
reset:                                  ; 0x0000
        di
        jmp     fatal_or_init
        db      0FFh, 0FFh, 0FFh, 0FFh  ; pad to the RST1 slot
...

; Regions not yet transcribed stay as verbatim bytes so the image still
; matches. Shrink these as understanding grows.
        org     0300h
        binclude "../original/firmware.bin", 0300h, 004Bh
```

Two rules that keep a matching build honest:

1. Never "fix" what the original does, however odd. A matching build documents
   the ROM, it does not improve it.
2. Erased space (`0x3048-0x7FFF`) is `0FFh` fill, not zeros — emit it
   explicitly or the checksum will not match.

# Annotated source

**The matching disassembly exists and passes.** [firmware.asm](firmware.asm)
reassembles to a file **byte-identical** to
[../original/firmware.bin](../original/firmware.bin) over its whole used extent,
`0000h-3047h` (12 360 bytes):

```
$ make verify
   6670 lines source file
      0 errors
      0 warnings
build/firmware.p==>>build/firmware.bin (12360 Bytes)
MATCH - reassembly is byte-identical to original/firmware.bin over 0000-3047
```

The erased `0FFh` tail above `3047h` is not assembled; `make rom-check` asserts
it separately, which is cheaper and just as conclusive.

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
| [firmware.asm](firmware.asm) | **Verified byte-identical.** Generated; see below |
| [hardware.inc](hardware.inc) | I/O and memory equates, kept for hand-written work |

### How it is produced

`firmware.asm` is **generated from the ROM**, not transcribed by hand:

```sh
make disasm        # tools/disasm.py --asm > src/firmware.asm
make verify        # assemble it and compare against the ROM
```

That is a deliberate choice. A hand transcription of 12 KiB would take a long
time and introduce its own errors; generating it means the listing is always a
faithful decode, and the *interesting* work — naming things — accumulates in
[../ghidra/symbols.csv](../ghidra/symbols.csv) and flows into the listing on the
next `make disasm`.

Current state: **561 labels, 78 of them real names** from `symbols.csv`, the rest
auto-generated `LXXXX`. Every symbol added to that file turns one more `LXXXX`
into something meaningful, so the listing improves as the analysis does. Data
regions are declared in `DATA_REGIONS` in `tools/disasm.py`; getting one wrong
costs readability, never correctness, because a mis-decoded stretch still
reassembles to the same bytes.

The generator refuses to emit anything unless the byte spans it accounts for
reconstruct the ROM exactly — so a bug there fails loudly rather than producing a
plausible-looking listing.

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

## Hand-writing regions instead

The generated listing is the baseline, not a ceiling. If a region is worth
transcribing properly — with real comments and structure — replace it and keep
`make verify` green.

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

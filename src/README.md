# Annotated source

[firmware.asm](firmware.asm) reassembles to a file **byte-identical** to
[../original/firmware.bin](../original/firmware.bin) over its whole used extent,
`0000h-3047h` (12 360 bytes):

```
$ make verify
   6245 lines source file
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

Current state: **553 labels, 174 of them real names** from `symbols.csv`. Data
regions are declared in `DATA_REGIONS` in `tools/disasm.py`; getting one wrong
costs readability, never correctness, because a mis-decoded stretch still
reassembles to the same bytes.

### How the other 379 labels are named

Each is `<routine>_<role><n>`:

* **routine** — the nearest preceding entry point, meaning a name from
  `symbols.csv` or an address something `CALL`s. So `cmd_eot_br3` is inside
  `cmd_eot`, and you know that without scrolling.
* **role** — read straight off the control flow, never guessed:

  | role | meaning |
  | --- | --- |
  | `loop` | something branches back to it from a higher address |
  | `ret` | it sits on a shared epilogue: `POP`s and a `RET` |
  | `ptr` | only `SHLD`/`LHLD`/`STA`/`LDA` reach it, so it is data |
  | `br` | a plain forward branch target |

* **n** — an ordinal within that routine and role.

The names make no claim about *meaning*; that still only comes from
`symbols.csv`. What they do is make the listing navigable and turn every naming
win into a compound one: name a routine there, and its whole body is renamed on
the next `make disasm`. The ordinals renumber when that happens, but the churn is
confined to the generated file.

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

The generated listing is the baseline, not a ceiling. A region worth transcribing
properly, with real comments and structure, can replace the generated one as long
as `make verify` stays green. Two rules keep a matching build honest:

1. Never "fix" what the original does, however odd. A matching build documents the
   ROM; it does not improve it.
2. Erased space, `0x3048-0x7FFF`, is `0FFh` fill rather than zeros. Emit it
   explicitly or the checksum will not match.

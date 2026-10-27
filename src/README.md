# Annotated source

[firmware.asm](firmware.asm) reassembles to a file **byte-identical** to
[../original/firmware.bin](../original/firmware.bin) over its whole used extent,
`0000h-3047h`:

```
$ make verify
   6245 lines source file
      0 errors
      0 warnings
MATCH - reassembly is byte-identical to original/firmware.bin over 0000-3047
```

The erased tail above `3047h` is not assembled. `make rom-check` asserts it
separately, which is cheaper and just as conclusive.

## Why bother

A matching build is the standard proof of understanding in firmware reverse
engineering. It makes every claim falsifiable: a wrong constant, a misread
operand or a mis-sized table shows up as a mismatch instead of quietly living on
in a document. It also makes the ROM modifiable later, if the project ever goes
that way.

## How it is produced

`firmware.asm` is generated from the ROM, not transcribed by hand:

```sh
make disasm        # tools/disasm.py --asm > src/firmware.asm
make verify        # assemble it and compare against the ROM
```

That is deliberate. Hand-transcribing 12 KiB would take a long time and
introduce its own errors. Generating it means the listing is always a faithful
decode, and the interesting work, which is naming things, accumulates in
[../ghidra/symbols.csv](../ghidra/symbols.csv) and flows into the listing on the
next `make disasm`.

Of the 553 labels, 174 are real names from the symbol table. The rest are
`<routine>_<role><n>`, built from the control flow: which routine they are
inside, and whether something branches back to them, falls into them, or only
ever loads from them. They claim nothing about meaning. They make the listing
navigable, and they make every naming win compound, because naming a routine in
the CSV renames its whole body on the next run.

The generator refuses to emit anything unless the byte spans it accounts for
reconstruct the ROM exactly, so a bug in it fails loudly instead of producing a
plausible-looking listing.

## Toolchain

[Macro Assembler AS](http://john.ccac.rwth-aachen.de:8000/as/) (`asl`). It
supports the 8085 with Intel mnemonics, which is what Ghidra emits.

```sh
sudo apt install asl
```

## Hand-writing a region instead

The generated listing is a baseline, not a ceiling. A region worth transcribing
properly, with real comments and structure, can replace the generated one as
long as `make verify` stays green. Two rules keep a matching build honest:

1. Never fix what the original does, however odd it looks. A matching build
   documents the ROM, it does not improve it.
2. Erased space is `0FFh` fill, not zeros.

[hardware.inc](hardware.inc) holds the I/O and memory equates for hand-written
work.

# Original artefacts — DO NOT EDIT

Everything in this directory is a *pristine capture from the physical device*.
It is the ground truth every other file in the repository is derived from, and
must never be edited, reformatted or "cleaned up". Derived artefacts belong in
[ghidra/export/](../ghidra/export/), [src/](../src/) or `build/`.

## firmware.bin

| Property | Value |
| --- | --- |
| Source chip | U4, STMicroelectronics M27256F1 (32 KiB UV-EPROM) |
| PCB | Motherboard |
| Size | 32768 bytes (0x8000) |
| SHA-256 | `c78debbed9f1725786647b72d766300d6eadaa64c09e5ec8eedc780b27e77c75` |
| MD5 | `428dead8bf514976e625f2ec4aed036c` |
| Dumped | 2026-05-25 |
| Reader used | *TODO — record programmer model + software version* |
| Reads compared | **2 independent reads, SHA-256 identical** |
| Internal version string | `AK0052.139U1` at offset 0x0064 |
| Label on the chip | **`TELENORMA  18 JUN 96`** |
| Programmed | **18 June 1996** — the first hard date found on the unit |

Verify with:

```sh
make rom-check                              # from the repository root
cd original && sha256sum -c SHA256SUMS      # equivalent, by hand
```

### Sanity checks already performed

* `0x0000-0x3047` contains code and data; `0x3048-0x7FFF` is uniform `0xFF`
  (erased). A 32 KiB part holding ~12.3 KiB of payload is consistent with an
  unpadded image, not a truncated or misaligned read.
* The reset vector at `0x0000` is `DI / JMP 0x034B`, and all eight 8085
  RST/TRAP vectors hold well-formed `JMP` trampolines — a strong indication the
  image is correctly aligned and not bit-shifted.
* The 5x7 glyph table at `0x1C0D-0x210C` has bit 7 clear in all 1280 bytes,
  which would be extremely unlikely in a corrupt read.
* **Two independent reads produced identical SHA-256 digests.** Together with the
  structural checks above, the image can be treated as the ROM's true contents.

### The TELENORMA label

The EPROM carries a **`TELENORMA 18 JUN 96`** sticker — not a PAGE Ibérica one.
That is worth more than the date alone. **Telenorma** was the German firm
descended from **Telefonbau und Normalzeit** of Frankfurt, whose name literally
means "telephone construction and standard time": alongside telephone systems
they built **public and station clock and time-distribution installations**. The
company later became part of Bosch Telecom. **[L]** — recalled rather than
sourced, and worth verifying.

Two things follow:

* **The firmware was not necessarily written by PAGE Ibérica.** A German
  time-systems company's label on the programmed part suggests PAGE Ibérica
  either licensed the design, OEM'd it, or acted as the Spanish partner in a
  Telenorma system. See [../docs/device.md](../docs/device.md).
* **It gives a new place to look for documentation** — Telenorma and Bosch
  Telecom, not only PAGE Ibérica. See
  [../docs/references.md](../docs/references.md).

### Provenance — complete

- [x] **Second read taken and the SHA-256 matches.** Two independent reads of the
      part produced identical images, so the dump is verified rather than merely
      plausible.
- [x] **EPROM label photographed** — and it turned out to carry the TELENORMA
      marking above
      ([../hardware/photos/ROM_chip.jpg](../hardware/photos/ROM_chip.jpg)).
- [x] **The UV window is covered by the TELENORMA sticker.** No action needed, and
      one consequence worth stating outright:

The only field still open is the programmer model and software version used for
the dump — worth filling in above if it is still to hand, but nothing depends on
it now that two reads agree.

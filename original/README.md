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
| Reads compared | *TODO — record how many independent reads were compared* |
| Internal version string | `AK0052.139U1` at offset 0x0064 |

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

### Outstanding provenance work

- [ ] Re-read the EPROM a second time on the same programmer and confirm the
      SHA-256 matches, then note it above.
- [ ] Photograph the EPROM label/window state before it is exposed to light
      again (see [../hardware/photos/ROM_chip.jpg](../hardware/photos/ROM_chip.jpg)).

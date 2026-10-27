# Original artefacts. Do not edit.

Everything in this directory is a pristine capture from the physical device. It
is the ground truth every other file in the repository is derived from, so it
must never be edited, reformatted or tidied up. Derived files belong in
[../src/](../src/) or `build/`.

## firmware.bin

| Property | Value |
| --- | --- |
| Source chip | U4, STMicroelectronics M27256F1, 32 KiB UV-EPROM |
| Size | 32768 bytes |
| SHA-256 | `c78debbed9f1725786647b72d766300d6eadaa64c09e5ec8eedc780b27e77c75` |
| Dumped | 2026-05-25 |
| Reads compared | **Two independent reads, identical SHA-256** |
| Version string | `AK0052.139U1` at offset `0x0064` |
| Label on the chip | `TELENORMA 18 JUN 96` |

```sh
make rom-check                              # from the repository root
cd original && sha256sum -c SHA256SUMS      # the same thing, by hand
```

## Why the dump can be trusted

* Two independent reads produced identical digests.
* `0x0000-0x3047` holds code and data and `0x3048-0x7FFF` is uniform `0xFF`.
  A 32 KiB part holding 12.3 KiB of payload is what an unpadded image looks
  like, not a truncated or misaligned read.
* The reset vector is a well-formed jump, and so are all eight 8085 vectors.
  That is a strong sign the image is aligned and not bit-shifted.
* Bit 7 is clear in all 1280 bytes of the glyph table, which would be very
  unlikely in a corrupt read.
* It reassembles. [../src/](../src/) rebuilds this file byte for byte.

The only thing not recorded is the programmer model and software used for the
dump. Nothing depends on it now that two reads agree.

## The label

The EPROM carries a `TELENORMA 18 JUN 96` sticker rather than a PAGE Ibérica
one, and the motherboard carries the Telenorma logo. The firmware and the
electronics are both German in origin, which is also where documentation is
worth hunting for. See
[../docs/1-the-device.md](../docs/1-the-device.md#who-built-it).

The sticker covers the UV window, so the part was protected in storage.

#!/usr/bin/env python3
"""Summarise the firmware image: integrity, used/erased extent, strings.

Cheap sanity check that the dump in original/ is still the dump we documented,
and a quick map of which parts of the 32 KiB part actually hold anything.
"""

import argparse
import hashlib
import re
import sys

ROM = "original/firmware.bin"
EXPECTED_SHA256 = "c78debbed9f1725786647b72d766300d6eadaa64c09e5ec8eedc780b27e77c75"
EXPECTED_SIZE = 32768

# Regions documented in docs/memory-map.md, for cross-checking.
KNOWN = [
    (0x0000, 0x003F, "RST/TRAP vector table"),
    (0x0040, 0x0063, "vector trampolines"),
    (0x0064, 0x006F, "version string"),
    (0x0070, 0x1C0C, "constants + code"),
    (0x1C0D, 0x210C, "5x7 glyph table (256 x 5)"),
    (0x210D, 0x3047, "code + tables"),
    (0x3048, 0x7FFF, "erased (0xFF)"),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rom", default=ROM)
    ap.add_argument("--strings", type=int, default=6, metavar="N",
                    help="minimum run length for the printable-string scan")
    args = ap.parse_args()

    with open(args.rom, "rb") as fh:
        rom = fh.read()

    digest = hashlib.sha256(rom).hexdigest()
    ok = digest == EXPECTED_SHA256 and len(rom) == EXPECTED_SIZE

    print(f"file      {args.rom}")
    print(f"size      {len(rom)} bytes (0x{len(rom):X}){'' if len(rom) == EXPECTED_SIZE else '  ** UNEXPECTED **'}")
    print(f"sha256    {digest}")
    print(f"integrity {'OK - matches the documented dump' if ok else '** MISMATCH vs original/SHA256SUMS **'}")
    print()

    # Extent of non-erased content.
    last = len(rom) - 1
    while last >= 0 and rom[last] == 0xFF:
        last -= 1
    print(f"payload   0x0000-0x{last:04X} ({last + 1} bytes used, "
          f"{len(rom) - last - 1} bytes erased, "
          f"{100.0 * (last + 1) / len(rom):.1f}% full)")
    print()

    print("documented regions")
    for start, end, name in KNOWN:
        chunk = rom[start:end + 1]
        ff = chunk.count(0xFF)
        print(f"  0x{start:04X}-0x{end:04X}  {len(chunk):5d} B  "
              f"{100.0 * ff / len(chunk):5.1f}% 0xFF  {name}")
    print()

    print(f"printable runs of >= {args.strings} chars")
    for m in re.finditer(rb"[\x20-\x7e]{%d,}" % args.strings, rom):
        if m.start() > last:
            break
        print(f"  0x{m.start():04X}  {m.group().decode('ascii')!r}")

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

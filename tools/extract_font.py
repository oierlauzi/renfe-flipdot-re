#!/usr/bin/env python3
"""Extract the 5x7 glyph table from the PI-T100-M firmware.

The table lives at 0x1C0D and holds 256 entries of 5 bytes. Each byte is one
column, left to right; within a byte bit 0 is the top row and bit 6 the bottom
row. Bit 7 is unused and is clear throughout the table -- the extractor checks
that as a self-test.

See docs/font.md for the derivation.
"""

import argparse
import json
import sys

ROM = "original/firmware.bin"
FONT_BASE = 0x1C0D
GLYPH_BYTES = 5
GLYPH_ROWS = 7
GLYPH_COUNT = 256


def load(path):
    with open(path, "rb") as fh:
        return fh.read()


def glyph(rom, code):
    """Return the 5 raw column bytes for a character code."""
    off = FONT_BASE + code * GLYPH_BYTES
    return rom[off : off + GLYPH_BYTES]


BLOCK = ("▮", " ")   # easiest to read in a terminal
ASCII = ("#", ".")   # survives whitespace-trimming editors and diffs


def rows(cols, charset=BLOCK):
    """Convert 5 column bytes into 7 row strings of 5 characters."""
    on, off = charset
    return [
        "".join(on if (col >> r) & 1 else off for col in cols)
        for r in range(GLYPH_ROWS)
    ]


def label(code):
    return repr(chr(code)) if 0x20 <= code < 0x7F else "--"


def check(rom):
    """Verify the table is where and what we think it is."""
    end = FONT_BASE + GLYPH_COUNT * GLYPH_BYTES
    if len(rom) < end:
        sys.exit(f"ROM is {len(rom)} bytes, need at least {end}")
    bad = [
        FONT_BASE + i
        for i, b in enumerate(rom[FONT_BASE:end])
        if b & 0x80
    ]
    if bad:
        sys.exit(
            f"{len(bad)} byte(s) in the table have bit 7 set "
            f"(first at 0x{bad[0]:04X}) -- the table base or geometry is wrong"
        )
    if glyph(rom, 0x20) != b"\x00" * GLYPH_BYTES:
        sys.exit("glyph for space (0x20) is not blank -- table base is wrong")
    if glyph(rom, 0x30) != bytes((0x3E, 0x41, 0x41, 0x41, 0x3E)):
        sys.exit("glyph for '0' (0x30) does not match -- table base is wrong")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=ROM, help=f"ROM image (default: {ROM})")
    ap.add_argument("--json", action="store_true",
                    help="emit the table as JSON on stdout")
    ap.add_argument("--range", default="0x00-0xFF", metavar="LO-HI",
                    help="character codes to emit (default: 0x00-0xFF)")
    ap.add_argument("--ascii", action="store_true",
                    help="render with '#' and '.' instead of block characters")
    args = ap.parse_args()

    charset = ASCII if args.ascii else BLOCK

    rom = load(args.rom)
    check(rom)

    lo, _, hi = args.range.partition("-")
    lo, hi = int(lo, 0), int(hi or lo, 0)
    codes = range(lo, min(hi, GLYPH_COUNT - 1) + 1)

    if args.json:
        json.dump(
            {
                "rom_sha_note": "regenerate from original/firmware.bin",
                "base": FONT_BASE,
                "width": GLYPH_BYTES,
                "height": GLYPH_ROWS,
                "column_major": True,
                "lsb_is_top_row": True,
                "glyphs": {
                    f"0x{c:02X}": {
                        "offset": FONT_BASE + c * GLYPH_BYTES,
                        "columns": list(glyph(rom, c)),
                        "rows": rows(glyph(rom, c), ASCII),
                    }
                    for c in codes
                },
            },
            sys.stdout,
            indent=2,
        )
        print()
        return

    for code in codes:
        cols = glyph(rom, code)
        blank = " (blank)" if not any(cols) else ""
        print(
            f"0x{code:02X} {label(code):>4}  @0x{FONT_BASE + code * GLYPH_BYTES:04X}"
            f"  {' '.join(f'{b:02x}' for b in cols)}{blank}"
        )
        if not blank:
            for row in rows(cols, charset):
                print(f"           {row}")
        print()


if __name__ == "__main__":
    main()

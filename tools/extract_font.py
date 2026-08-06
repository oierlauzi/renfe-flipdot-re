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


# Flip-dot panel palette: a near-black bezel and "reflective" yellow discs
# (base + offset highlight, so the on-dots read as lit rather than flat-filled).
PNG_BG        = (8, 8, 7)
PNG_OFF_FILL  = (28, 26, 22)
PNG_ON_EDGE   = (196, 168, 0)
PNG_ON_FILL   = (255, 225, 0)
PNG_ON_HILITE = (255, 252, 180)
PNG_TEXT      = (130, 126, 114)

PNG_FONT_CANDIDATES = (
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
)


def render_png(rom, codes, path, cols=16, supersample=4):
    """Render `codes` as a grid of flip-dot glyphs, `cols` per row.

    Drawn at `supersample`x and downscaled, so the discs anti-alias instead
    of coming out jagged.
    """
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        sys.exit("--png requires Pillow: pip install pillow")

    dot, gap, pad, margin, header = 7, 3, 16, 26, 22
    pitch = dot + gap
    cell_w = GLYPH_BYTES * pitch + pad
    cell_h = GLYPH_ROWS * pitch + pad
    rows = -(-len(codes) // cols)

    width = margin * 2 + header + cols * cell_w
    height = margin * 2 + header + rows * cell_h

    ss = supersample
    img = Image.new("RGB", (width * ss, height * ss), PNG_BG)
    d = ImageDraw.Draw(img)

    font = None
    for candidate in PNG_FONT_CANDIDATES:
        try:
            font = ImageFont.truetype(candidate, 12 * ss)
            break
        except OSError:
            continue
    if font is None:
        font = ImageFont.load_default()

    grid_x, grid_y = margin + header, margin + header

    for c in range(cols):
        x = (grid_x + c * cell_w + cell_w / 2) * ss
        y = (margin + header / 2) * ss
        d.text((x, y), f"{c:X}", font=font, fill=PNG_TEXT, anchor="mm")
    for r in range(rows):
        i = r * cols
        if i >= len(codes):
            break
        x = (margin + header / 2) * ss
        y = (grid_y + r * cell_h + cell_h / 2) * ss
        d.text((x, y), f"{codes[i] >> 4:X}_", font=font, fill=PNG_TEXT, anchor="mm")

    for i, code in enumerate(codes):
        r, c = divmod(i, cols)
        ox = (grid_x + c * cell_w) * ss
        oy = (grid_y + r * cell_h) * ss
        cols_bytes = glyph(rom, code)
        rad = dot / 2 * ss
        corner = rad * 0.3
        for col, byte in enumerate(cols_bytes):
            for row in range(GLYPH_ROWS):
                cx = ox + (col * pitch + pitch / 2) * ss
                cy = oy + (row * pitch + pitch / 2) * ss
                box = [cx - rad, cy - rad, cx + rad, cy + rad]
                if (byte >> row) & 1:
                    d.rounded_rectangle(box, radius=corner, fill=PNG_ON_FILL,
                                         outline=PNG_ON_EDGE, width=max(1, ss // 2))
                    hr = rad * 0.45
                    hx, hy = cx - rad * 0.3, cy - rad * 0.3
                    d.rounded_rectangle([hx - hr, hy - hr, hx + hr, hy + hr],
                                         radius=hr * 0.3, fill=PNG_ON_HILITE)
                else:
                    d.rounded_rectangle(box, radius=corner, fill=PNG_OFF_FILL)

    img = img.resize((width, height), Image.LANCZOS)
    img.save(path)
    return width, height


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
    ap.add_argument("--png", metavar="PATH",
                    help="write a glyph-map PNG (flip-dot style) instead of printing text")
    ap.add_argument("--cols", type=int, default=16,
                    help="glyphs per row in --png output (default: 16)")
    args = ap.parse_args()

    charset = ASCII if args.ascii else BLOCK

    rom = load(args.rom)
    check(rom)

    lo, _, hi = args.range.partition("-")
    lo, hi = int(lo, 0), int(hi or lo, 0)
    codes = range(lo, min(hi, GLYPH_COUNT - 1) + 1)

    if args.png:
        w, h = render_png(rom, list(codes), args.png, cols=args.cols)
        print(f"wrote {args.png} ({w}x{h}, {len(codes)} glyphs)")
        return

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

# Glyph table

**Status: confirmed.** Regenerate the rendering below with:

```sh
python3 tools/extract_font.py                       # every glyph, as blocks
python3 tools/extract_font.py --ascii --range 0x30-0x39
python3 tools/extract_font.py --json > font.json
```

The tool re-derives the table base and geometry from the ROM on every run and
fails loudly if either is wrong, so it doubles as a regression check on the
claims below.

## Format

| Property | Value |
| --- | --- |
| ROM range | `0x1C0D-0x210C` (1280 bytes) |
| Entries | 256, indexed **directly by character code** (`glyph = 0x1C0D + code*5`) |
| Bytes per glyph | 5 (one per column, left to right) |
| Bits per column | 7 — **bit 0 = top row**, bit 6 = bottom row, bit 7 unused |

Bit 7 is clear in all 1280 bytes, which is what makes the geometry certain
rather than assumed.

Column-major with LSB at the top means a glyph is read as:

```
byte:   b0   b1   b2   b3   b4
row 0:  b0.0 b1.0 b2.0 b3.0 b4.0
row 1:  b0.1 b1.1 b2.1 b3.1 b4.1
  ...
row 6:  b0.6 b1.6 b2.6 b3.6 b4.6
```

Worked examples:

```
'0' = 0x30  @ 0x1CFD   3e 41 41 41 3e      'A' = 0x41  @ 0x1D52   7e 09 09 09 7e
      .###.                                      .###.
      #...#                                      #...#
      #...#                                      #...#
      #...#                                      #####
      #...#                                      #...#
      #...#                                      #...#
      .###.                                      #...#

' ' = 0x20  @ 0x1CAD   00 00 00 00 00      'B' = 0x42  @ 0x1D57   7f 49 49 49 36
      (blank)                                    ####.
                                                 #...#
                                                 #...#
                                                 ####.
                                                 #...#
                                                 #...#
                                                 ####.
```

## Code point coverage

* `0x20-0x7E` follows ASCII.
* `0x00-0x1F` are **not** blank — they hold graphic glyphs (arrows and similar
  symbols are visible around `0x1C80-0x1C93`). These are presumably the
  pictograms a station sign needs.
* `0x7F-0xFF` also hold real glyphs. Some resemble accented Latin letters
  (Spanish needs `Ñ Á É Í Ó Ú Ü`), but **the mapping is not yet established** —
  do not assume Latin-1 or IBM code page 437.

Determining the upper-half encoding matters for the eventual control protocol,
because it decides what byte to send for `Ñ`. Two firmware references worth
following: `LXI HL,0x1C0D` (3 sites) and `LXI HL,0x1C5D` (3 sites, = table
base + 16 entries).

## Relation to the physical display

The panel is 16 characters of 5x7 dots, i.e. 80x7 = 560 flip-dots, matching a
5-byte-per-character glyph exactly with no inter-character gap stored in ROM.
Whether the hardware inserts a physical gap between characters is a mechanical
question — see [hardware/pcbs.md](../hardware/pcbs.md).

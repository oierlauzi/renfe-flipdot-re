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

## Code page: IBM CP437, with two substitutions **[C]**

![Glyph map](font-map.png)

Regenerate the map with `make font-map`.

**The encoding is IBM Code Page 437.** Every region of the table matches, which
is what makes this certain rather than a guess:

| Range | CP437 content | Matches |
| --- | --- | --- |
| `0x01-0x0F` | smileys, card suits, `•`, `♪`, `♫`, `☼` | yes |
| `0x10-0x1F` | `►◄↕‼¶§▬↨↑↓→←∟↔▲▼` | yes |
| `0x80-0x9F` | `Çüéâäàåçêëèïîì ÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ` | yes |
| `0xA0-0xAF` | `áíóúñÑªº¿⌐¬½¼¡«»` | yes |
| `0xB0-0xDF` | shading and box drawing | yes |
| `0xE0-0xEF` | `αßΓπΣσµτΦΘΩδ∞φε∩` | yes |
| `0xF0-0xFE` | `≡±≥≤⌠⌡÷≈°∙·√ⁿ²■` | yes |

Spot-checked against the raw bytes: `0xA5` renders an unmistakable **Ñ**, `0xA8`
an inverted `¿`, `0xAD` an inverted `¡`, and `0x82` an `é`.

### What to send for Spanish text

| Character | Byte | | Character | Byte |
| --- | --- | --- | --- | --- |
| `Ñ` | `0xA5` | | `á` | `0xA0` |
| `ñ` | `0xA4` | | `í` | `0xA1` |
| `¿` | `0xA8` | | `ó` | `0xA2` |
| `¡` | `0xAD` | | `ú` | `0xA3` |
| `é` | `0x82` | | `Ü` | `0x9A` |

In Python, `text.encode("cp437")` produces the right bytes directly.

### The two substitutions

`0x24` and `0x25` are **not** `$` and `%`. Both are accented Latin capitals;
`0x25` reads clearly as a second **Ñ**:

```
0x24  7c 09 05 05 78        0x25  7c 09 11 21 7c
      .###.                       .###.
      .....                       .....
      #.##.                       #...#
      ##..#                       ##..#
      #...#                       #.#.#
      #...#                       #..##
      #...#                       #...#
```

A second `Ñ` inside the 7-bit range is exactly what a Spanish installation needs
when the link, the controller or an intermediate system is not 8-bit clean. The
identity of `0x24` is less certain — another accented capital, but which one is
**[?]**.

**Consequence for a controller:** do not send `$` or `%` expecting them to
appear. Everything else in `0x20-0x7E` matches ASCII as far as the glyph map
shows.

Two firmware references worth following if the table is ever indexed
differently: `LXI HL,0x1C0D` (3 sites) and `LXI HL,0x1C5D` (3 sites, = table
base + 16 entries).

## Relation to the physical display

The panel is 16 characters of 5x7 dots, i.e. 80x7 = 560 flip-dots, matching a
5-byte-per-character glyph exactly with no inter-character gap stored in ROM.
Whether the hardware inserts a physical gap between characters is a mechanical
question — see [hardware/pcbs.md](../hardware/pcbs.md).

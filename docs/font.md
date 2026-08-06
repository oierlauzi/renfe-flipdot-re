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

### The two substitutions: `ñ` and `Ñ`, reachable over a 7-bit link **[C]**

`0x24` and `0x25` are **not** `$` and `%`. They are **`ñ` and `Ñ`**, deliberate
duplicates of CP437's `0xA4` and `0xA5`, placed so that **stripping bit 7 lands on
the right glyph**:

```
0xA4 & 0x7F = 0x24      n-tilde
0xA5 & 0x7F = 0x25      N-tilde
```

That is the whole explanation, and it is not a coincidence of two arbitrary
positions: it is the *only* pair of characters Spanish cannot do without, put
exactly where a 7-bit link would deliver them.

The glyphs are redrawn rather than copied — same letters, different artwork:

```
    0x24  ñ       0xA4  ñ           0x25  Ñ       0xA5  Ñ
    .###.         .#.#.             .###.         .##.#
    .....         #.#..             .....         #..#.
    #.##.         .....             #...#         .....
    ##..#         ###..             ##..#         ##..#
    #...#         #..#.             #.#.#         #.#.#
    #...#         #..#.             #..##         #..##
    #...#         #..#.             #...#         #...#
```

The `0x2x` pair use a full-width tilde on row 0 with the letter below; the CP437
originals squeeze a smaller tilde into rows 0-1. In each pair the letter body is
identical — compare the raw bytes, where `0x25` is `7c **09 11 21** 7c` against
`0xA5`'s `7a **09 11 22** 79`.

`0x24` and `0x25` differ from each other exactly where lowercase differs from
uppercase: `0x24`'s body is an `n` shoulder-and-stem, `0x25`'s carries the
diagonal that both `N` glyphs show.

**Three findings now agree**, arrived at independently:

1. The DIP switches as found decode, under the most plausible reading, to **7 data
   bits** ([dip-switches.md](dip-switches.md#as-found-c-for-the-readings--for-the-mapping)).
2. The firmware masks with `ANI 0x7F` wherever it takes a parameter — the `GS`
   address at `0x1060`, `VT`'s row and column at `0x0CD0`/`0x0D00`, `EOT`'s
   sub-mode at `0x1123`.
3. The font table duplicates precisely `ñ` and `Ñ` at their bit-7-stripped
   positions.

A 7-bit link physically cannot carry `0xA5`. The duplicates are not a curiosity,
they are what makes the display usable in Spanish over the link it was given.

The original reading of this section — "both are accented Latin capitals, `0x24`
unidentified" — was half right and is superseded. **[C]**

### The original observation, for the record

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

These were first read as "two accented Latin capitals, `0x24` unidentified". The
`& 0x7F` relationship above supersedes that: they are `ñ` and `Ñ`.

### CP437's gap: uppercase accented vowels

Worth knowing before writing station names, because they are usually uppercase.
CP437 carries the *lowercase* accented vowels (`á í ó ú` at `0xA0-0xA3`) but of
the uppercase ones only **`É` (0x90)**, plus `Ä Å Ö Ü Æ` and `Ñ (0xA5)`.

**There is no `Á`, `Í`, `Ó` or `Ú`.** So `CHAMARTÍN` cannot be sent as written;
it has to be `CHAMARTIN`, or use one of the two substitute glyphs. `Ñ` and `É`
are the two that do work — fortunately the two Spanish most needs.

Note this is a *different* gap from the one `0x24`/`0x25` fill. Those two exist so
that `ñ` and `Ñ` survive a 7-bit link; they do nothing for the missing accented
capitals, which simply cannot be displayed at all.

`tools/pit100.py` refuses to encode a character with no glyph rather than
silently sending the wrong one, and offers `fold=True` to drop the impossible
accents.

**Consequence for a controller:**

* Do not send `$` or `%` expecting them to appear — they are `ñ` and `Ñ`.
* Over a **7-bit link**, send `ñ` as `0x24` and `Ñ` as `0x25`; `0xA4`/`0xA5` will
  have their top bit stripped and arrive at those codes anyway, so either works.
  `tools/pit100.py`'s `encode(seven_bit=True)` does the mapping explicitly.
* Fold or avoid uppercase `Á Í Ó Ú`, which have no glyph at any code point.
* Everything else in `0x20-0x7E` matches ASCII as far as the glyph map shows.

Two firmware references worth following if the table is ever indexed
differently: `LXI HL,0x1C0D` (3 sites) and `LXI HL,0x1C5D` (3 sites, = table
base + 16 entries).

## Relation to the physical display

The panel is 16 characters of 5x7 dots, i.e. 80x7 = 560 flip-dots, matching a
5-byte-per-character glyph exactly with no inter-character gap stored in ROM.
Whether the hardware inserts a physical gap between characters is a mechanical
question — see [hardware/pcbs.md](../hardware/pcbs.md).

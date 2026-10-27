# 5. The panel

How a character in the buffer becomes moving dots, and what the dots draw.

| | |
| --- | --- |
| Topology | 16 character modules on a shared 26-conductor bus, not daisy-chained |
| Carriers | Modules mount in pairs on carrier boards, 8 boards for 16 characters |
| Addressing | Encoded binary position, 0 to 63, active low |
| Data | A serial shift register: one clock line, one data line |
| Frame | 74 bits per character |
| Update | Differential. Only characters that changed are redrawn |
| Driver | Selected by the panel family switches |

## Safety

**Flip-dot coils are designed for short pulses. Leaving one energised will
overheat it and can destroy it.** Any experiment that drives the panel directly,
bypassing this firmware, must bound the pulse width in hardware or in a
watchdog, not merely in software that could hang.

## The signal chain

```
M81C55-5 at I/O 0xE0          3 x ULN2003A
  port A  (glyph data)        7 Darlington sinks each    ribbon     560 coils
  port B  (character address) = 21 sinks, with        ->  26     ->  16 x 5 x 7
  port C  (clock, data, attributes)   clamp diodes       wires
```

Two facts about the ULN2003A set the shape of everything downstream:

* **It has free-wheel clamp diodes**, which exist for inductive loads. That
  confirms the panel is driven as coils.
* **It can only sink current.** A flip-dot needs current through its coil both
  ways, to set and to reset, so something else must supply the high side. What,
  exactly, was never traced. Daughter 2 carries 9 discrete transistor stages
  near the ribbon that are the obvious candidate.

Twenty-one lines cannot address 560 dots under any scheme, so the panel carries
its own decoding.

Every value is complemented before it is written to a port, so the drive is
active low, as current sinks require.

## How a character is sent

| Port | Bit | Function |
| --- | --- | --- |
| `0xE2` (port B) | 7:0 | Character position, inverted, bit 7 forced set |
| `0xE3` (port C) | 4 | Shift clock. Data is clocked on the rising edge |
| `0xE3` (port C) | 5 | Serial data, active low, so low means dot on |
| `0xE3` (port C) | 1:0 | Display effect lines |

The transaction is bracketed by two writes to port B: one selects the character
position, one puts `0x80` back to idle. Between them, 74 bits are clocked out:

```
[ 37 bits data high ][ 1 bit data LOW, the marker ][ 35 glyph bits ][ 1 bit high ]
```

The 35 glyph bits are 5 columns of 7 rows, top row first.

**Anchor a decoder on the marker, not on the idle run.** A blank glyph is 35
high bits and is indistinguishable from idle padding. The single low marker bit
is the only reliable way to find a frame.

### Addressing is encoded, not one-hot

Port B carries `~position | 0x80`. For a six-character message the bus sees:

```
written  : FF 80  FE 80  FD 80  FC 80  FB 80  FA 80
position :  0      1      2      3      4      5
```

`0x80` is `~0x7F`, which is address 127, outside the 64 valid positions, so it
is a bus idle state rather than an address.

This matters for two reasons. Encoded addressing is why the bus needs only six
conductors for the address instead of sixteen select lines, and six is exactly
the number of address switches on each carrier board. And it is why the panel
can span 64 positions on a cable that is only 26 conductors wide.

How the two modules on one carrier get distinct addresses from a single set of
six switches was never traced. A base value plus a fixed per-socket offset would
fit, but nothing confirms it.

### The other 48 positions

The firmware addresses positions 0 to 63 on one bus, contiguously, with no split
at character 16. The flip-dot line is 16 characters, so 48 addresses reach
something else. What, was never traced.

The most likely answer is the LED matrix row that sits under the flip-dot line
in a complete sign, which this specimen did not come with. One test separates
the possibilities: instrument the ribbon, send a 64-character message, and watch
which addresses draw a response.

## The panel itself

* Each character is an independent module. A single large matrix was never an
  option.
* Modules mount in pairs on a **flip-dot carrier board**, so this unit's 16
  characters sit on 8 carriers. Each carrier holds the ribbon connector and the
  pair's six address switches.
* The carriers hang off the ribbon as a common bus. They are not daisy-chained:
  the ribbon passes across them and leaves the enclosure after the last one.
* **The character module is a diode matrix, not a shift register.** The back of
  a module shows 35 diodes wired against row and column nets, which is a plain
  5 by 7 matrix. The serial-to-parallel conversion happens on the carrier.

The carrier's parts fit that reading: six dual 4-bit shift registers, a hex
buffer, an 8-input NAND and two quad XNORs. XNOR into NAND is the textbook shape
of an address comparator, and shift registers are what turns a serial bit stream
into parallel drive.

The carrier silkscreen carries the Telenorma logo, which puts Telenorma's design
on the panel as well as in the enclosure.

### Driving the panel directly

The interface is an encoded address plus a clock and a data line, so the panel
can be driven directly, bypassing the original electronics. An ESP32 can produce
that without difficulty, and it needs no modification to the display, only a
different cable onto the existing ribbon header.

That is not how this project drives the sign, because the point was to go
through the original firmware, but it is a genuine alternative. It gives up the
ROM's glyph table, effects and addressing, and the safety note above becomes
your problem rather than the firmware's.

## Power-on

On power-up a buzzing sound sweeps across the panel. That tells you several
things at once:

* The rest state is all dots black, and power-on drives every dot to it.
* The sweep is sequential, not simultaneous. The sound travels, so coils are
  energised in groups small enough to be heard moving across the panel.
* **The firmware does not know the panel state at power-on and does not try
  to.** Flip-dots are bistable, so after a power cut the dots are wherever they
  were left while the frame buffer is empty. Forcing every dot is the only way
  to make the two agree.
* It is audible proof that the drive electronics and at least some coils work,
  which is useful before any serial work succeeds.

The sweep does not appear in emulation. Booting the ROM against emulated
peripherals produces only eight writes to the display ports, with no per-dot
activity at any interrupt rate. So the sweep is probably done by hardware on
daughter 2, triggered by the short pulse the firmware puts on ports A and B
during initialisation.

## Display effects

`BEL` plus a parameter sets two hardware lines on port C, giving four states.
The lines are held between glyph frames, which is why the shift loop carefully
preserves the low bits of port C on every write. A third bit makes an effect
permanent instead of timed.

**What those two lines physically do is not known.** Two things are ruled out:
they are not brightness, because the dots are mechanical and bistable, and they
are not the fluorescent tube, which has its own mains feed and no connection to
the electronics at all. The most likely answer is that they reach the LED matrix
row, which is the part of a complete sign that can blink or dim.

## The font

The glyph table is at ROM `0x1C0D`, 256 entries of 5 bytes. It is indexed
directly by character code, so a glyph is at `0x1C0D + code * 5`. Each byte is
one column, left to right, and bit 0 is the top row. Bit 7 is clear in all 1280
bytes, so the geometry is certain rather than assumed.

![The 256 glyphs](font-map.png)

Regenerate the map with `make font-map`, or dump glyphs as text with
`tools/extract_font.py`. The tool re-derives the table position and geometry
from the ROM on every run and fails loudly if either is wrong, so it doubles as
a check on everything in this section.

### It is IBM CP437

Every region of the table matches CP437: the smileys and card suits at the
bottom, the box drawing in the middle, the Greek letters near the top. In Python
`text.encode("cp437")` produces the right bytes.

For Spanish text:

| | Byte | | | Byte |
| --- | --- | --- | --- | --- |
| `Ñ` | `0xA5` | | `á` | `0xA0` |
| `ñ` | `0xA4` | | `í` | `0xA1` |
| `¿` | `0xA8` | | `ó` | `0xA2` |
| `¡` | `0xAD` | | `ú` | `0xA3` |
| `é` | `0x82` | | `Ü` | `0x9A` |

### Two deliberate substitutions

**`0x24` and `0x25` are not `$` and `%`. They are `ñ` and `Ñ`.**

They are duplicates of CP437's `0xA4` and `0xA5`, placed exactly where stripping
the top bit lands:

```
0xA4 & 0x7F = 0x24
0xA5 & 0x7F = 0x25
```

That is the whole explanation. This unit runs a 7-bit link, which physically
cannot carry `0xA5`, and `ñ` is the one character Spanish cannot do without. The
duplicates are what make the display usable in Spanish over a link that is not
8-bit clean. The glyphs are redrawn rather than copied, with a full-width tilde
instead of the squeezed CP437 one.

The firmware agrees from the other side: it masks the top bit off every
parameter byte it takes.

### What you cannot display

CP437 has the lowercase accented vowels but almost none of the uppercase ones.
There is no `Á`, `Í`, `Ó` or `Ú`. Only `É` and `Ñ` work, which happens to be the
two Spanish needs most.

So `CHAMARTÍN` cannot be sent as written. It has to be `CHAMARTIN`.
`tools/pit100.py` refuses to encode a character with no glyph rather than
sending the wrong one, and offers a fold option that drops the impossible
accents instead.

### The ROM holds two more tables

Worth knowing before making any claim about "the font". They belong to other
panel types in the family and share nothing with the one above.

| Table | Address | Geometry | Codes |
| --- | --- | --- | --- |
| 5x7 | `0x1C0D` | 5 by 7 | all 256 |
| 4x7 | `0x23CF` | 4 by 7 | `0x20-0x5A` |
| 3x5 | `0x2692` | 3 by 5 | 42 glyphs, digits and capitals |

`tools/extract_font.py --font 4x7` and `--alt` render them. The 4x7 table stores
its columns right to left, so read naively every asymmetric letter comes out
mirrored. The 3x5 table is bit-packed behind a pointer table and still has
unexplained bits in it.

Three independent searches for a fourth table all came back negative.

## Proof that all of this is right

[../tools/panel_view.py](../tools/panel_view.py) watches the emulated firmware
drive the display ports, recovers the clocked serial data, and reconstructs the
glyphs:

```sh
PYTHONPATH=tools python3 tools/panel_view.py --text "HOLA MUNDO"
```

```
   #...#  .###.  #....  .###.  #...#  #...#  #...#  ####.  .###.
   #...#  #...#  #....  #...#  ##.##  #...#  #...#  #...#  #...#
   #...#  #...#  #....  #...#  #.#.#  #...#  ##..#  #...#  #...#
   #####  #...#  #....  #####  #.#.#  #...#  #.#.#  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #..##  #...#  #...#
   #...#  #...#  #....  #...#  #...#  #...#  #...#  #...#  #...#
   #...#  .###.  #####  #...#  #...#  .###.  #...#  ####.  .###.
```

Frame length, marker position, bit order and polarity all had to be right at the
same time for that to come out.

Nine glyphs for ten characters: the space between the words produces no frame,
because that position already held a space. That is the differential update
doing its job.

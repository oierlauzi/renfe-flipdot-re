# Lab notebook

Chronological, newest first. One entry per session. Record what was *tried*, not
just what worked — a hypothesis that failed is the most useful thing in the file
six months later, because it stops you re-running it.

Settled conclusions belong in [docs/](docs/); this file is the trail that leads
to them.

Entry template:

```
## YYYY-MM-DD — short title

**Did:**
**Found:**
**Concluded:** (with a [C]/[L]/[?] mark)
**Next:**
```

---

## 2026-08-06 — EOT, BS and SI: three commands finish decoding, one surprise each

**Did:** worked the remaining "what does this command actually do" items with the
new disassembler, then confirmed every reading by emulation rather than stopping
at the listing.

**Found — `EOT` is a block-write, with an escape hiding in it.** Three sub-modes,
all verified: `'1'` writes 64 bytes to `0xE084`; `'2'` takes a 16-bit offset and
count; **`'3'` stores the next byte verbatim, bypassing the command dispatcher.**

That third one is the practically useful discovery. The dispatcher eats `0x00-0x1F`
as commands and raises no error, so control bytes silently vanish from a message.
CP437 has real glyphs there — smileys, card suits, arrows — and `EOT '3'` is the
only way to reach them. Confirmed mid-message and for `0x00`:

```
STX 'A' 0x01 'B' ETX          -> 41 42        the 0x01 vanished
STX 'A' EOT'3' 0x01 'B' ETX   -> 41 01 42     stored
```

**Found — `BS` reports a sticky hardware status byte.** `0xA024` is not a software
error code, it is an **accumulator of I/O port `0xDE`**: the RST7.5 tick samples
the port, complements it (active low) and ORs it in. `BS` returns it *and clears
it*. So the semantics are edge-triggered — a fault that came and went is still
reported once, and polling returns `0` while a condition persists.

I had `0xDE` written down as an unidentified port read twice and never explained.
Its *role* is now clear even though its bit meanings are not: it is the unit's
hardware status input, readable over the serial link. That upgrades a dead entry
in the I/O map into a concrete hardware task — ground its driver pins one at a
time and watch `BS`.

**Correction made mid-session:** I first wrote that port `0xDE` was "a hardware
input this build never reads", because `0x1101` (the alternate `BS` path) is
unreachable with ROM `0x008B = 0`. Wrong — `0x19FE` reads it on every tick. I had
looked at one of the two read sites and generalised from it. Fixed before commit.

**Found — `SI` reads the shadow, not the message buffer.** `0x0E45` sources from
`write_ptr + 0x42`, which is the shadow copy: **`SI` reports what is displayed,
not what was written.** That makes it the natural way to confirm an update
committed, and it produced the cleanest single confirmation of the whole model:

```
select, message("CHAMARTIN"), goto(0,0), SI   -> spaces
deselect, select, goto(0,0), SI               -> 'CHAMARTIN'
```

One experiment validating the repaint interlock, the shadow buffer and `SI`'s
source together.

**Concluded:** handler B's command set is now fully decoded with every semantic
claim tested, not just read. **[C]**

**Next:** the 8 display attribute values (`BEL`); what the `0xE084` region is
*for*, since nothing in the render path reads it; port A bit 7.

---

## 2026-08-06 — The render path closes: text on the virtual panel

**Did:** built `tools/disasm.py` first, because the repo had an emulator but no
disassembler and I had been reading code through throwaway scripts. Validated it
by reproducing the hand-annotated listings at `0x210D` and `0x29F6` in the docs
byte for byte. Then instrumented the emulator: window writes with their callers,
a PC ring buffer to catch what jumped where, and marks on the candidate render
entry points.

**Found:** the buffer-to-glyph link had **two** breaks, neither in the frame
format.

1. **The repaint interlock.** `0x130B` is `LDA 0xA040 / CPI 0 / JNZ 0x130B` — the
   repaint task spins for as long as the unit is *selected*. Send `GS <addr> STX
   text ETX` and stop, and the text is stored, `ACK`ed, and never drawn. Without
   `RS`: 259 016 spin hits, zero render passes. With `RS`: 131 passes and exactly
   two characters drawn for a two-character message. Almost certainly deliberate —
   it stops a half-received line from being displayed.
2. **Port B bit 0.** Set, the pad character becomes the solid block `0x23`. The
   "fill character" I had recorded as a symptom of a missing step was the pad
   character working correctly all along.

`panel_view.py --text "HOLA MUNDO"` now renders. Kept that string as the example
rather than a station name because the space in the middle produces **no frame** —
it was already a space, so the differential update correctly does nothing. A
single word would not show that.

**Also found, while in there:**

* The window layout, every boundary written by identifiable code: `0xE000` start
  char, `0xE001-0xE040` buffer, `0xE041`/`0xE042` terminators, `0xE043-0xE082`
  shadow, `0xE083` terminator, and a **third 64-byte region at `0xE084`** whose
  purpose is unknown.
* Both render passes terminate on the byte matching `0xA007`, **not on a count**.
* `0xA023` splits the supported displays into continuously refreshed (types 2, 8,
  9, `0x0B`-`0x0E`) and differentially updated (everything else, including this
  panel's `0x11`). **This independently supports assumption A1**: bistable dots
  must be driven differentially or you are just heating coils, so `0x11` landing
  in the differential class agrees with it being the flip-dot driver — reached
  from a different direction than the "most drive activity" heuristic that
  originally picked it.
* `HL` in the paged accessors is a flat 16-bit address into the expansion, split
  by the accessor into a bank for the latch and an offset inside the window.

**A wrong turn worth recording.** My first probe sent `GS 0x65` because I had
`0x60 | addr` in mind. `0x60 | addr` is real but it is `0xA006`, handler A's frame
*delimiter* — handler B's `GS` address is printable ASCII, `0x30`/`0x40 | bits
3:0`. Every byte was silently discarded and I spent a probe cycle thinking
selection was broken. `tools/pit100.py` had it right the whole time; I hand-rolled
the bytes instead of using the driver.

**Concluded:** the render path is understood end to end, from serial byte to
clocked glyph bit. **[C]** in emulation. The last structural unknown is whether
*this* unit has the memory the buffer lives in — A6, and only the hardware can
answer it.

**Next:** the `0xE084` region; `EOT` sub-modes and the `BS` status report; the 8
display attribute values.

---

## 2026-08-06 — Correction: the module address is encoded, not one-hot

**Three observations from the owner, and the second one refutes yesterday's
model.**

1. **9 transistors on daughter 2 near the ribbon**, each with a diode and a 22 kΩ
   resistor.
2. **The ribbon continues to the LED panel, which has more characters and
   scrolls.** Allocating 16 of 26 conductors to flip-dot selects leaves almost
   nothing for a *larger* downstream display. That does not work.
3. **Only the rear of the display modules has been inspected** — the front needs
   extensive disassembly — so "no address switches" was never established.

**I was wrong about one-hot selects, and the evidence was already in hand.** Port
B takes **49 distinct values**, which an 8-bit one-hot scheme cannot produce. I
recorded that number and did not draw the conclusion. Undoing the `CMA`:

```
pre-CMA : 7F 3D 7F 3E 7F 3F 7F 00 7F 01 7F 02 7F 03 ...
stripped: 3D 3E 3F 00 01 02 03 04 05 06 07 08 09 0A ...
```

A plain **incrementing counter over `0x00-0x3F`** — 48 distinct values, only 6 of
them one-hot. `0x7F` between each is a strobe or bus-idle state.

**So port B is an encoded module address spanning 64 positions**, and three things
fall into place at once:

* **64 positions matches the 64-byte message buffer exactly** — one address per
  buffer column.
* **The conductor budget works**: ~6-7 address bits plus clock, data and control is
  about 10 signals, leaving most of the 26 for supply *and* a downstream display.
* **The LED panel fits.** A 64-position encoded address serves a 16-character
  flip-dot line and a longer scrolling panel from one ribbon. Sixteen dedicated
  selects could not.

Observation 3 removes the very constraint that pushed me to one-hot in the first
place. And an encoded broadcast address *requires* each module to know its own
position, so something must configure it — the module fronts are now worth a look
specifically.

**On the 9 transistors.** A 22 kΩ base resistor means low base current, so
small-signal switches, not power drivers. Two readings, not exclusive: **bus
drivers** for a long capacitive 26-way bus leaving the enclosure (note port B's 8
bits plus clock plus data is 10 signals, and 9 is suggestively close); or **the
missing high side** — ULN2003As only sink, yet a coil needs current both ways, so
9 source drivers with a Darlington pulling each base through 22 kΩ would close that
gap. Their part numbers and which side of the load they sit on decides it.

**Lesson, again:** the 49-distinct-values figure was sitting in my own output when
I wrote "16 one-hot selects". Counting is not the same as checking that the count
is consistent with the model.

---

## 2026-08-06 — 26 conductors: the select-line model is confirmed

**Counted (owner):** the panel ribbon has **26 conductors**.

That sits in the predicted 20-26 band and **decisively rules out** the ~8-10
alternative in which the character position would have been decoded on the panel.
The select-line model stands: 16 per-character selects, a shift clock, a serial
data line, two control lines, and 6 conductors left for 24 V and returns — which
flip-dot coils need, being genuinely current-hungry. 26 is also a standard IDC
width (13 x 2), so the header is very likely 26-way IDC.

What is confirmed is the **count**, and therefore the *shape* of the interface. The
per-conductor allocation is still [L] until traced — but tracing is now a guided
job rather than guesswork: find the 16 conductors that each reach exactly one
module, the 2 that reach every module, and the rest carrying supply.

**A strategic consequence worth recording.** With 16 selects plus clock and data,
and the 74-bit frame format already decoded, **the panel can be driven directly** —
bypassing the original electronics, the DB37, the serial link and the entire
unresolved current-loop question. An ESP32 with a shift register does it easily,
and it needs no modification to the display, only a different cable onto the
existing ribbon header.

That is *not* the project's goal, which is to drive the display through its own
firmware and so keep the machine intact as a working PI-T100. But it is a real
fallback if the electrical layer proves intractable, and it is worth knowing the
project cannot be completely blocked. Recorded in `docs/controller.md` as plan B,
explicitly flagged as second choice.

---

## 2026-08-06 — Panel topology: independent modules on a shared bus

**Observed (owner):** each character is an **independent module**, sixteen of them,
hanging off the ribbon as a **common bus** rather than daisy-chained. The ribbon is
not interrupted and re-emitted per module, no module has address switches, and it
**exits the unit** after the last character — spliced where the display was
removed, and almost certainly continuing to the **second LED panel**.

**What that forces.** Identical modules on a shared bus with no per-module
addressing can only be distinguished by *which conductor each is wired to*. So
position must travel on **dedicated select lines**, one per character. And that is
exactly what the sink count allows:

| Function | Lines |
| --- | --- |
| Character selects | 16 |
| Shift clock, port C bit 4 | 1 |
| Serial data, port C bit 5 | 1 |
| Control, port C bits 1:0 | 2 |
| **Total** | **20**, against 21 ULN2003A sinks |

So **port A (`0xE1`) and port B (`0xE2`) supply the selects, port C the clock and
data**. The `CMA` before every select write plus Darlington sinks pulling low means
**select is active low at the module**. Consistent with what the drivers do: type
`0x11` writes only `0xE2`, while types `0x10` and `0x12` write both — different
panel widths using one or both select ports.

**A sharp test falls out.** The conductor count at the splice: ~20-26 confirms the
model, ~8-10 refutes it and means the position is decoded on the panel instead.

**And the LED panel being on the same ribbon sharpens the `0xE000` question into a
falsifiable prediction.** `0xE7` is a register on the expansion; the message buffer
sits at `0xE001-0xE040` inside the window `0xE7` pages; the LED panel assembly is
the obvious expansion. If those hold, **this unit is missing the RAM its message
buffer lives in** — which would explain why the emulator needs `--xram board`
before text renders at all, and why `STX AB ETX` paints the fill character.

Prediction, testable at first power-up: with the LED panel absent the unit should
still sweep and still answer `SUB` (neither needs the buffer) but should be
**unable to display sent text**. If it does display text, the reasoning is wrong and
the `0xE000` memory is on a board that is present. Either way it costs one power-up
and one message.

---

## 2026-08-06 — The panel is a serial shift register, and the render path runs

**Did:** Cracked the render path. It needed four separate conditions satisfied
before a single dot moved, which is why every earlier attempt saw only the eight
init writes.

1. **A panel type.** Port C bits 4:0 pick a branch at `0x055A`, each loading a
   type into `0xA009`; `0x136A` dispatches on it to a panel-specific driver. **The
   default branch sets `0xA009 = 0`, for which no driver exists** — so unrecognised
   switches drive nothing at all. 13 of the 32 values select a real driver.
2. **The RST7.5 tick.** The idle loop at `0x1675` spins while `0xA01B & 1` is set,
   and only the RST7.5 handler clears it.
3. **Changed content.** `0x136A` is reached by **`CNZ`** — a conditional call after
   comparing against a shadow copy. **Only changed characters are redrawn.** My
   all-zero buffer against an all-zero shadow flipped nothing, which is the whole
   reason nothing appeared.
4. Somewhere for the buffer to live.

**The big structural finding: the panel is fed serially, not as a matrix.** That
dissolves the "21 lines cannot address 560 dots" problem entirely.

* Port C **bit 4 = shift clock**, rising edge.
* Port C **bit 5 = serial data, active low** — low means the dot is on.
* Port B = character position, inverted.
* **74 bits per character**: 37 idle high, one **low marker**, 35 glyph bits
  (5 columns x 7 rows, top row first), one trailing high.

`RLC` five times then `RRC` per row emits the glyph byte's bits in order 0..6,
matching the font's bit-0-is-top layout exactly.

**Validated end to end.** `tools/panel_view.py` recovers the stream and
reconstructs glyphs; sending `FF` renders solid blocks, which is precisely the
ROM's `0x23` glyph. Frame length, marker position, bit order *and* polarity all
had to be right simultaneously for that to come out — the strongest single
confirmation in the project so far.

**Anchor on the marker, not the idle run.** A blank glyph is 35 high bits and is
indistinguishable from padding; my first decoder looked for runs of idle and
mis-located every frame.

**Also newly attributed: port B bit 0 selects the fill character.** Set stores
`0x23` into `0xA041`, so the pad is a solid block rather than a space. Clearing it
makes the fill activity vanish from a capture. One of the last two unattributed
port B bits.

**What did not work, stated plainly:** message text does not reach the rendered
glyphs. `STX AB ETX` paints the fill character instead of `A` and `B`, so the link
from message buffer to render source still has a missing step — most likely tied
to the unresolved `0xE000` window. The frame mechanism is unaffected, having been
validated via `FF`, but `panel_view.py --text` should not be trusted yet.

**Assumptions recorded for falsification** in `docs/flipdot-interface.md`, with a
table giving for each one why it was chosen, how to check it on the hardware, and
what breaks if it is wrong. The load-bearing ones are the panel type (0x11, chosen
because it produced the most activity) and the 37 leading idle bits, which may
address a longer register chain than a single character.

---

## 2026-08-06 — A host driver, and an emulator bug that faked a protocol timeout

**Did:** Wrote `tools/pit100.py`, a host-side driver for the command protocol,
and tested it against the emulator. It works end to end: select, identify, cursor
query, message, ACK.

```
version : AK0052.139U1
config  : 00 00 01 00 00 2f 30 00 01 00 ff 40 01 00 40 ...
cursor  : row 0, col 0
message : 'HOLA' -> ACK
```

**Nearly documented a protocol constraint that does not exist.** While building
it, splitting `GS <addr>` and the following command into separate emulator runs
made the command silently fail. Bisecting gave a suspiciously crisp figure — the
selection survived 24.8 ms and was gone by 25.2 ms — and sweeping port B showed it
did not scale with baud rate, which would have made the protocol unusable at 300
baud. All of that was ready to write up as a finding.

It was **my bug**. Instrumenting the RAM showed the *whole machine* resetting, not
a flag expiring, and the receive handler being entered 73 times for three fed
bytes. Cause: `poll_interrupts` called `cpu.request(5.5)` and nothing ever
cleared `pending[5.5]`. **RST5.5 is level sensitive on the 8085**, so a model must
drive it low again when RxRDY deasserts; latching it makes the handler re-enter
forever on stale data, which looks exactly like a timeout.

Fixed by assigning `pending[5.5] = bool(status & RXRDY)` each step, and documented
the level-vs-edge distinction in `emu8085.request()`.

**Two earlier findings re-checked against the fix:**

* **Retracted:** the stack reaching `0x9FFF` at `0x2C80`. Same bug — runaway ISR
  nesting. The stack now peaks at `0xA0C1`, 50 bytes of the 243 available, with
  zero writes outside mapped memory. Nothing to explain, and the TODO item is
  gone.
* **Survives:** the absence of a power-on panel sweep. Still only eight
  display-port writes in two million steps with no input, unaffected by the bug.
  The hardware-sequencer hypothesis stands.

The lesson worth keeping: an emulator finding needs a mechanism before it becomes
a documented fact. The crispness of "24.8 ms" was what made it convincing, and
crispness is exactly what a software artefact produces.

**A real finding did come out of the driver work.** CP437 carries the *lowercase*
accented vowels but of the uppercase ones only `É`, plus `Ä Å Ö Ü Æ` and `Ñ`.
**There is no `Á`, `Í`, `Ó` or `Ú`** — so `CHAMARTÍN` cannot be sent as written,
and station names are usually uppercase. `Ñ` and `É` happen to be the two Spanish
most needs. This is plausibly *why* `0x24` and `0x25` carry substitute glyphs:
`0x25` is a second `Ñ` for the 7-bit link, and `0x24` is likely one of the missing
accented capitals. The driver refuses rather than sending a wrong glyph, with
`fold=True` to drop impossible accents.

---

## 2026-08-06 — The motherboard carries the Telenorma logo

**Recorded (owner):** the mother PCB bears the Telenorma logo.

**This is a bigger shift than the EPROM sticker was.** Yesterday's reading had
Telenorma supplying only the *firmware*, with PAGE Ibérica having built the board.
A logo on the silkscreen moves them to **hardware designer and manufacturer**.
Two independent marks now:

| Evidence | Covers |
| --- | --- |
| `TELENORMA 18 JUN 96` on the EPROM | the firmware |
| Telenorma logo on the motherboard | the hardware design and build |

**Revised division of labour:** Telenorma designed and made the control
electronics and wrote the firmware; PAGE Ibérica supplied the *sign* — enclosure,
flip-dot panel, fluorescent lighting, the Spanish market — and put its plate on the
outside. The plate does read `FABRICADO`, so manufacture under licence or assembly
around bought-in Telenorma boards are both plausible. `MAESTRA` is PAGE Ibérica's
Spanish label on a Telenorma product.

**The practical payoff: the silkscreen numbering is German-industrial and highly
searchable.** Read off the photographs (and needing verification against the
board):

* Mother: `300-48.0216.5586` / `324-48.0216.5586` rev `A.2`, `41.0222.0026 L1 A.1`
* Daughter 1: `300-40.0219.9078` / `324-40.0219.9078` rev `A.3`, `11.0238.4030`,
  `37623.6`

The paired `300-`/`324-` prefixes on the same number probably distinguish bare
board from populated assembly, in which case `48.0216.5586` and `40.0219.9078`
are the real design numbers. **[L]** These are far more specific than any product
name and are now the best documentation lever the project has — daughter 2 has
not been transcribed at all yet.

**Also read from the motherboard photo:** the two 8-position DIP banks sit
immediately beside the M81C55-5, confirming they are the ports A and B switches;
the lone separate switch is designated **`S17`**; connector designators are
`STA`-`STD` plus `ASTU1`.

**And a consequence worth stating:** this is definitively *not* a RENFE one-off.
Telenorma-designed hardware implies a product family deployed wherever Telenorma
sold, very plausibly including German stations — so identical boards may survive
elsewhere, and the protocol is a documented product bus rather than a bespoke
Spanish arrangement. That also keeps nudging the electrical question towards the
20 mA current loop common in German time-distribution installations.

---

## 2026-08-06 — Switch positions as found, and a convergence on 7 data bits

**Recorded (owner)** — the P0 item, the one piece of state no amount of later work
could have recovered:

| Where | As found (1 = closed) |
| --- | --- |
| Bank A', beside the 81C55 | `1 0 0 0 0 1 0 1` |
| Bank B', beside the 81C55 | `0 1 1 1 1 1 0 1` |
| Single switch elsewhere on the motherboard | open |
| Two switches on daughter 2 | both open |

**Three unknowns sit between those digits and the port values**: polarity, bit
order, and which bank is which port. That is eight self-consistent readings,
tabulated in `docs/dip-switches.md`. Two arguments narrow them:

* **Closed almost certainly reads 0** — the near-universal DIP arrangement is a
  pull-up on the pin with the switch shorting to ground. Halves the table.
* **Seven data bits would explain the second `Ñ`.** This is the interesting one.
  `docs/font.md` already recorded that `0x25` holds a duplicate `Ñ` inside the
  7-bit range, noted at the time as "what a Spanish installation needs when the
  link is not 8-bit clean". If port B really is set for 7 data bits then that is
  not a coincidence but the *reason*: a 7-bit link physically cannot carry `0xA5`,
  the CP437 `Ñ`. **Two findings reached from opposite directions agreeing.** [L]

Together those favour **address 1, command protocol, 1200 baud, 7 data bits, even
parity** — an unremarkable 1990s industrial-bus setting, and a sensible one for
the first sign on a platform. Still **[?]**.

**No guessing will ultimately be needed, because the device reports its own
switches.** The status frame's last byte is `0x60 | (port A & 0x1F)` — port A bits
4:0 with no assumption about polarity or bit order — it is sent at the
switch-selected baud rate, and its character format on the wire is the
switch-selected one. **One clean capture of `18 18 1E 15 <addr>` resolves almost
the entire question** and cross-checks the eight-row table.

**A structural finding from the counts.** 16 + 1 + 2 = **19 switches against the
81C55's 22 input bits**. So the two banks of 8 are ports A and B, and **port C is
not on a DIP bank at all** — the lone motherboard switch plus hardwired links.
That fits exactly what port C does: bits 4:0 select the display *type* and bit 5
is the serial enable, both factory decisions, while ports A and B carry the
address, baud rate and character format an installer sets. [L]

**And a testable inference:** the unit was in service and receiving, so port C bit
5 must have read as enabled. The lone motherboard switch was **open**. If that
switch is bit 5, open must read as 1 — independently confirming the pull-up
polarity argued above. [L]

---

## 2026-08-06 — Dump provenance closed out

**Confirmed (owner):** a second independent read of the EPROM produced an
identical SHA-256, and the UV window is covered by the TELENORMA sticker.

So the image in `original/` is now **verified rather than merely plausible** — two
reads agreeing rules out the marginal-cell and misread failure modes that the
structural checks (vector table, `0xFF` fill boundary, bit 7 clear across the
glyph table) could only make unlikely.

**One consequence worth stating loudly, because it cuts against the advice given
for the other sticker:** the TELENORMA label must **not** be peeled. It is doing
two jobs at once — it is the unit's primary date evidence *and* the UV shield over
the window. The DC-DC converter's QA sticker is the opposite case: it covers a
part number worth reading and shields nothing, so removing that one is fine.

Only the programmer model and software version remain unrecorded, and nothing
depends on them now.

---

## 2026-08-06 — The EPROM says TELENORMA 18 JUN 96

**Recorded (owner):** the ROM chip carries a sticker reading
**`TELENORMA  18 JUN 96`**. A quick search identifies Telenorma as a German clock
manufacturer that made station clocks.

**Two distinct findings, and the second is the bigger one.**

**A date.** 18 June 1996 is the **first hard date found anywhere on the unit** —
the chassis plate carries a serial (`985412?`) but no date — and it dates the
*firmware*, which is the date that matters most here. It also fits the other
weak signals: the DC-DC converter's `970593-960119980401` marking may embed a
`9601` code, and its QA sticker reads `30-01-9?`. A mid-1990s build throughout.

**A different company than expected.** The label is on the *programmed EPROM*,
not the chassis. Telenorma descends from **Telefonbau und Normalzeit** of
Frankfurt — "telephone construction and standard time" — which built public and
station clock and **time-distribution installations** alongside telephone systems,
later becoming part of Bosch Telecom. **[L]**, recalled rather than sourced, and
flagged for verification.

The economical reading: **the firmware came from Telenorma while PAGE Ibérica
built the sign** — licensed design, OEM arrangement, or PAGE Ibérica as the
Spanish partner in a Telenorma installation. **[?]**

**Three consequences.**

1. **This is probably not a RENFE one-off.** A time-and-information distribution
   product from a clock-systems firm implies a family with a documented bus,
   deployed in more than one country. That makes finding the original
   specification a realistic prospect rather than a fantasy — and it fits what
   the protocol already looks like: ASCII control codes, `GS` addressing,
   `STX`/`ETX`, `ACK`/`NAK`, an identify command, cursor positioning. That is a
   generic intelligent-terminal bus, not something bespoke.
2. **The documentation hunt has a new front.** Telenorma / Telefonbau und
   Normalzeit / Bosch Telecom, in German, rather than only Spanish sources.
   Search terms recorded in `docs/references.md`.
3. **It quietly supports the current-loop theory.** German clock and
   time-distribution buses of that era commonly ran on **20 mA current loops** —
   independently where the daughter-1 component analysis already points. **[L]**

**Housekeeping:** this supersedes the earlier note that the converter's QA
sticker would be "the only date evidence on the board". It is now corroboration
rather than the sole record. Also added a check that the EPROM's UV window is
covered — a 1996 part with an exposed window is worth protecting.

---

## 2026-08-06 — U19 is a third ULN2003A: 21 sinks, and no source drivers

**Recorded (owner):** U19 on daughter 2 is also a ULN2003A. So the board carries
**three** of them — U18a and U18b from TI, U19 from ST — for **21 Darlington
sinks**.

**Two consequences, one numerical and one topological.**

*Numerical:* the 81C55's three ports give 8 + 8 + 6 = **22 output bits** against
21 sinks. That near-match looks deliberate rather than coincidental — the ports
probably drive the Darlington inputs close to one-for-one, leaving a single bit
for a strobe or enable. Previously recorded as 14 sinks, which made the
arithmetic look nothing like the port width.

*Topological, and more interesting:* **the ULN2003A can only sink.** It pulls a
line to ground and cannot source. A flip-dot needs current through its coil in
both directions to set and reset, so sinks alone cannot flip anything. There are
no source drivers on the board at all. Options:

1. High-side drive lives **on the panel**, with the ribbon carrying only
   low-side selects.
2. **Two windings per dot** — a set coil and a reset coil, each returning to
   ground through its own sink from a common +24 V. Needs only sinks, which is
   exactly what the board has.
3. A split supply with the coil common at a mid-rail.

Option 2 is the tidiest fit for the parts actually present, but it stays **[?]**
until the ribbon is traced. Whatever the scheme, 21 lines cannot address 560 dots
directly, so the panel must carry its own decoding or shift registers.

**Also worth noting:** the ULN2003A's built-in free-wheel clamp diodes (common
cathode, pin 9) exist for inductive loads. Their presence is quiet confirmation
that the panel is driven as coils.

---

## 2026-08-05 — ESP32 added to the controller comparison

**Did:** Worked the ESP32 into `docs/controller.md` alongside the Pi and Arduino.

**Concluded it is the best fit of the three**, and not merely as another option
that happens to work:

* **Parity is the discriminator.** The link is 8E1 by default. Arduino's
  `SoftwareSerial` has no parity; the Pi's `/dev/ttyS0` mini-UART has none either
  and needs Bluetooth disabled to reach the PL011. The ESP32 has three hardware
  UARTs, all supporting `SERIAL_8E1`, so the problem simply does not arise.
* **It can replace the original controller outright.** Since the display computes
  nothing — no arithmetic, no digit formatting, no minute timer — the countdown
  and the destination text were always the upstream unit's job. An ESP32 with
  WiFi and NTP can do exactly that job, with no PC in the loop.
* **It fits inside the enclosure** off the existing 24 V rail via a small buck,
  keeping the finished object a self-contained sign.
* If the link is 2-wire RS-485, the ESP32's UART has a native half-duplex mode
  that drives DE in hardware.

**The catch worth writing down: 3.3 V logic.** Everything on the display side is
5 V-era CMOS.

* Use **MAX3485 / MAX3490**, not the 5 V MAX485/MAX490 that most cheap breakout
  boards carry. A 5 V module will accept 3.3 V as a logic high but its receiver
  output swings to 5 V into a 3.3 V pin — the standard way to kill an ESP32 on
  this sort of project.
* For a current loop, rescale the opto drive: the original uses 560 Ω from 5 V for
  ~6.5 mA; from 3.3 V that gives only 3.5 mA. **220-330 Ω** reproduces the
  original drive and stays inside the ~12 mA per-pin continuous limit.

**Recommended division of labour:** bring the link up on a PC with a USB adapter,
where iteration is fast and everything can be logged, then move the working
protocol onto an ESP32 for the permanent installation.

---

## 2026-08-05 — Daughter 1's parts list argues for a current loop

**Recorded (owner):** besides the listed ICs and the DC-DC converter, daughter 1
carries 4 diodes, 2 electrolytics, 8 film capacitors, 3 x 560 Ω and 3 resistors
whose bands read brown(wide) / brown / black / red / brown in an uncertain
direction.

**On the ambiguous bands.** Five bands with one wider means a 1% part and the
wide band is tolerance. The candidate readings are 1.2 kΩ, 1.02 kΩ and 1.0 kΩ —
all valid E96 values, all within 20% of each other. **The exact reading barely
matters; they are ~1 kΩ.**

**The strongest inference is topological, not numerical.** Daughter 1 has a
DC-DC converter, a hex Schmitt trigger, six probable optocouplers, resistors,
diodes and capacitors — and **no differential transceiver IC of any kind**. An
RS-422/485 interface of this era would use a 26LS31/26LS32, 75174/75175 or
similar. A hex Schmitt trigger driving optocouplers is not how anyone builds
RS-422; it is exactly how everyone builds a current loop. Nothing near 120 Ω is
present either.

**The values fit, with a caveat that must not be glossed over.** 560 Ω from 5 V
gives ~6.5 mA, a comfortable opto LED drive on the logic side. ~1 kΩ from 24 V
gives ~20 mA, textbook loop current. But the same ~1 kΩ from 5 V gives 3.6 mA and
would say nothing at all. **The resistor value does not settle it — the node
voltage does.** Measuring the DC-DC converter's isolated output is now the single
highest-value measurement on the board.

**A mechanism for the scope transitions.** An isolated DC-DC converter means the
line side has its own supply, so the transmitter is *active* rather than a bare
passive switch. That is precisely why transitions were visible where a passive
loop would have shown nothing. An active-transmit, active-receive current loop is
consistent with everything observed: the optos, the missing transceiver, the
~20 mA resistors, the isolation, and the visible edges.

**Also noted:** three of each resistor value, not two. That hints at **three
signals per direction** — perhaps TX, RX and RTS, or a second channel for the LED
matrix. Worth not assuming a simple two-signal link when tracing.

**Concluded:** current loop is now the leading theory **[L]**, up from a coin
flip, on the strength of the missing transceiver rather than the resistor values.
Still **[?]** until the converter output voltage is known.

**Converter pinout (owner):** `VI+`, `VI-`, `VO+`, `VO-`, each doubled into two
connected through-holes. The doubling is ordinary brick practice — current and
mechanical anchoring — and says nothing. **But four nets with no shared ground
does say something [C]:** a buck or linear regulator must share a return between
input and output and would expose three pins. Two independent pairs means the
output floats galvanically with respect to the display's 0 V, confirming the
module exists to power an isolated line side. That is the premise the whole
current-loop reading rests on, and it now stands on its own rather than being
assumed.

Flagged before the sticker covering the part number is removed: photograph it
first. It looks like a handwritten QA mark, `30-01-9?` with `OK`, which would be
the **only date evidence on the board** — the chassis plate carries a serial but
no date. Peeling it destroys that.

---

## 2026-08-05 — 6.144 MHz crystal: the baud ladder is now exact

**Observed (owner):** a **6.144 MHz crystal** on the motherboard beside the CPU.

The 8085 halves its crystal internally, so the machine runs at **3.072 MHz** and
CLK OUT is 3.072 MHz — the conventional feed for the 81C55 TIMER IN. With
`baud = TIMER_IN / (divider x 16)` the switch-selected dividers give:

| Divider | 20 | 40 | 80 | 160 | 320 | 640 |
| --- | --- | --- | --- | --- | --- | --- |
| Baud | 9600 | 4800 | 2400 | 1200 | 600 | 300 |

**All six exact, zero error.** That is itself the confirmation: an arbitrary
TIMER IN would not land on six standard rates at once. This was the prediction
recorded in `docs/dip-switches.md` before the crystal was read, so it is a clean
hit rather than a fit after the fact. Upgraded from **[L]** to **[C]**.

One link is still assumed rather than traced: that TIMER IN comes from CLK OUT
and not from the raw crystal, which would double every rate. A continuity check
from 8085 pin 37 to 81C55 pin 3 settles it.

**Knock-on: the firmware's delay loops now convert to real time.** Measured by
executing them in the simulator and counting cycles at 325.5 ns each:

* `FUN_1B4F`, one delay unit: 277 cycles = **90 µs**
* Line turnaround before a reply, `0x1ABD` with C=0x28: 12017 cycles = **3.9 ms**
* Inter-character gap: **zero in this build** — both configurable loops in
  `0x1A1A` are loaded with `MVI C,0x00` and skipped entirely, so characters go out
  back to back. Another value that looks meant to be patched per installation.

So a host must allow **at least ~4 ms before the first reply byte**, plus the
frame time — 5.7 ms at 9600 baud, 183 ms at 300. A `SUB` reply at 300 baud takes
about 1.8 seconds.

**Also:** calibrated `pit100_sim.py` with `CPU_HZ = 3_072_000`, so boot reports
simulated milliseconds instead of bare instruction counts. The 200k-step boot is
487 ms of real device time.

---

## 2026-08-05 — Power-on sweep, and what the emulator says about it

**Observed (owner):** on power-up a brief buzzing sound sweeps across the panel.
No visual change on this unit because its dots were already all black.

**What it settles:**

* The rest state is **black**, and power-on drives every dot to it.
* The sweep is **sequential** — the sound travels, so coils fire in small groups.
  Two ULN2003A packages could not drive 560 dots at once anyway.
* The firmware **does not know the panel state at power-on and does not try to**.
  Flip-dots are bistable, so after a power cut the dots are wherever they were
  left while RAM comes up cleared. Forcing every dot is the only way to make the
  two agree. A controller therefore cannot assume the panel survives a power
  cycle.
* It is audible proof the drive electronics work — a bring-up signal available
  before any serial success.

**Checked it against the emulator, and got a useful negative.** Booting the real
ROM produces only **eight** display-port writes, regardless of switch settings,
over four million instructions and with the periodic interrupt driven at several
rates: the `0xE0` command word, a `0x80`-then-`0x00` pulse on ports A and B
around `0x03FF`, and three zeros at `0x068D`. **No sweep.**

**Concluded:**

* The observation is **[C]**; the emulator's silence is **[C]** as a fact about
  the model, not about the hardware.
* Best explanation **[L]**: daughter 2 carries a **hardware sequencer** that
  performs the sweep autonomously, strobed by that short pulse at `0x03FF`. That
  would explain both the audible scan and the complete absence of per-dot writes
  from the CPU. Previously the pulse was guessed to be a "reset/home pulse" — the
  buzz corroborates that reading and sharpens it.
* Alternatively the render path waits on a hardware handshake the model stubs.
  Distinguishing the two needs a scope on the ribbon.

**Also found while chasing this:** port C bits 4:0 are a **display-type
selector** — a 32-way dispatch at `0x055A`, with seven values having distinct
handlers. One firmware, several PI-T100 variants.

**Cheap follow-up worth doing:** time the buzz. Duration / 560 dots gives the
per-dot drive time, which `docs/flipdot-interface.md` has been carrying as an
open question. A one-second sweep would mean ~1.8 ms per dot.

---

## 2026-08-05 — Operational context, and the display computes nothing

**Did:** Recorded how the sign works in service, and checked whether any of that
behaviour lives in this ROM.

**Operational picture** (from the owner): the flip-dot line shows the train's
final destination plus the ETA to this station in minutes, counting down. A vinyl
sticker reading `min` sits to the right of the panel, so the last one or two
characters are the countdown. A separate LED matrix below scrolls upcoming
stations and notices. A fluorescent tube lights the panel, with its ballast
inside the body and its own mains feed — presumably switched with the street
lighting, while the PSU feed stays live.

**Found — this device computes nothing.** Checked directly:

* There is **no minutes-to-ASCII conversion** in the entire firmware. The only
  two `ADI 0x30` instructions are in the `TAB` cursor-position report.
* No minute-scale constants, no ASCII-digit decrement, no real-time counter
  beyond the RST7.5 watchdog.

So the **countdown is pushed from upstream** as plain text; the sign does not
tick down by itself. What *is* local: glyph rendering, the flip-dot drive, the 8
display effects and their durations, addressing, framing and ACK/NAK error
reporting.

**Concluded [C]:** a replacement controller must send the whole 16-character
line including the digits, and re-send it whenever the ETA changes. The `min`
sticker being physical confirms the layout is the *controller's* convention, not
something this board knows about.

**On the scope attempt** (clarified by the owner): **transitions were visible**,
but the signal was unclear with an AC component — not a null result, which is how
it was first recorded here. Three consequences:

* Something is actively driving those pins, and the only thing this unit emits
  unprompted is its status frame. So **the pins showing transitions are the
  transmit side**.
* **The transmitter is not a bare passive switch.** A passive 20 mA loop
  transmitter is an optocoupler output with no supply of its own and cannot make
  voltage transitions into an open circuit. That weakens the plain
  passive-current-loop theory.
* Most likely cause of the unclear trace: the unit transmits *continuously*, so
  at a slow timebase a non-stop serial stream looks exactly like a messy AC
  waveform. Add an undefined reference across the opto isolation and the ballast
  hum, and that fully accounts for it.

**Next:** capture one frame with the ground clip on the other conductor of the
same pair, single-shot triggered, and decode it against the known
`18 18 1E 15 <addr>`. That is a known-answer test which settles baud, polarity and
framing at once.

---

## 2026-08-05 — The whole command protocol, validated end to end

**Did:** Decoded the remaining handler-B commands, then drove the real firmware
in the simulator and captured its replies.

**The unblocking discovery: port C bit 5 is a serial enable.** Two sites test it
— the init path at `0x0430` *and* the RST5.5 ISR at `0x0268`. With it clear the
ISR returns immediately, so **the unit never receives anything**. Every earlier
experiment ran with bit 5 clear (`--port-c 0xDF`) and was therefore testing a
deaf machine. That also reframes the "9600 8E1 default": that configuration
belongs to a unit that cannot receive. A working installation uses the port B
switches.

**Second key discovery: the unit must be addressed before it listens.** Handler B
guards its dispatch table on a *selected* flag at `0xA040`. Until it is set, only
`EOT`, `VT` and `GS` are accepted. `GS <addr>` selects — comparing against the
port A address, with **`0x7F` as a broadcast** — and `RS` deselects. So every
transaction is `GS <addr> <commands> [RS]`.

**The command set is a small cursor-addressable terminal.** The 64-byte buffer is
rows of 64 columns with the cursor in `0xA043`/`0xA044`: `VT <row> <col>`
positions (two ASCII-digit parameters), `CR`/`LF`/`DC1`/`DC2`/`DC3`/`DC4` move,
`ETB`/`EM` erase to end of line/display, `CAN` clears, `FF` fills with `#`.

**Five commands make it answer with real data**, each wrapped in ROM-defined
prefix/suffix strings: `SUB` identifies, `TAB` reports the cursor, `SO` reads the
character at the cursor, `SI` reads back the display, `BS` reports status.

**Verified in emulation** — actual captured replies:

```
GS '5' SUB           -> 16 16 16 16 "AK0052.139U1" + the config block
GS 0x7F SUB          -> identical (broadcast works)
GS '5' TAB           -> "00"
GS '5' STX HOLA ETX  -> ACK '5'
GS '5' ... SI        -> ACK '5' then the buffer contents
GS '9' SUB           -> nothing at all
```

**Concluded:**

* All of it **[C]**: read from the listing *and* reproduced by executing the ROM.
* `SUB` is the best bring-up command — no prior state, works under broadcast, and
  returns a known-answer string while dumping the unit's own config block.
* `ESC` is **disabled in this build** (`0x0E8C` requires ROM `0x0077 == 1`; it is
  `0x00`).

**Also closed port A completely** by enumerating every `IN 0xA1` site: bits 4:0
address, bit 5 selects the blank-substitute character (`'?'` or space), bit 6
picks the receive protocol, bit 7 is a render option. And `0x055A` computes
`0x60 | (port A & 0x1F)` into `0xA006` — **handler A's start-of-frame byte is the
display address**, the same value the status frame carries as its last byte. The
two protocols share one addressing scheme.

**Next:** the `EOT` sub-modes and what `BS` returns; then the render path and the
8 display attributes, which need the panel to interpret.

---

## 2026-08-05 — The code page is IBM CP437

**Did:** Rendered the whole glyph table to a PNG map (`make font-map`) and read
it, then spot-checked individual code points against the raw bytes.

**Found:** **The encoding is IBM Code Page 437**, and every region of the table
agrees: `0x01-0x0F` smileys and card suits, `0x10-0x1F` the arrow set,
`0x80-0x9F` the accented Latin block, `0xA0-0xAF` ending in `¿ ⌐ ¬ ½ ¼ ¡ « »`,
`0xB0-0xDF` shading and box drawing, `0xE0-0xEF` Greek, `0xF0-0xFE` maths.
Byte-level spot checks: `0xA5` is an unmistakable `Ñ`, `0xA8` `¿`, `0xAD` `¡`,
`0x82` `é`.

**Two substitutions**: `0x24` and `0x25` are *not* `$` and `%`. Both are accented
Latin capitals, and `0x25` reads clearly as a second `Ñ` — exactly what a Spanish
installation needs when the link or the controller is not 8-bit clean. `0x24` is
another accented capital of uncertain identity.

**Concluded:**

* CP437 for `0x80-0xFF` and the control ranges **[C]**. This **corrects**
  `docs/font.md`, which previously said "do not assume Latin-1 or IBM code page
  437" — the evidence now says otherwise, across seven independent regions.
* Practically: a controller can just use `text.encode("cp437")`, but must avoid
  `$` and `%`.
* `0x24`'s identity **[?]**.

**Next:** the remaining handler-B commands, and what the 8 display attributes do.

---

## 2026-08-05 — Handler B command semantics; error reporting and a config block

**Did:** Followed the dispatch targets for STX, ETX, CAN and RS, and the state
variables that turn some commands into parameterised ones.

**Found:**

* **The message buffer is `0xE001-0xE040`, 64 bytes**, and the length is read
  from the ROM constant at `0x007B` rather than hard-coded. Overflow is caught at
  `0x0C41`: the pointer is clamped and error code `0x30` latched, so an
  over-long message gets NAKed instead of silently truncated. A second buffer
  sits at `0xE084`. 64 bytes for a 16-character panel means the surplus is for
  scrolling.
* **The reply is richer than just ACK/NAK.** Full frame:
  `[4xSYN] <ACK|NAK> <address> [errcode [param1 param2]]`. Error codes are
  printable ASCII; `0x30` is overflow, `0x32` is another bare failure, and any
  other code carries two detail bytes from `0xA050`/`0xA051`. A NAK says *why*.
* `0x1A7F` does line turnaround before replying: re-issues the 8251A command
  word to drive RTS, then optionally four `SYN` bytes or a fixed delay. **That
  explains why `0x16` SYN has no receive handler — it is a transmit-side
  preamble.**
* Command semantics: **CAN** clears the display (fills 64 bytes with spaces, or
  the `0xE084` buffer with zero, then posts `0x4A` to repaint). **RS** resets the
  receive state but is *ignored when the unit's address is 0*. **DLE** resets the
  unit. **ACK**, oddly, also clears the display.
* **Three commands take a parameter byte** via a state variable, which is what
  those state variables at the top of handler B are for: `BEL` (low 3 bits of the
  parameter become the display attribute `0xA014` — 8 effects), `ENQ` (parameter
  to `0xA041`, with DC2 translated to `'#'`), and `GS` (parameter is an address,
  matched against port A). That is why `0x1D` GS never appears in the dispatch
  table — it is consumed a level earlier.
* **`0x0070-0x008B` is a build-time configuration block.** Buffer length, address
  mode, turnaround options and feature bits all come from there. The same
  firmware was clearly customised per installation by patching these bytes.

**Concluded:**

* All of the above is **[C]** from the listing.
* **Caveat worth carrying forward:** anyone building a controller should read the
  config block out of *their own* ROM. A different PI-T100 may be configured with
  a different buffer length, addressing mode or turnaround behaviour, and nothing
  in the protocol announces it.

**Next:** the remaining ~19 handler-B commands, and what the 8 display attribute
values actually look like on the panel.

---

## 2026-08-05 — Handler B is a command protocol; the unit answers ACK/NAK

**Did:** Extracted the handler B (`0x0ACB`) dispatch table, followed the ETX
path, and used the simulator to sweep DIP switch port A.

**Found:**

* **Handler B is a 28-command protocol** keyed on ASCII control codes, with a
  multi-state parser: several state variables each divert following bytes to a
  different continuation, so commands take parameters. `STX` opens a message,
  `ETX` closes it, and **`DLE` jumps straight to `fatal_or_init` — a reset
  command**. `0x01`, `0x15`, `0x16` and `0x1D` have no handler, so they are
  transmit-side only.
* **On ETX the unit replies `ACK` (0x06) or `NAK` (0x15)** depending on an error
  flag accumulated at `0xA03A`, then sends its address. This is the most useful
  single fact found so far: it turns protocol discovery from reading into
  experiment, and gives an unambiguous link test that does not depend on the
  panel being healthy.
* **DIP switch port A bits 4:0 are the display address.** Confirmed twice: from
  `0x0F69`, which encodes them as printable ASCII (`0x30-0x3F` when bit 4 is
  clear, `0x40-0x4F` when set — 32 addresses); and by experiment, sweeping port A
  in the simulator and watching the transmitted frame's last byte track
  `0x60 | (port A & 0x1F)` exactly.
* Port A bit 6 selects which receive protocol runs. That accounts for most of
  port A; bits 7 and 5 remain unknown.
* The continuously transmitted frame decodes as **`CAN CAN RS NAK <0x60|addr>`**,
  with a second variant at `0xA0|addr`.

**Concluded:**

* The command table and the ACK/NAK behaviour are **[C]**. What each individual
  command *does* is still **[?]** — the dispatch targets are known, the semantics
  are not.
* The status frame is **[C]** as an observation. Calling it a beacon rather than
  a poll is **[L]**, and it cuts against the MAESTRA reading: this unit may be
  master of its *display chain* while being a slave on the *serial bus*.
* Revised yesterday's guess that the frame was a master polling slaves.

**Next:** decode the individual handler-B commands, starting with `0x0F17` STX,
`0x0F29` ETX, `0x101E` CAN and `0x10A3` RS. Then the `0xA014` attribute bits.

---

## 2026-08-05 — Receive handler A decoded; emulator running the real ROM

**Did:** Disassembled `rx_handler_a` (`0x08B7`) and built an 8085 emulator
(`tools/emu8085.py`) plus a machine model (`tools/pit100_sim.py`) that boots the
actual firmware against stub peripherals.

**Found — the framing, from the listing:**

* **Start and end delimiters are runtime variables**, not literals: `0xA006` and
  `0xA007`. A second start byte (`0xA006 + 1`) selects an alternate buffer at
  `0xE03C`. Messages accumulate at `0xE000`, pointer in `0xA00E`.
* On the end byte the handler posts **`0x4A`** to the mailbox — that is the
  "message complete" signal the main loop waits for at `0x03A0`.
* Per-character filtering is layered: bit 7 set means skip-but-advance; `0x00-0x1F`
  is ignored; `0x60-0x7F` changes state without storing; a configurable byte at
  `0xA00B` stores as a space.
* In-band control characters `0x7C`, `0x7D`, `0x7F` and (in an extended sub-mode)
  `0x64`/`0x65`/`0x66` write an attribute byte `0xA014` and duration words
  `0xA016`/`0xA018`. These are display effects with timings.
* **A parity error inserts `0x23` `#` into the message** and re-issues command
  `0x17`. Externally observable, and a good live-link test.

**Found — from running it:**

* The emulator reproduces the documented cold-boot 8251A init exactly: mode
  `0x7E`, command `0x37`, with port C bit 5 clear. Independent confirmation that
  both the listing reading and the model are right.
* With port C bit 5 *set* the switch-derived path produces mode `0xFA` instead,
  as `docs/dip-switches.md` predicts.
* **The firmware transmits continuously**: a 5-byte frame `18 18 1E 15 7F`, with
  `18 18 1E 15 BF` periodically substituted. Only two distinct frames in ~2000
  bytes. Given the MAESTRA label this looks like a poll or beacon, and the last
  byte differs only in bits 7:6 — an address or flag field. Note `0x7F` is also
  what `0xA006` holds.
* Both transmit routines poll TxRDY before writing. The one at `0x1A1A` runs
  configurable inter-character delay loops first, which is what a half-duplex or
  current-loop link needs for turnaround.

**Concluded:**

* Framing structure **[C]**; the meaning of the individual `0xA014` attribute
  bits **[?]**.
* The beacon is **[C]** as an observation of the firmware, **[L]** as "a poll".
* **The firmware writes to `0xE000-0xE083` even with no expansion board and
  handler A selected.** So something must answer there. The simulator offers
  `none` / `board` / `alias` and reports unbacked writes; deciding between them
  needs the hardware.

**Next:** handler B (`0x0ACB`), then the `0xA014` attribute bits. Also worth
chasing: ~120 emulated writes land at `0x9FFD-0x9FFF`, i.e. the stack dipping
below `0xA000`. Either a model artefact or a real clue about RAM size.

---

## 2026-08-05 — Baud ladder, stack, and the receive architecture

**Did:** Worked through the "cheap wins, no hardware needed" list in
`docs/open-questions.md`. All of it was mechanical listing reading.

**Found:**

* **The complete baud table.** The five unread branch targets give dividers
  40, 80, 160, 320, 640 alongside the default 20 — an exact 1:2:4:8:16:32
  ladder. `baud = TIMER_IN / (divider x 16)`, so one frequency measurement fixes
  all six rates. 3.072 MHz gives 9600 down to 300.
* **Port B is fully attributed.** `0x04EA` builds the 8251A mode word from port
  B: bits 7:5 baud, bit 4 word length (7 vs 8 data bits), bits 3:2 parity
  (off / odd / even). The hardwired `0x7E` (8E1) is what the common switch
  setting produces.
* **Stack is at `0xA0F3`** (`LXI SP,0xA0F3` at `0x034C`), growing down. The RAM
  clear stops at `0xA0EF`, so it never touches the stack. The phantom Ghidra
  symbols at `0xA0ED`/`0xA0EF`/`0xA0F1` are stack slots.
* **RST5.5 is the serial receive interrupt.** It masks 5.5 and 6.5 but leaves
  7.5 unmasked and re-enables interrupts, so the tick can preempt reception. It
  is gated by DIP switch port C bit 5 — that switch turns reception off
  entirely. It then dispatches on `0xA00C` to **one of two receive handlers**,
  `0x08B7` or `0x0ACB`, both of which touch the 8251A.
* `0xA00C` is set at `0x0689` from either the expansion board or DIP switch port
  A bit 6. So there are **two protocols or two link roles**, chosen by
  configuration.
* **RST7.5 is a tick and watchdog**, not part of the receive path: reads `0xCD`,
  decrements `0xA01E`, and jumps to `fatal_or_init` on expiry. It also drives the
  8085 SOD pin as a GPIO output (`SIM` `0xC0` sets it, `0x0344` clears it).
* `0x4A`/`0x4E` at `0x03A0` are an **ISR/main-loop mailbox**, swapped atomically
  under `DI`/`EI`. The earlier guess that they might be `'J'`/`'N'` framing
  delimiters is wrong.

**Concluded:**

* Everything above is **[C]** from the listing. The only remaining unknown on the
  link layer is the absolute clock frequency, and the ladder's exactness makes
  even that low-risk.
* **Revised an earlier claim:** `0xC0-0xCF` was labelled "LED matrix interface,
  board absent". RST7.5 reads `0xCD` every tick and there is no absence check
  anywhere for `0xC0` — unlike the careful probing done for `0xE7`. Either the
  device is on the motherboard, or RST7.5 never fires here. Downgraded to **[?]**
  pending a scope probe on the RST7.5 pin.

**Next:** disassemble `0x08B7` and `0x0ACB` in full. The framing lives there and
needs no hardware. Then attribute DIP switch port A, which is the likeliest home
of a display address.

---

## 2026-08-05 — I/O port 0xE7 is a memory bank register, not a peer handshake

**Did:** Chased every one of the 11 `IN 0xE7` / 10 `OUT 0xE7` sites, after the
one-line "board detect" claim in the first pass was questioned.

**Found:** `0xE7` is a **bank/page register plus capability register on the
expansion board**, and the earlier description of it was too shallow.

* Three accessor functions wrap every banked memory access: `0x1B69` (read,
  21 callers), `0x1B95` (write, 21 callers), `0x1BC4` (compare, 4 callers).
* Each one reads `0xE7` first. `0xFF` means the expansion board is absent, and
  the accessor falls straight through to a plain `(HL)` access on local RAM.
  Otherwise it writes the target address's high byte to `0xE7` as a page select
  and forces the address into the `0xE000-0xFFFF` window with `ORI 0xE0`.
* `0xA05A` is set to 1 inside the ISRs (`0x0271`, `0x0296`); when set, the
  accessors also save the previous page in `0xA059` and restore it afterwards.
  That is re-entrancy protection, so an interrupt cannot leave the page
  switched under the code it interrupted.
* `0xE7` also carries status bits when a board is present: `0x112E` tests bit 4.
* This explains the stray `DAT_ram_e000`, `DAT_ram_e03b`, `DAT_ram_e07d`
  references in the Ghidra decompilation — they are banked addresses, not real
  locations in this machine.

**Concluded:**

* **There is no handshake with the expansion board, and no possibility of one.**
  All 11 read sites are immediate conditional branches. Not one loops, retries,
  waits, or has an error path. **[C]**
* **Standalone operation is the designed behaviour, not a lucky accident.**
  `0x0675` is the clincher: with the board absent the firmware reads DIP switch
  port A bit 6 instead, deliberately substituting a switch for a value the board
  would otherwise have supplied. **[C]**
* Corrected `docs/io-map.md`, `docs/memory-map.md` (the `0xE000-0xFFFF` window
  was previously listed as "unmapped [?]", which was wrong),
  `docs/flipdot-interface.md` and `ghidra/symbols.csv`.

**Next:** unchanged — the serial framing is still the blocker. The `0xE7` bits
cannot be probed further without an expansion board, so that question is parked.

---

## 2026-08-05 — Project structure, and a first pass over the ROM

**Did:** Reorganised the repository (see [README.md](README.md#layout)) and read
the Ghidra export looking for peripheral accesses.

**Found:**

* 322 `IN`/`OUT` instructions across ports `0x80-0x81`, `0xA0-0xA5`,
  `0xC0-0xCF`, `0xDE`, `0xE0-0xED`. Everything is in 8085 I/O space; nothing is
  memory-mapped except the 81C55's RAM at `0xA000`.
* `0x0443` is a textbook 8251A wake-up: 3 dummy writes, reset `0x40`, mode
  `0x7E`, command `0x37`. Mode `0x7E` decodes to 8 data bits / even parity /
  1 stop / x16 clock.
* `0x03F2` initialises **two** more 81C55-shaped register groups at `0xE0` and
  `0xE8`, and `IN 0xE7` at `0x040F` is a presence check that skips the second
  group when it reads `0xFF`.
* The 81C55's ports A/B/C are only ever *read* → all three are the DIP switch
  banks. Port B bits 7:5 index a 6-way baud table at `0x0474`; port C bit 5
  picks between two startup paths at `0x0430`.
* Only `0x0000-0x3047` of the 32 KiB EPROM is programmed; the rest is `0xFF`.
* Region `0x1C0D-0x210C` is a 5x7 font: 256 entries x 5 bytes, column-major,
  bit 0 = top row. Verified by rendering — `'0'`, `'A'`, `'B'` come out right,
  and bit 7 is clear in all 1280 bytes.
* Chassis sticker: **PAGE Ibérica S.A., PI-T100-M, MAESTRA**, "1 línea de texto,
  módulos de PLACA MÓVIL de 100 mm".

**Concluded:**

* Link layer is **9600 8E1** **[L on the 9600, C on the 8E1]** — the 9600
  depends on the 81C55 TIMER IN being 3.072 MHz, which has not been measured.
* Font format **[C]**.
* Master/slave with a switch-set display address **[L]** — inferred from
  "MAESTRA" plus the second-board probe, not yet from the protocol code.

**Next:** the application-layer framing. Follow the `IN 0x80` sites back to
their buffer and forward to the state machine at `0x03A0`, which compares
against the constants `0x4A`/`0x4E`. Also fill in the five missing rows of the
baud table — pure listing reading, no hardware needed.

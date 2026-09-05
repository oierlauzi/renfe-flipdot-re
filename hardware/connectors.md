# Connectors

## DB37 — external control input

The only interface to the outside world. Of 37 pins, **4 conductors are
populated**, inside a grey overall-shielded cable (see
[photos/DB37_back.jpg](photos/DB37_back.jpg)). They land on a terminal block on
daughter 1.

| DB37 pin | Wire | Transceiver pin | Pair | Direction | Conf. |
| --- | --- | --- | --- | --- | --- |
| ? | **green** | `A` (6) | RX | **display RECEIVES** (host → display) | **[C]** |
| ? | **yellow** | `B` (7) | RX | **display RECEIVES** | **[C]** |
| ? | **blue** | `A` (6) | TX | display transmits | **[L]** |
| ? | **red** *(or orange — see below)* | `B` (7) | TX | display transmits | **[L]** |
| — | grey (shield) | — | — | chassis / drain | **[?]** |

**Only the DB37 pin numbers are still missing.** Everything else in that table is
traced.

> **A colour discrepancy to resolve.** The four conductors were first recorded as
> **orange**, blue, green and yellow; the pin trace reported **red** on the
> transmit pair. Orange and red are easy to confuse on a photograph, and only one
> of them can be right. **Check the fourth conductor against the cable**, because
> a wiring diagram that names the wrong colour is worse than one that names none.

### Attempt 1: oscilloscope, 2026-08 — transitions present, signal unclear

All four pins probed. **Transitions were visible**, but the signal was not clean
and appeared to carry an AC component. Two channels subtracted across candidate
pairs did not resolve a clean differential either.

> **All three conclusions drawn from this are retracted**, and the observation
> itself probably was not a signal.
>
> The reasoning was: transitions mean something is driving the line, the driver
> must be the unit's own status frame, therefore those pins are the transmit side
> and the transmitter is active rather than a passive switch.
>
> The premise is false. **The firmware transmits nothing unprompted**
> ([../docs/serial-protocol.md](../docs/serial-protocol.md#the-transmitted-status-frame-c)),
> so with the link quiet nothing was driving anything. And the line is now known
> to be an **unterminated, unbiased RS-485 pair**, which floats when undriven and
> picks up whatever is nearby — with a fluorescent ballast inside the same
> enclosure. "Transitions, unclean, with an AC component" is what a floating
> differential pair looks like. **[L]**
>
> So: the pins that showed transitions are **not** thereby the transmit side, and
> nothing here says whether the transmitter is active or passive. Both questions
> are open. See
> [the physical layer](#the-physical-layer-opto-isolated-rs-422rs-485-c).

### Why it looked unclean

Most likely causes, in order:

1. **There was probably no signal.** The likeliest reading now: an undriven
   RS-485 pair with no termination and no fail-safe bias, floating. Nothing to
   trigger on, and nothing to decode.
2. **No trigger.** Asynchronous data free-runs unless you trigger on an edge and
   use single-shot capture — but this only matters once the unit is actually being
   polled, since it never transmits on its own.
3. **Reference undefined.** U7 is an optocoupler, so the DB37 side floats
   relative to chassis. A ground clip on the chassis puts real transitions on top
   of a wandering hum baseline. Channel-subtract does not fix this — a scope's
   A−B has poor common-mode rejection compared with a differential probe.
4. **Mains hum.** The fluorescent **ballast is inside the enclosure**
   ([psu.md](psu.md)), so a live tube feed puts a strong 50 Hz inductive source
   centimetres from the probe.

### Attempt 2: capture one frame properly

**First: the unit must be polled, or there is nothing to capture.** It replies;
it does not announce itself. Until a host is driving the inbound pair, the only
thing on the outbound pair is whatever noise a floating line collects.

The interface is isolated and the far end is disconnected, so you may legitimately
**clip the probe ground to one conductor of the pair and probe the other**. That
defines the reference and turns the floating signal into an ordinary
single-ended waveform. (Safe here precisely *because* nothing else is connected —
scope ground is mains earth, so never do this on a live installed link.)

1. Disconnect the fluorescent tube feed to remove the hum source.
2. Ground clip on one conductor of the pair that showed transitions, probe on the
   other.
3. DC coupling. Trigger on an edge, **single shot**.
4. Sweep the timebase from ~200 µs/div down to ~20 ms/div until a repeating
   5-byte burst separated by idle resolves.

Then read three things off the capture:

| Measurement | Tells you |
| --- | --- |
| **Peak-to-peak amplitude** | Expect **±2-5 V differential**, the RS-485 range. |
| **Narrowest pulse width** | One bit time. Invert for the baud rate — which also settles the 81C55 TIMER IN question in [../docs/dip-switches.md](../docs/dip-switches.md). |
| **Idle level** | Mark polarity, and hence whether the line is inverted. |

Finally, **decode the frame by hand and check it against `18 18 1E 15 <addr>`**,
where `<addr>` is `0x60 | (port A switches & 0x1F)`. Five bytes, LSB first, one
start bit and the parity/stop configuration the port B switches select. If it
matches, the baud rate, polarity and framing are all confirmed at once, and the
receive direction can be attempted with confidence.

### The physical layer: opto-isolated RS-422/RS-485 **[C]**

**Settled by reading the packages** (owner, 2026-09). Daughter 1's five DIP8s are:

| Count | Part | Function |
| --- | --- | --- |
| 3, yellow bodies | **HP 6N139** | high-speed optocouplers — the isolation barrier |
| 2, black bodies | **TI SN75176BP** | **differential bus transceivers**, RS-485 / RS-422 |

The SN75176B is *the* standard RS-485 transceiver of the period: one driver and
one receiver in a DIP8, sharing a single `A`/`B` pair, with separate driver-enable
(`DE`) and receiver-enable (`RE`) pins.

**That ends the current-loop question.** Nothing about a 20 mA loop needs a
differential transceiver, and a board that has two of them is a differential
board.

#### It is 4-wire: one pair per transceiver **[C]**

**Traced by the owner, 2026-09: one cable pair lands on the `A`/`B` pins of one
SN75176BP, the other pair on the `A`/`B` pins of the other.**

That rules out 2-wire. A 2-wire link puts both directions on a single pair and a
single transceiver; here each pair has its own. So the four conductors are **two
independent unidirectional pairs — 4-wire full duplex**.

The one competing reading — *two independent half-duplex channels*, which the
`MAESTRA` label makes tempting — **does not survive the parts count**:

| Evidence | Why it forces one channel, not two |
| --- | --- |
| **One USART.** U3, the only M82C51A-2 in the unit, has one `TxD` and one `RxD` | Two independent channels would need two USARTs, or external muxing that is not on the board |
| **Three optocouplers**, not four or more | Exactly enough for one TX data, one RX data and one control line crossing the barrier |
| **Two transceivers, each half-used** | The classic 4-wire idiom: the driver of one, the receiver of the other |

So: **the display drives one pair and listens on the other.** Which pair is which
is still open — that needs `DI` (pin 4) and `RO` (pin 1) followed back through the
optocouplers.

This fits everything else already known:

* **Multidrop with addressing.** `GS <addr>` selects one unit of many and `0x7F`
  broadcasts ([../docs/serial-protocol.md](../docs/serial-protocol.md)). In 4-wire
  multidrop the master drives one pair to every slave, and the slaves share the
  return pair — so each slave **must** tri-state its driver when not replying.
* **Which is what RTS is for.** The firmware asserts RTS through
  `line_turnaround` before every reply and drops it afterwards. Wired to `DE`,
  that is textbook shared-return direction control, and it explains why a
  supposedly full-duplex link bothers with turnaround delays at all.
* **Three optocouplers, three signals.** TX data, RX data and the driver enable —
  matching the three 560 Ω and three ~1 kΩ resistors exactly.

#### The yellow-green pair is the receive side **[C]**

**Traced by the owner, 2026-09:** on the transceiver carrying the **yellow-green**
pair, three pins are strapped to ground — `/RE`, `DE` and `D`.

Against the SN75176B pinout that is unambiguous:

| Pin | Name | Strapped | Effect |
| --- | --- | --- | --- |
| 2 | `/RE` | **GND** | receiver enable is active **low** → **receiver permanently on** |
| 3 | `DE` | **GND** | driver enable is active **high** → **driver permanently tri-stated** |
| 4 | `D` (`DI`) | **GND** | driver input tied to a defined level; irrelevant, the driver is off |

**That package can only ever receive.** So the **yellow-green pair is the inbound
pair — host to display** — and by elimination the blue-red pair is the one the display
transmits on. **[C]** for yellow-green, **[L]** for orange-blue until its `DI` is
seen to be driven.

It also confirms the three-optocoupler account: the receive transceiver needs
exactly one signal crossing the barrier (`RO` → the USART's `RxD`), leaving two
for the transmit side (`DI` ← `TxD`, and `DE`).

##### The next question this raises: is `DE` on the other package driven, or tied high?

This discriminates two quite different installations, and the answer changes what
a host must do:

| If the transmit package's `DE` is… | Then | Consequence |
| --- | --- | --- |
| driven from **RTS** through the third optocoupler | **multidrop** — several displays share the return pair, each tri-stating until addressed | the host must tolerate a pair that is undriven between replies, and terminate accordingly |
| tied **high** | **point-to-point** — this display drives its return pair continuously | one display per host port, and the `GS <addr>` addressing is then a bus convention it does not electrically need |

The firmware asserting RTS before every reply argues for the first, but that is an
inference from software; the strap settles it. **[?]**

#### The resistors, re-read

| Resistor | Role under RS-485 |
| --- | --- |
| 3 x **560 Ω** | 6N139 LED drive from 5 V — about 6.5 mA, right for the part |
| 3 x **~1 kΩ** | 6N139 output pull-ups |

> **Retracted: the 20 mA arithmetic.** This section previously argued that
> `(24 − 1.35 V) / 1 kΩ ≈ 22 mA` made a ~1 kΩ resistor from a 24 V rail "a 20 mA
> loop, whichever way the bands are read". The caveat it recorded — that the
> arithmetic assumes the far end sits at 24 V — turned out to be the whole story.
> The isolated rail is **5 V**, and the parts are transceivers. These are opto
> drive and pull-up resistors.

> **Retracted: "neither resistor value is a termination".** True, and no longer
> evidence of anything. RS-485 terminates only at the **two ends of the bus**; a
> mid-bus slave carries no 120 Ω, and a station sign is very unlikely to be an end
> node. Its absence was never an argument against differential signalling.

> **Retracted: "there is no transceiver IC", the strongest argument of the old
> case.** There were two, unread, in the same parts list the argument was built
> from.

#### This probably explains the scope transitions **[L]**

The 2026-08 attempt saw "transitions with an AC component" on the DB37 pins and
could not resolve a clean differential. Two facts now bear on it: the firmware
transmits **nothing unprompted**, and the line is an **RS-485 pair with no
termination and no fail-safe bias**.

An undriven, unterminated, unbiased differential pair floats and picks up
whatever is around it — and there is a fluorescent ballast inside the same
enclosure ([psu.md](psu.md)). Noise on a floating pair is precisely "transitions,
unclean, with an AC component". **[L]**

That would mean the transitions were never a signal at all, which also disposes of
the conclusion drawn from them — that the pins carrying them must be the transmit
side.

#### What is left to measure

- [x] ~~Which pair goes where~~ — **one pair per transceiver, so 4-wire.** Done.
- [x] ~~Which pair is inbound~~ — **yellow-green**, its transceiver strapped
      receive-only. Orange-blue is the transmit pair by elimination.
- [x] ~~Which conductor is `A` and which is `B`~~ — **green = `A`, yellow = `B`**
      on the receive pair; **blue = `A`, red = `B`** on the transmit pair.
- [ ] **Confirm the fourth conductor's colour** — red or orange? Both are on
      record and they disagree.
      - Result:
- [ ] **DB37 pin numbers for all four.** The last missing column.
- [ ] **Is `DE` on the blue-red package driven or tied high?** Driven from RTS
      → multidrop; tied high → point-to-point. See
      [above](#the-next-question-this-raises-is-de-on-the-other-package-driven-or-tied-high).
- [ ] **Confirm the blue-red package's `DI` is driven** (rather than strapped),
      which upgrades the transmit-pair assignment from **[L]** to **[C]**.
- [ ] **Check for 120 Ω** across either pair. Absent is expected for a mid-bus
      node and means the host end must terminate.
- [ ] **Idle differential voltage**, powered, nothing connected: near 0 V with a
      few volts common mode is normal for an undriven RS-485 pair.

### Measurement plan — what is left

Most of this section's original content is done. What remains is continuity work
on daughter 1 and the DB37:

1. **Number the DB37 pins** from the connector shell and photograph the numbering
   alongside the wires, so the colour-to-pin mapping is unambiguous later. **This
   is the one remaining gap in a complete wiring diagram.**
2. **Buzz each of the four wires** from its DB37 pin to the daughter 1 terminal
   block, and from there to an SN75176BP pin 6 or 7. That gives colour → pin →
   `A`/`B` in one pass.
3. **Check the blue-red package's `DE` and `DI`** as above.

Only then is a scope worth setting up — and remember the unit **answers, it does
not announce**, so a host must be polling before its transmit pair carries
anything.

## Flip-dot panel ribbon — daughter 2

**Topology, observed:** each character is an independent module; the sixteen modules
hang off the ribbon as a **common bus** rather than being daisy-chained; none has
address switches; and the ribbon **exits the unit** after the last character,
spliced where the display was removed. It almost certainly continued to the
**second LED panel**, making this one bus shared by both displays.

That forces per-character **select lines** — see
[../docs/flipdot-interface.md](../docs/flipdot-interface.md#what-that-forces).

**Counted: 26 conductors.** That confirms the model and rules out the ~8-10
alternative in which position would have been decoded on the panel. Expected
allocation: 16 selects, 1 clock, 1 data, 2 control, and 6 for 24 V and returns.

Next: trace which conductor is which. Look for **16 that each reach exactly one
module**, **2 that reach every module**, and the remainder carrying supply.


Large ribbon header on daughter 2, driven by the ULN2003A Darlington arrays
(U18) and, most likely, an 81C55 at I/O `0xE0` (see
[../docs/io-map.md](../docs/io-map.md)).

| Property | Value | Conf. |
| --- | --- | --- |
| Conductor count | **26** | **[C]** |
| Type | Very likely 26-way IDC (13 x 2), standard width | **[L]** |
| Pitch | *TODO* | |
| Pinout | *TODO* — 16 selects + clock + data + control + supply expected | **[L]** |

For 16 characters x 5 columns = 80 columns and 7 rows, a plausible arrangement
is 7 row lines plus a column address, or column-strobe shift registers on the
panel itself. Counting ribbon conductors will discriminate immediately.

## Power

24 VDC input; see [psu.md](psu.md). Flip-dot coils typically want 24-30 V
pulses, so the panel is probably driven straight off the input rail with the
logic on a local 5 V regulator (there is a DC converter module soldered to
daughter 1).

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

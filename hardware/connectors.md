# Connectors

## DB37 — external control input

The only interface to the outside world. Of 37 pins, **4 conductors are
populated**: orange, blue, green and yellow, inside a grey overall-shielded
cable (see [photos/DB37_back.jpg](photos/DB37_back.jpg)). They land on a
terminal block on daughter 1.

| Pin | Wire colour | Signal | Conf. |
| --- | --- | --- | --- |
| ? | orange | ? | **[?]** |
| ? | blue | ? | **[?]** |
| ? | green | ? | **[?]** |
| ? | yellow | ? | **[?]** |
| — | grey (shield) | chassis / drain | **[?]** |

### Attempt 1: oscilloscope, 2026-08 — transitions present, signal unclear

All four pins probed. **Transitions were visible**, but the signal was not clean
and appeared to carry an AC component. Two channels subtracted across candidate
pairs did not resolve a clean differential either.

**Transitions are a positive result, not a null one.** They mean something is
actively driving that conductor, and the obvious candidate is the unit's own
status frame: the firmware transmits `18 18 1E 15 <addr>` continuously, forever,
with no prompting (see
[../docs/serial-protocol.md](../docs/serial-protocol.md#the-transmitted-status-frame-c)).

That gives three immediate conclusions:

1. **The unit is alive and transmitting**, and the pins that showed transitions
   are the **transmit** side. Record which ones they were.
2. **The transmit side is not a bare passive switch.** A passive 20 mA loop
   transmitter is just an optocoupler output with no supply of its own; on an open
   circuit it cannot produce voltage transitions. Either the transmitter is
   *active* (supplies its own loop current), or the interface is voltage-mode
   after all. This weakens the plain passive-current-loop theory.
3. **The waveform is decodable.** We know exactly what it should say, which makes
   any capture a known-answer test.

### Why it looked unclean

Most likely causes, in order:

1. **Timebase too slow.** The unit transmits *continuously*. A non-stop serial
   stream viewed at a slow timebase looks precisely like a messy AC waveform —
   that alone could account for the "AC component". At 9600 baud a bit is 104 µs
   and a 5-byte frame about 5 ms; at 300 baud a bit is 3.3 ms and the frame
   ~170 ms. The switch-selected rate may be anywhere in that range, so sweep the
   timebase rather than assuming.
2. **No trigger on the frame.** Asynchronous data free-runs unless you trigger on
   an edge and use single-shot capture. Untriggered, successive frames overlay
   into a smear.
3. **Reference undefined.** U7 is an optocoupler, so the DB37 side floats
   relative to chassis. A ground clip on the chassis puts real transitions on top
   of a wandering hum baseline. Channel-subtract does not fix this — a scope's
   A−B has poor common-mode rejection compared with a differential probe.
4. **Mains hum.** The fluorescent **ballast is inside the enclosure**
   ([psu.md](psu.md)), so a live tube feed puts a strong 50 Hz inductive source
   centimetres from the probe.

### Attempt 2: capture one frame properly

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
| **Peak-to-peak amplitude** | ±2-5 V → RS-422/485. ±5-12 V → RS-232. 12-30 V swinging to near a diode drop → an active current loop. |
| **Narrowest pulse width** | One bit time. Invert for the baud rate — which also settles the 81C55 TIMER IN question in [../docs/dip-switches.md](../docs/dip-switches.md). |
| **Idle level** | Mark polarity, and hence whether the line is inverted. |

Finally, **decode the frame by hand and check it against `18 18 1E 15 <addr>`**,
where `<addr>` is `0x60 | (port A switches & 0x1F)`. Five bytes, LSB first, one
start bit and the parity/stop configuration the port B switches select. If it
matches, the baud rate, polarity and framing are all confirmed at once, and the
receive direction can be attempted with confidence.

### What daughter 1's parts imply **[L]**

The passive inventory in [pcbs.md](pcbs.md) is more informative than it looks.

#### The main argument: no *identified* transceiver IC

Daughter 1 carries a DC-DC converter, a hex Schmitt trigger, five DIP8s and a
handful of resistors, diodes and capacitors. **That reads like an opto-isolated
current loop built from discretes.**

An RS-422 or RS-485 interface of this era would normally use a purpose-made
differential transceiver — a 26LS31/26LS32 pair, 75174/75175, SN75179 or similar
— and none of those is a DIP8, so none is present.

> **But this is weaker than it was written.** Only **three** of the five DIP8s are
> yellow-bodied and likely optocouplers
> ([chips.md](chips.md#the-five-dip8s-on-daughter-1--unidentified-and-unlabelled));
> the other two are **unidentified and unmarked**. Several common RS-485
> transceivers *are* DIP8 — SN75176, MAX485, LTC485 — so this is an argument from
> parts nobody has read yet. Reading those two packages would settle the physical
> layer faster than any measurement on the list below.
>
> An isolated 5 V rail on the line side, which daughter 1 does have, suits an
> isolated differential transceiver at least as well as it suits a loop.

#### Neither resistor value is a termination

RS-422/485 wants **120 Ω** across the pair. The board has **560 Ω** and
**~1 kΩ**, and nothing near 120 Ω. Termination is not always fitted, so this is
suggestive rather than conclusive — but combined with the missing transceiver it
points the same way.

#### The values make sense as a current loop, with a caveat

| Resistor | Plausible role | Current |
| --- | --- | --- |
| 3 x **560 Ω** | Optocoupler LED drive on the **logic** side, from 5 V | ~6.5 mA — comfortable for any opto |
| 3 x **~1 kΩ** | Series element on the **line** side | ~20 mA if the node is at 24 V |

`(24 − 1.35 V) / 1 kΩ ≈ 22 mA`, and `/1.2 kΩ ≈ 19 mA`. **Whichever way the bands
are read, a ~1 kΩ resistor from a 24 V rail is a 20 mA loop.**

The caveat matters: that arithmetic assumes the resistor's far end sits at ~24 V.
From 5 V the same resistor gives ~3.6 mA, which would be an ordinary opto drive
and would say nothing about current loops. **So the value alone does not settle
it — the node voltage does.**

#### Three of each, not two

Three 560 Ω and three ~1 kΩ suggests **three signals in each direction**, not one.
The obvious third is **RTS**, which the firmware drives through
`line_turnaround` on every reply.

> A "second channel for the LED matrix" was also offered here. **The board
> architecture rules it out** — the LED panel is a display, so it belongs behind
> daughter 2, and no host communication passes through daughter 1 to a display.
> See [pcbs.md](pcbs.md#architecture-one-function-per-board-l).

Worth keeping in mind when tracing: do not assume only two signals exist.

#### The line side has its own isolated supply **[L]**

Daughter 1's Melcher `24 IMP 3-05-7` gives an **isolated 5 V / 500 mA**
([pcbs.md](pcbs.md#the-dc-dc-converter)). The motherboard has its own TO-220
regulators, so this rail is not the logic supply — it powers the far side of the
isolation barrier, which is what a media-conversion board needs.

So the transmitter **can** be active rather than a bare passive switch. But note
what the 5 V figure costs:

* **An active 20 mA current loop from this rail is implausible.** 5 V cannot drive
  20 mA around a loop with any useful compliance. The project's own stated test —
  *"if the isolated rail is ~24 V the ~1 kΩ resistors are loop-current setting; if
  it is 5 V they are not"* — has been answered on the "they are not" side.
* An isolated **5 V** feeding optocouplers and a line driver fits either a
  **passive** current loop, powered from the far end, or an **isolated
  differential** link. Both remain open.

The ~1 kΩ resistors could still be fed from the **24 V input** rather than `VO+`,
which would revive an active loop. Nobody has traced them.

> **Retracted, then restored.** An intermediate revision of this section removed
> the isolated-supply argument entirely, on the grounds that a 5 V output made the
> converter the logic rail. It does not — the motherboard's regulators do that.
> Only the *active 20 mA loop from this rail* is dead.

Separately, the scope transitions no longer have an explanation: the earlier
attribution to the unit's own status frame is retracted, because the firmware
transmits **nothing unprompted**
([serial-protocol.md](../docs/serial-protocol.md#the-transmitted-status-frame-c)).

#### What would settle it

- [ ] **Confirm `VO-` is *not* continuous with the display's 0 V / chassis**
      (power off, continuity). It should read open. If it beeps, the isolation is
      not where this analysis assumes it is and the reasoning needs revisiting.
- [ ] **Confirm `VO-` is *not* continuous with the display 0 V**, and that `VO+`
      does *not* reach the motherboard's 5 V net. Both should be open if the rail
      is line-side only, as [psu.md](psu.md#the-logic-5-v-comes-from-the-motherboard-not-the-converter-l)
      now assumes.
- [x] ~~Measure the converter's output voltage.~~ **Answered by the type code:
      5 V, 500 mA.** By the criterion stated here, the ~1 kΩ resistors are *not*
      loop-current setting from this rail.
- [ ] **Trace the ~1 kΩ resistors' supply end.** If they come from the 24 V input
      rail rather than `VO+`, an active loop is still possible.
- [ ] **Does a ~1 kΩ resistor run from `VO+` to a terminal-block screw or an
      optocoupler pin?** This is the single measurement that closes the question:
      an isolated rail feeding the line through a current-setting resistor *is* an
      active current loop.
- [ ] **Trace one ~1 kΩ resistor**: does it sit between a terminal-block screw and
      a 6N139/optocoupler pin?
- [ ] **Measure the ~1 kΩ resistors** with a meter to remove the band ambiguity.
- [ ] **Identify the 4 diodes' arrangement.** A full bridge on an input pair means
      a polarity-insensitive current-loop input, which is a strong signature.
      Individual diodes across opto LEDs are just reverse protection.

### Attempt 3: the standard, without a scope

Power **off**, multimeter in **diode mode**, across each pair, both polarities:

| Reading | Interpretation |
| --- | --- |
| ~1.1-1.6 V one way, open the other | **20 mA current loop.** That is the 6N139's LED. Decisive. |
| No diode behaviour; ~120 Ω, or ~12 kΩ | **RS-422/485.** 120 Ω is a termination resistor, 12 kΩ a receiver input. |
| Open both ways | Not the receive pair — try another combination |

Supporting checks:

- [ ] Look for a **120 Ω resistor across a pair** on daughter 1. Present →
      RS-422/485. Absent, with series resistors feeding the opto → current loop.
- [ ] Read the **series resistor value** feeding the 6N139 input. With the supply
      voltage it gives the intended loop current directly: ~1 kΩ from 24 V is a
      20 mA loop.
- [ ] Determine whether the transmit side is **passive or active** — i.e. whether
      anything on daughter 1 supplies current to the TX pair, or whether the yellow DIP8s
      are bare switches expecting an external supply.

### Attempt 4: if the transmitter turns out to be a passive loop after all

Then you must **supply the loop current yourself** before anything is observable:

1. Power the display. It transmits its status frame continuously.
2. Put a current-limited source across the suspected TX pair — e.g. 24 V through
   ~1 kΩ for roughly 20 mA. Mind the polarity; a loop only conducts one way.
3. Scope **across the series resistor**, not across the pair. That converts the
   loop current into a voltage you can actually see. Reference the scope ground
   to your own supply's 0 V, not to the display chassis.
4. You should see the frame at whatever baud the port B switches select.

A current probe or a USB current-loop adapter does the same job without the
resistor trick.

### Measurement plan

Do this before powering anything up. It is pure continuity work and it unblocks
the whole project — see [../docs/open-questions.md](../docs/open-questions.md).

1. **Number the pins** from the connector shell, and photograph the numbering
   alongside the wires so the mapping is unambiguous later.
2. **Buzz out** each of the 4 wires from its DB37 pin to the daughter 1
   terminal block; record the terminal number.
3. **Trace from the terminal block to U7** (6N139 optocoupler). The two wires
   reaching the 6N139 anode/cathode (pins 2 and 3) are the **receive** pair.
4. **Find the transmit driver.** Look at the three yellow DIP8 parts on
   daughter 1: if they are optocouplers, the TX path is opto-isolated too, and
   whichever pair they drive is the transmit pair.
5. **Measure the idle voltage** across each pair with the unit powered and
   nothing connected. This distinguishes the two candidates:
   * ~0 V differential with a few volts common mode → RS-422/RS-485.
   * A constant current (~20 mA) flowing, or a supply feeding a series
     resistor into the loop → 20 mA current loop.
6. Record polarity: for a current loop, note which conductor sources current.

Note the series resistor values feeding the 6N139 — with the supply voltage they
give the loop current directly, which tells you the expected drive level.

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

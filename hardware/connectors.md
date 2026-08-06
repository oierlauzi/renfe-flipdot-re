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

### Attempt 1: oscilloscope, 2026-08 — inconclusive

All four pins probed; no clean signal, an apparent AC component. Two channels
subtracted across candidate pairs showed no differential signal either.

**This is not evidence for or against any standard.** It is what a correct
measurement of an *isolated, passive* interface looks like, and also what a
badly-referenced measurement of any interface looks like. See below.

### Why the scope showed nothing useful

Three independent reasons, any one of which is sufficient:

1. **The interface is galvanically isolated.** U7 is a 6N139 optocoupler, so the
   DB37 side has no defined potential relative to the display's chassis or 0 V.
   A scope ground clip on the chassis therefore measures across an isolation
   barrier with an undefined reference: the reading floats and picks up whatever
   is around. Channel-subtract does not rescue this — a scope's A−B has poor
   common-mode rejection compared with a real differential probe, and both inputs
   may sit outside their common-mode range to begin with.

2. **A mains-hum explanation for the "AC component".** A floating conductor next
   to mains wiring reads 50 Hz. The fluorescent **ballast is inside the
   enclosure** ([psu.md](psu.md)), so if the tube feed was live there was a
   substantial inductive noise source a few centimetres from the probe.

3. **A passive current loop has no voltage to show.** This is the important one.
   In a passive 20 mA loop the receiver is just an LED and a series resistor, and
   the transmitter is just a switch (an optocoupler's output transistor). Neither
   end generates a voltage — the loop current is supplied by the *other* end of
   the link. With nothing connected, there is nothing to see, **even while the
   unit is transmitting**.

### So does this point to a current loop?

**Weakly yes, and only under one condition.** The reasoning:

* An RS-422/485 transmitter is an active driver. If the unit was **powered** while
  probing, its TX pair should have shown a steady idle differential of a few
  volts and clean transitions, because the firmware transmits its status frame
  continuously (`18 18 1E 15 <addr>` — see
  [../docs/serial-protocol.md](../docs/serial-protocol.md)). Seeing nothing there
  argues against an active differential driver.
* A passive current-loop transmitter would show exactly nothing under the same
  conditions.

So *if the unit was powered and running*, the null result leans towards a current
loop. **If it was unpowered, the measurement says nothing at all.** Record which
it was — that single fact changes how much the result is worth.

### Attempt 2: the decisive test, and it needs no scope

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
      anything on daughter 1 supplies current to the TX pair, or whether U14-U17
      are bare switches expecting an external supply.

### Attempt 3: seeing actual traffic

If it is a passive loop, you must **supply the loop current yourself** before
anything is observable:

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
4. **Find the transmit driver.** Look at the four yellow DIP8 parts U14-U17 on
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

Large ribbon header on daughter 2, driven by the ULN2003A Darlington arrays
(U18) and, most likely, an 81C55 at I/O `0xE0` (see
[../docs/io-map.md](../docs/io-map.md)).

| Property | Value | Conf. |
| --- | --- | --- |
| Pin count | *TODO* | |
| Pitch | *TODO* | |
| Pinout | *TODO* | |

For 16 characters x 5 columns = 80 columns and 7 rows, a plausible arrangement
is 7 row lines plus a column address, or column-strobe shift registers on the
panel itself. Counting ribbon conductors will discriminate immediately.

## Power

24 VDC input; see [psu.md](psu.md). Flip-dot coils typically want 24-30 V
pulses, so the panel is probably driven straight off the input rail with the
logic on a local 5 V regulator (there is a DC converter module soldered to
daughter 1).

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

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
      anything on daughter 1 supplies current to the TX pair, or whether U14-U17
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

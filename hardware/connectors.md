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

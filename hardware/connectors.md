# Connectors

## DB37 — external control input

The only interface to the outside world. Of 37 pins, four conductors are
populated, inside a grey overall-shielded cable (see
[photos/DB37_back.jpg](photos/DB37_back.jpg)). They land on a terminal block on
daughter 1.

| DB37 pin | Wire | Transceiver pin | Pair | Direction |
| --- | --- | --- | --- | --- |
| not traced | **green** | `A` (6) | RX | display receives, host → display |
| not traced | **yellow** | `B` (7) | RX | display receives |
| not traced | **blue** | `A` (6) | TX | display transmits |
| not traced | **red** | `B` (7) | TX | display transmits |
| — | grey shield | — | — | chassis / drain |

The DB37 pin numbers are the only part of this not established; identify the
conductors by colour at the connector.

## The physical layer

Daughter 1's five DIP8 packages are:

| Count | Part | Function |
| --- | --- | --- |
| 3, yellow bodies | **HP 6N139** | high-speed optocouplers — the isolation barrier |
| 2, black bodies | **TI SN75176BP** | differential bus transceivers, RS-485 / RS-422 |

The SN75176B is the standard RS-485 transceiver of the period: one driver and one
receiver in a DIP8, sharing a single `A`/`B` pair, with separate driver-enable
(`DE`) and receiver-enable (`RE`) pins. **The link is RS-422 differential,
opto-isolated.**

### It is 4-wire: one pair per transceiver

One cable pair lands on the `A`/`B` pins of one SN75176BP, the other pair on the
`A`/`B` pins of the other. A 2-wire link puts both directions on a single pair and
a single transceiver, so the four conductors are two independent unidirectional
pairs: 4-wire full duplex.

Two independent half-duplex channels would fit two transceivers equally well, but
three things exclude them:

| Evidence | Why it forces one channel |
| --- | --- |
| **One USART.** U3 is the only M82C51A-2 in the unit; one `TxD`, one `RxD` | Two independent channels need two USARTs, or external muxing that is not on the board |
| **Three optocouplers**, not four or more | Exactly enough for one TX data, one RX data and one control line |
| **Each transceiver half-used** | The standard 4-wire idiom |

It also accounts for the firmware's RTS turnaround, which would be redundant on a
point-to-point full-duplex link: on a 4-wire multidrop bus the slaves share the
return pair, so each must tri-state its driver until addressed.

### The green-yellow pair is the receive side

On the transceiver carrying green and yellow, three pins are strapped to ground:

| Pin | Name | Strapped | Effect |
| --- | --- | --- | --- |
| 2 | `/RE` | GND | receiver enable is active low, so the **receiver is permanently on** |
| 3 | `DE` | GND | driver enable is active high, so the **driver is permanently tri-stated** |
| 4 | `D` (`DI`) | GND | driver input tied to a defined level; moot with the driver off |

That package can only receive, so green/yellow is the inbound pair and blue/red is
the pair the display transmits on. It also confirms the three-optocoupler account:
the receive transceiver needs exactly one signal crossing the barrier, `RO` to the
USART's `RxD`, leaving two for the transmit side, `DI` from `TxD` and `DE`.

Whether `DE` on the blue-red package is driven from RTS or tied high has not been
traced, and the two mean different installations:

| Transmit package's `DE` | Installation | Consequence for a host |
| --- | --- | --- |
| driven from RTS through the third optocoupler | multidrop; several signs share the return pair, each tri-stating until addressed | the pair is undriven between replies, so bias it |
| tied high | point-to-point; this sign drives its return pair continuously | one display per host port |

The firmware asserting RTS before every reply argues for multidrop, but that is an
inference from software.

### The resistors

| Resistor | Role |
| --- | --- |
| 3 x 560 Ω | 6N139 LED drive from 5 V, about 6.5 mA |
| 3 x ~1 kΩ | 6N139 output pull-ups |

Three of each, for the three signals crossing the isolation barrier.

There is no 120 Ω anywhere on the board, which is normal: RS-485 terminates only
at the two ends of the bus, and a station sign is unlikely to be an end node. A
host at the end of the link needs its own termination.

### The line side has its own supply

The Melcher `24 IMP 3-05-7` on daughter 1 gives an isolated 5 V at 500 mA
([pcbs.md](pcbs.md#the-dc-dc-converter)). The motherboard has its own TO-220
regulators, so this rail is not the logic supply — it powers the far side of the
isolation barrier, which is what a media-conversion board needs.

### The link is quiet unless polled

The firmware transmits nothing unprompted, confirmed both in emulation and on the
unit: listening with nothing sent produces silence.

An undriven, unterminated, unbiased differential pair floats and picks up whatever
is nearby, and there is a fluorescent ballast inside the same enclosure
([psu.md](psu.md)). Noise on a floating pair reads as framing errors and phantom
bytes at a UART, which is worth expecting if the host end is unbiased.

### What is left to measure

- [ ] **DB37 pin numbers** for all four conductors. Number the pins from the
      connector shell, photograph the numbering next to the wires, and buzz each
      through to the daughter 1 terminal block.
- [ ] **Is `DE` on the blue-red package driven or tied high?** Follow pin 3 back.
- [ ] **Confirm the blue-red package's `DI` is driven** rather than strapped.

## The motherboard terminal block

The motherboard carries its own four-way terminal block, unconnected in this unit,
labelled `RP RN TP TN`. Those are differential pair names — receive and transmit,
positive and negative — rather than a TTL USART header, which would be labelled
`TxD`/`RxD` and single-ended.

Nothing identified on the motherboard can drive a differential pair. U9 is a
CD74HC138E decoder and U10 a CD74HC373E address latch, both glue logic, which
leaves **U8**, the last unidentified part on the board, as the only candidate.

Two readings fit, and reading U8 decides between them:

| Reading | Would mean | Discriminating trace |
| --- | --- | --- |
| Daughter 1's pairs brought to the motherboard | an alternative connection point to the DB37, with direction already labelled | continuity from `RP`/`RN`/`TP`/`TN` to daughter 1's SN75176BP `A`/`B` pins |
| An independent line interface, used when daughter 1 is not fitted | the product ships with either interface, and `U7` on the motherboard is its isolation | continuity to `U7` and `U8`, but not to daughter 1 |

The first is the more economical, and would be useful: the block is labelled by
direction, which the DB37 is not.

Both are settled by continuity with the power off. It is also worth checking
whether the block reaches the 8251A's `TxD` (pin 19) and `RxD` (pin 3) directly,
which would make the P/N naming misleading.

## Flip-dot panel ribbon — daughter 2

| Property | Value |
| --- | --- |
| Conductor count | **26** |
| Type | very likely 26-way IDC, 13 x 2 |
| Pitch | not recorded |
| Pinout | not traced |

Each character is an independent module, but modules mount in pairs on a shared
**flip-dot carrier board** — 8 carriers for this unit's 16 characters — which also
carries the ribbon connector and the pair's 6 address-setting switches. Carriers
hang off the ribbon as a common bus rather than being daisy-chained, and the
ribbon exits the unit after the last carrier, spliced where the display was
removed.

The panel is fed serially, not as a row/column matrix reaching over the ribbon.
Each character module is wired internally as a 5x7 diode matrix, but that matrix is
decoded locally on the carrier board. Addressing is encoded rather than one-hot,
and each carrier's 6 switches match the firmware's 6-bit module-address field
exactly. See
[../docs/flipdot-interface.md](../docs/flipdot-interface.md#the-flip-dot-carrier-board).

Expected conductor allocation: six or seven for the encoded address, one clock,
one data, one or two control, and the remainder for 24 V and returns. To trace
which is which, look for the group carrying a binary count, the two that toggle
per bit, and the supply.

## Power

24 VDC input; see [psu.md](psu.md). Flip-dot coils typically want 24-30 V pulses,
so the panel is probably driven straight off the input rail, with the logic on the
motherboard's own regulators.

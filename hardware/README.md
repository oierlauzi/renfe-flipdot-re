# Hardware

Physical description of the unit. What it does with all this is in
[../docs/](../docs/).

## Photographs

| File | Subject |
| --- | --- |
| [working.jpg](photos/working.jpg), [working-panel.jpg](photos/working-panel.jpg) | The unit running, after the work in this repository |
| [cover_open.jpg](photos/cover_open.jpg) | The whole unit with the cover off |
| [sticker.jpg](photos/sticker.jpg) | Chassis label: model, serial, "MAESTRA" |
| [Electronics.jpg](photos/Electronics.jpg), [Electronics + PSU.jpg](photos/Electronics%20%2B%20PSU.jpg) | The three boards, and with the power supply |
| [Mother (top).jpg](photos/Mother%20%28top%29.jpg), [Mother (bottom).jpg](photos/Mother%20%28bottom%29.jpg) | Motherboard |
| [Daughter 1 (top).jpg](photos/Daughter%201%20%28top%29.jpg), [Daughter 1 (bottom).jpg](photos/Daughter%201%20%28bottom%29.jpg) | Serial interface board |
| [Daughter 1 Terminal block connections.jpg](photos/Daughter%201%20Terminal%20block%20connections.jpg) | Where the four DB37 conductors land |
| [Daughter 2 (top).jpg](photos/Daughter%202%20%28top%29.jpg), [Daughter 2 (bottom).jpg](photos/Daughter%202%20%28bottom%29.jpg) | Flip-dot drive board |
| [DB37_back.jpg](photos/DB37_back.jpg) | The DB37 connector, wire side |
| [ROM_chip.jpg](photos/ROM_chip.jpg) | The EPROM and its TELENORMA label |
| [switches1.jpg](photos/switches1.jpg), [switches2.jpg](photos/switches2.jpg), [switches3.jpg](photos/switches3.jpg) | The configuration switches |
| [Flip-dot carrier.jpg](photos/Flip-dot%20carrier.jpg) | A carrier board: address switches and ribbon connector |
| [Flip-dot daughter (front).jpg](photos/Flip-dot%20daughter%20%28front%29.jpg), [Flip-dot daughter (back).jpg](photos/Flip-dot%20daughter%20%28back%29.jpg) | One character module |

[photos/in-service/](photos/in-service/) holds photographs of signs of this kind
on platforms, in service and being taken down.

## The three boards

| Board | Job |
| --- | --- |
| Motherboard | CPU, RAM, EPROM, USART, configuration switches, logic regulators |
| Daughter 1 | Turns the USART's logic levels into an isolated RS-422 line |
| Daughter 2 | Drives the flip-dot panel |

This is a modular product. The same motherboard serves any physical layer by
swapping daughter 1, and any panel type by swapping daughter 2, which is what
the firmware's 13 panel drivers and switch-selected line settings are for.

## Parts

| Ref | Part | Board | Role | Datasheet |
| --- | --- | --- | --- | --- |
| U1 | OKI MSM80C85AH | Mother | CPU, with a 6.144 MHz crystal | [pdf](https://www.silicon-ark.co.uk/datasheets/m80c85AH-datasheet-oki.pdf) |
| U2 | OKI M81C55-5 | Mother | 256 B RAM, switch inputs, baud timer | [pdf](https://docs.rs-online.com/1f1a/0900766b800264a3.pdf) |
| U3 | OKI M82C51A-2 | Mother | USART, the whole external interface | [pdf](https://www.cryptomuseum.com/crypto/rs/pu104/files/M82C51_datasheet.pdf) |
| U4 | ST M27256F1 | Mother | 32 KiB EPROM | [pdf](https://deramp.com/downloads/mfe_archive/050-Component%20Specifications/ST-SGS/ST%20M27256.pdf) |
| U5, U11 | MC14584BCP | Mother, D1 | Hex Schmitt trigger | [pdf](https://www.onsemi.com/download/data-sheet/pdf/mc14584b-d.pdf) |
| U6 | 7555 | Mother | Timer. Probably the power-on reset or a watchdog | [pdf](https://www.nxp.com/docs/en/data-sheet/ICM7555.pdf) |
| U7 | HP 6N139 | Mother? | Optocoupler. Board attribution doubtful, see below | [pdf](https://www.vishay.com/docs/83605/6n139.pdf) |
| U8 | ? | Mother | **Never identified** | |
| U9 | CD74HC138E | Mother | 3-to-8 decoder: the I/O chip selects | [pdf](https://www.ti.com/lit/ds/symlink/cd74hc138.pdf) |
| U10 | CD74HC373E | Mother | Octal latch: demultiplexes the address bus for the EPROM | [pdf](https://www.ti.com/lit/ds/symlink/cd74hc373.pdf) |
| 3 x | HP 6N139 | D1 | Optocouplers: the isolation barrier | [pdf](https://www.vishay.com/docs/83605/6n139.pdf) |
| 2 x | TI SN75176BP | D1 | RS-422 differential transceivers | [pdf](https://www.ti.com/lit/ds/symlink/sn75176b.pdf) |
| | Melcher 24 IMP 3-05-7 | D1 | Isolated DC-DC, 10-36 V in, 5 V 500 mA out | |
| U17-U19 | ULN2003A | D2 | 21 Darlington sinks for the coils | [pdf](https://www.ti.com/lit/ds/symlink/uln2003a.pdf) |
| U20 | OKI M81C55-5 | D2 | The flip-dot interface ports | [pdf](https://docs.rs-online.com/1f1a/0900766b800264a3.pdf) |

D1 is daughter 1, D2 is daughter 2. The Intel originals (8085AH, 8155, 8251A)
are the better reference for register-level behaviour than the OKI datasheets.

Two loose ends on the motherboard. **U8** was never identified, and it is the
only candidate for anything that could drive the terminal block described below.
**U7** is recorded as being on the motherboard, but daughter 1 already carries
three 6N139s, which is a complete isolation barrier, so a fourth on the logic
board would be redundant. The silkscreen is worth checking against the board.

## Telenorma part numbers

The motherboard carries the Telenorma logo. These silkscreen numbers are the
most specific identifiers on the unit and the most promising thing to search
for. They were read off photographs, so check them against the board before
quoting them anywhere.

| Board | Silkscreen | Rev |
| --- | --- | --- |
| Mother | `300-48.0216.5586`, `324-48.0216.5586` | A.2 |
| Mother | `41.0222.0026` | A.1 |
| Daughter 1 | `300-40.0219.9078`, `324-40.0219.9078` | A.3 |
| Daughter 1 | `11.0238.4030`, `37623.6` | A.3 |
| Flip-dot carrier | `37624.0 DV2 A.1` / `S7D5-2` | |

The paired `300-` and `324-` prefixes on the same number probably distinguish
the bare board from the populated assembly, which is usual German practice. That
would make `48.0216.5586` and `40.0219.9078` the real design numbers.

## The serial connector

A DB37 with four conductors populated, inside a grey overall-shielded cable.
They land on a terminal block on daughter 1.

| Wire | Transceiver pin | Direction |
| --- | --- | --- |
| green | `A` (6) | Display receives |
| yellow | `B` (7) | Display receives |
| blue | `A` (6) | Display transmits |
| red | `B` (7) | Display transmits |
| grey shield | | Chassis and drain |

**The DB37 pin numbers were never traced.** Identify the conductors by colour at
the connector.

### Why it is 4-wire RS-422

An SN75176BP holds one driver and one receiver sharing a single `A`/`B` pair.
One of them is a 2-wire half-duplex transceiver. Two of them, each half used, is
the standard 4-wire full-duplex arrangement, and one cable pair lands on each.

Two independent half-duplex channels would fit two transceivers equally well,
but three things rule that out: there is only one USART in the unit, there are
only three optocouplers crossing the isolation barrier (enough for one transmit,
one receive and one control line), and each transceiver is half used.

On the transceiver carrying green and yellow, the receiver enable, driver enable
and driver input are all strapped to ground. That package can only receive, so
green and yellow are the inbound pair.

There is no 120 Ω termination anywhere on the board, which is normal: a bus
terminates at its two ends, and a station sign is unlikely to be an end node.

### The motherboard terminal block

The motherboard carries its own four-way terminal block, unconnected in this
unit, labelled `RP RN TP TN`. Those are differential pair names, receive and
transmit, positive and negative. Nothing identified on the motherboard can drive
a differential pair, which leaves U8. What it connects to was never traced.

## The panel ribbon

| Property | Value |
| --- | --- |
| Conductors | 26, counted at the splice |
| Type | Very likely 26-way IDC |
| Pinout | Not traced |

Expected allocation: six or seven conductors for the encoded address, one clock,
one data, one or two control, and the rest for 24 V and returns. To trace which
is which, look for the group carrying a binary count, the two that toggle per
bit, and the supply.

## Power

Two separate mains feeds enter the enclosure. One runs the 24 V supply and so
the electronics. The other runs the fluorescent tube, with its ballast inside
the display body.

| Rail | Source |
| --- | --- |
| 24 V DC | External supply. Goes to the coils through the Darlington sinks |
| 5 V logic | Two TO-220 regulators on the motherboard |
| 5 V line side | The isolated Melcher converter on daughter 1 |

Separating the mains feeds is what a sign needs if it must run all day while its
backlight follows the station lighting. The panel is bistable and readable in
daylight, so losing the tube does not blank the sign.

### Before working inside

* **Mains is live in two places**, because of the tube's ballast. Treat the
  interior as live whenever either feed is connected.
* **Disconnect the tube feed** before probing anything. It removes both the
  hazard and a large inductive noise source sitting next to the logic.
* Bring a stored unit up on a current-limited supply. The electrolytics may have
  gone leaky.

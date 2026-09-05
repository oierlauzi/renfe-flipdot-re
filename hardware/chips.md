# Chip inventory

Reference designators as silkscreened. Unknown parts are listed with their
package so they can be identified later; see
[../docs/open-questions.md](../docs/open-questions.md).

## Index

| Ref | Part | PCB | I/O window | Role |
| --- | --- | --- | --- | --- |
| U1 | MSM80C85AH | Mother | — | CPU, 6.144 MHz crystal (3.072 MHz CLK OUT) |
| U2 | M81C55-5 | Mother | `0xA0-0xA5` + RAM `0xA000` | RAM, DIP switch inputs, UART baud clock |
| U3 | M82C51A-2 | Mother | `0x80-0x81` | USART, external serial link |
| U4 | M27256F1 | Mother | `0x0000-0x7FFF` | 32 KiB EPROM, labelled `TELENORMA 18 JUN 96` |
| U5, U11 | MC14584BCP | Mother, D1 | — | Hex Schmitt trigger |
| U6 | 7555 | Mother | — | Timer (watchdog / reset?) |
| U7 | 6N139 | Mother? | — | Optocoupler; **board attribution doubtful**, see below |
| U8-U10 | ? | Mother | ? | unidentified; **U9 (DIP16) is an RS-422 driver/receiver candidate** |
| (3 unlabelled) | 6N139 (yellow DIP8) | D1 | — | Optocouplers: the isolation barrier |
| (2 unlabelled) | **SN75176BP** (black DIP8) | D1 | — | **RS-485/422 differential transceivers** |
| U17-U19 | ULN2003A (U17/U18 TI, U19 ST) | D2 | — | Darlington sinks, flip-dot drive; **21 sinks** in total |
| U20 | M81C55-5 | D2 | `0xE0-0xE5` | Flip-dot interface; same part as U2 |

D1 = daughter 1, D2 = daughter 2. See [pcbs.md](pcbs.md) and
[../docs/io-map.md](../docs/io-map.md).

## Clock chain **[C] except the last link**

```
6.144 MHz crystal  ->  8085 X1/X2 (pins 1-2)
                       8085 divides by 2
                   ->  CLK OUT (pin 37) = 3.072 MHz
                   ->  81C55 TIMER IN (pin 3)          [to be confirmed]
                       divide by the switch-selected 20...640
                   ->  81C55 TIMER OUT (pin 6)
                   ->  8251A TxC and RxC (pins 9, 25)  [to be confirmed]
                       divide by 16 (x16 mode)
                   ->  300 - 9600 baud
```

---

## U1 — Microprocessor
- **Manufacturer:** OKI
- **Part number:** MSM80C85AH
- **PCB:** Mother
- **Datasheet:** [https://www.silicon-ark.co.uk/datasheets/m80c85AH-datasheet-oki.pdf]
- **Notes:** Intel 8085 clone. Only RST5.5 (`0x0260`) and RST7.5 (`0x02A9`) are
  used; every other vector traps to `0x034B`.
- **Clock:** a **6.144 MHz crystal** sits beside the CPU (observed 2026-08). The
  8085 halves it internally, so the machine runs at **3.072 MHz** and CLK OUT
  (pin 37) is 3.072 MHz. That fixes the entire baud ladder at 300-9600 — see
  [../docs/dip-switches.md](../docs/dip-switches.md#confirmed-by-the-crystal-c).
  One instruction cycle is 325.5 ns.

## U2 — Static RAM with I/O Ports and Timer
- **Manufacturer:** OKI
- **Part number:** M81C55-5
- **PCB:** Mother
- **Datasheet:** [https://docs.rs-online.com/1f1a/0900766b800264a3.pdf]
- **Notes:** The system's only RAM (256 B at `0xA000`). Its I/O section is at
  ports `0xA0-0xA5`: ports A/B/C are read-only in the firmware and carry the
  three DIP switch banks; the timer (`0xA4`/`0xA5`) generates the 8251A clock.
  **Measuring TIMER IN on this chip pins down the baud rate** — see
  [../docs/io-map.md](../docs/io-map.md).

## U3 — USART
- **Manufacturer:** OKI
- **Part number:** M82C51A-2
- **PCB:** Mother
- **Datasheet:** [https://www.cryptomuseum.com/crypto/rs/pu104/files/M82C51_datasheet.pdf]
- **Notes:** Intel 8251A clone; the entire external interface. Data at port
  `0x80`, mode/command/status at `0x81`. Initialised to 8 data bits / even
  parity / 1 stop / x16 clock at `0x0443`. Clocked by the U2 timer.

## U4 — EPROM
- **Manufacturer:** STMicroelectronics
- **Part number:** M27256F1
- **PCB:** Mother
- **Datasheet:** [https://deramp.com/downloads/mfe_archive/050-Component%20Specifications/ST-SGS/ST%20M27256.pdf]
- **Notes:** 32 KiB UV-EPROM. Only `0x0000-0x3047` is programmed; the rest is
  erased. Dump and provenance in [../original/](../original/).

## U5 — Hex Schmitt Trigger
- **Manufacturer:** Motorola
- **Part number:** MC14584BCP
- **PCB:** Mother
- **Datasheet:** [https://www.onsemi.com/download/data-sheet/pdf/mc14584b-d.pdf]
- **Notes:**

## U6 — Low-power NE555
- **Manufacturer:** 
- **Part number:** 7555
- **PCB:** Mother
- **Datasheet:** [https://www.nxp.com/docs/en/data-sheet/ICM7555.pdf]
- **Notes:** Function unconfirmed. A 555 next to an 8085 is usually either the
  power-on reset generator or a watchdog. Worth checking whether its output
  reaches the 8085 RESET IN pin.

## U7 — High Speed Optocoupler
- **Manufacturer:** Hewlett-Packard
- **Part number:** 6N139
- **PCB:** Mother — **but this now needs re-checking**
- **Datasheet:** [https://www.vishay.com/docs/83605/6n139.pdf]
- **Notes:** Recorded early on as the motherboard's serial RX isolation. Two later
  findings make the board attribution doubtful:

  1. Daughter 1 has **three 6N139s of its own**, which is the complete isolation
     barrier for three signals. A fourth would be redundant.
  2. The board architecture puts media conversion — and therefore isolation — on
     daughter 1, not the motherboard
     ([pcbs.md](pcbs.md#architecture-one-function-per-board-l)).

  Either `U7` is on daughter 1 and the board was misrecorded, or the motherboard
  really does carry a fourth optocoupler doing something else. **Check which board
  the `U7` silkscreen is actually on.**

## U8 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Mother
- **Datasheet:** []
- **Notes:**

## U9 — ? **DIP16, now worth reading first**
- **Manufacturer:** 
- **Part number:**
- **PCB:** Mother
- **Datasheet:** []
- **Notes:** DIP16 package. The motherboard has an unconnected `RP RN TP TN`
  terminal block — differential pair naming — and **`AM26LS31` (quad differential
  driver) and `AM26LS32` (quad differential receiver) are both DIP16**, the
  standard RS-422 parts of the era. If U9 is one of those, the motherboard has a
  line interface of its own, which would also explain why `U7` (a 6N139) sits on
  this board rather than on daughter 1. **[?]** — a guess from package size.
  → [connectors.md](connectors.md#motherboard-terminal-block--rp-rn-tp-tn-c-observed-unconnected)

## U10 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Mother
- **Datasheet:** []
- **Notes:** DIP20 Package

## U11 — Hex Schmitt Trigger
- **Manufacturer:** Motorola
- **Part number:** MC14584BCP
- **PCB:** Daughter 1
- **Datasheet:** [https://www.onsemi.com/download/data-sheet/pdf/mc14584b-d.pdf]
- **Notes:** 

## The five DIP8s on daughter 1 — identified **[C]**

Read by the owner, 2026-09. They carry **no reference designators** on the
silkscreen, so they are described by body colour and position.

### Three yellow DIP8s — HP 6N139 optocouplers
- **Manufacturer:** Hewlett-Packard / Agilent
- **Part number:** 6N139
- **PCB:** Daughter 1
- **Datasheet:** [https://www.vishay.com/docs/83605/6n139.pdf]
- **Notes:** High-speed optocouplers forming the isolation barrier between the
  motherboard's logic and the line side. **Three** of them for three signals —
  transmit data, receive data and the driver enable — matching the three 560 Ω
  LED-drive resistors and three ~1 kΩ pull-ups.

### Two black DIP8s — TI SN75176BP differential bus transceivers
- **Manufacturer:** Texas Instruments
- **Part number:** SN75176BP
- **PCB:** Daughter 1
- **Datasheet:** [https://www.ti.com/lit/ds/symlink/sn75176b.pdf]
- **Notes:** **These settle the physical layer: the link is RS-485 / RS-422, not a
  current loop.** Each package holds one driver and one receiver sharing a single
  `A`/`B` pair, with separate `DE` and `RE` enables — so a single one is a 2-wire
  half-duplex transceiver, and **two are what you fit for 4-wire full duplex**,
  using the driver of one and the receiver of the other. Four conductors are
  populated on the DB37, which fits.
  → [connectors.md](connectors.md#the-physical-layer-opto-isolated-rs-422rs-485-c)

  Earlier revisions of this file recorded these five packages as unidentified and
  built an argument on their absence: *"an RS-422 interface would use a
  differential transceiver; none is present"*. Two were, unread, the whole time.

## U17 — 7 NPN Darlington Pairs
- **Manufacturer:** Texas Instruments
- **Part number:** ULN2003AN
- **PCB:** Daughter 2
- **Datasheet:** [https://www.ti.com/lit/ds/symlink/uln2003a.pdf]
- **Notes:** With U18 and U19 this makes **21 Darlington sinks** on daughter 2 —
  see [../docs/flipdot-interface.md](../docs/flipdot-interface.md).

## U18 — 7 NPN Darlington Pairs
- **Manufacturer:** Texas Instruments
- **Part number:** ULN2003A
- **PCB:** Daughter 2
- **Datasheet:** [https://www.ti.com/lit/ds/symlink/uln2003a.pdf]
- **Notes:** Drives the flip-dot coils. Being current *sinks* is consistent with
  the firmware complementing (`CMA`) every value before writing it to the display
  ports — i.e. active-low drive.

## U19 — 7 NPN Darlington Pairs
- **Manufacturer:** STMicroelectronics
- **Part number:** ULN2003A
- **PCB:** Daughter 2
- **Datasheet:** [https://www.st.com/resource/en/datasheet/uln2001.pdf]
- **Notes:** **Third** ULN2003A on daughter 2, bringing the total to **21
  Darlington sinks**. A second-source part from a different manufacturer to
  U17/U18, which is ordinary production practice.

## U20 — Static RAM with I/O Ports and Timer
- **Manufacturer:** OKI
- **Part number:** M81C55-5
- **PCB:** Daughter 2
- **Datasheet:** [https://docs.rs-online.com/1f1a/0900766b800264a3.pdf]
- **Notes:** **The same part as U2** (owner read the top marking, 2026-09),
  serving an entirely different role: its I/O section at `0xE0-0xE5` is the
  flip-dot interface. Port A carries glyph data, port B the encoded character
  address, and port C the shift clock, serial data and the two attribute lines.

  Its 256 bytes of RAM appear to be unused — the firmware's only RAM is U2's at
  `0xA000`, and nothing addresses a second block.

  **Port C is in ALT2**, all six pins outputs, which is what the flip-dot drive
  requires. An earlier revision of this file argued at length that the command
  words selected **ALT4** and that this contradicted the 81C55 reading. That was
  a decode error here, not a fault in the hardware: ALT4 is command bits 3:2 =
  `10`, and both words carry `11`. See
  [../docs/io-map.md](../docs/io-map.md#port-c-is-in-alt2--all-six-pins-are-outputs-c).

# Chip inventory

Reference designators as silkscreened. Unknown parts are listed with their
package so they can be identified later; see
[../docs/open-questions.md](../docs/open-questions.md).

## Index

| Ref | Part | PCB | I/O window | Role |
| --- | --- | --- | --- | --- |
| U1 | MSM80C85AH | Mother | — | CPU, 6.144 MHz crystal (3.072 MHz CLK OUT) |
| U4 | M27256 | Mother | `0x0000-0x7FFF` | 32 KiB EPROM, labelled `TELENORMA 18 JUN 96` |
| U2 | M81C55-5 | Mother | `0xA0-0xA5` + RAM `0xA000` | RAM, DIP switch inputs, UART baud clock |
| U3 | M82C51A-2 | Mother | `0x80-0x81` | USART, external serial link |
| U4 | M27256F1 | Mother | — | 32 KiB EPROM (firmware) |
| U5, U11 | MC14584BCP | Mother, D1 | — | Hex Schmitt trigger |
| U6 | 7555 | Mother | — | Timer (watchdog / reset?) |
| U7 | 6N139 | Mother | — | Optocoupler, serial RX isolation |
| U8-U10 | ? | Mother | ? | unidentified |
| U12-U17 | ? (DIP8) | D1 | — | unidentified; likely optocouplers |
| U18a, U18b | ULN2003A | D2 | — | Darlington sinks, flip-dot drive |
| U19 | ULN2003A (ST) | D2 | — | Third Darlington array; 21 sinks in total |
| U20 | ? (DIP40) | D2 | `0xE0-0xE5`? | unidentified; **81C55 candidate** |

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
- **Manufacturer:** 
- **Part number:** HP 6N139
- **PCB:** Mother
- **Datasheet:** [https://www.vishay.com/docs/83605/6n139.pdf]
- **Notes:** Isolates the incoming serial line. **The key clue to the physical
  layer**: an opto-isolated receiver here means a 20 mA current loop or isolated
  RS-422, not plain RS-232. The series resistors feeding its input give the
  expected drive current — see [connectors.md](connectors.md).

## U8 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Mother
- **Datasheet:** []
- **Notes:**

## U9 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Mother
- **Datasheet:** []
- **Notes:** DIP16 Package

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

## U12 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** DIP8 Package

## U13 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** DIP8 Package

## U14 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** Yellow DIP8 package — the colour suggests an optocoupler. Candidate for the serial TX isolation; see [connectors.md](connectors.md).

## U15 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** Yellow DIP8 package — the colour suggests an optocoupler. Candidate for the serial TX isolation; see [connectors.md](connectors.md).

## U16 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** Yellow DIP8 package — the colour suggests an optocoupler. Candidate for the serial TX isolation; see [connectors.md](connectors.md).

## U17 — ?
- **Manufacturer:** 
- **Part number:**
- **PCB:** Daughter 1
- **Datasheet:** []
- **Notes:** Yellow DIP8 package — the colour suggests an optocoupler. Candidate for the serial TX isolation; see [connectors.md](connectors.md).

## U18a — 7 NPN Darlington Pairs
- **Manufacturer:** Texas Instruments
- **Part number:** ULN2003A
- **PCB:** Daughter 2
- **Datasheet:** [https://www.ti.com/lit/ds/symlink/uln2003a.pdf]
- **Notes:** One of two packages sharing the U18 designator on the silkscreen.
  Drives the flip-dot coils. Being current *sinks* is consistent with the
  firmware complementing (`CMA`) every value before writing it to the display
  ports — i.e. active-low drive.

## U18b — 7 NPN Darlington Pairs
- **Manufacturer:** Texas Instruments
- **Part number:** ULN2003AN
- **PCB:** Daughter 2
- **Datasheet:** [https://www.ti.com/lit/ds/symlink/uln2003a.pdf]
- **Notes:** Second package. With U19 also a ULN2003A there are **21 Darlington
  sinks** on daughter 2 — see
  [../docs/flipdot-interface.md](../docs/flipdot-interface.md). Confirm the
  designators against the silkscreen; one may actually be U21.

## U19 — 7 NPN Darlington Pairs
- **Manufacturer:** STMicroelectronics
- **Part number:** ULN2003A
- **PCB:** Daughter 2
- **Datasheet:** [https://www.st.com/resource/en/datasheet/uln2001.pdf]
- **Notes:** **Third** ULN2003A on daughter 2, bringing the total to **21
  Darlington sinks**. A second-source part from a different manufacturer to
  U18a/U18b, which is ordinary production practice.

## U20 — ?
- **Manufacturer:**
- **Part number:**
- **PCB:** Daughter 2
- **Datasheet:** []
- **Notes:** DIP40 package. **Strong 81C55/8155 candidate**: the firmware
  initialises a register group at I/O `0xE0` in exactly the 8155 layout
  (command, PA, PB, PC, timer LSB, timer MSB), and an 8155 is a DIP40 part.
  Reading the top marking would confirm the whole `0xE0` interpretation in
  [../docs/io-map.md](../docs/io-map.md).

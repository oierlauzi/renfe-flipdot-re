# External references

## Datasheets

Component-level links live in [hardware/chips.md](../hardware/chips.md) next to
each part. Only the four that matter for the protocol are repeated here:

| Part | Role | Datasheet |
| --- | --- | --- |
| OKI MSM80C85AH | CPU | <https://www.silicon-ark.co.uk/datasheets/m80c85AH-datasheet-oki.pdf> |
| OKI M81C55-5 | RAM + I/O ports + timer | <https://docs.rs-online.com/1f1a/0900766b800264a3.pdf> |
| OKI M82C51A-2 | USART | <https://www.cryptomuseum.com/crypto/rs/pu104/files/M82C51_datasheet.pdf> |
| HP 6N139 | Optocoupler (serial RX isolation) | <https://www.vishay.com/docs/83605/6n139.pdf> |

The Intel originals are the better reference for register-level behaviour:
search for the **Intel 8085AH**, **8155/8156**, and **8251A** datasheets. The
8251A one in particular has the mode/command word bit tables used throughout
[io-map.md](io-map.md).

## Still to find

- [ ] Any PAGE Ibérica **PI-T100** documentation: service manual, installation
      guide, protocol specification. PAGE Ibérica still exists as a display
      manufacturer in Tres Cantos, Madrid — worth an email.
- [ ] **Telenorma / Telefonbau und Normalzeit / Bosch Telecom** display and clock
      system documentation. The EPROM is labelled `TELENORMA 18 JUN 96`, so the
      firmware plausibly originates there rather than with PAGE Ibérica — see
      [device.md](device.md#dating-and-the-telenorma-connection-c-for-the-label).
      Search terms worth trying:
      - Telenorma *Anzeigetafel* / *Fahrgastinformation* (passenger information)
      - Telefonbau und Normalzeit *Nebenuhr* / *Hauptuhr* (slave / master clock)
      - Bosch Telecom station display protocol
      - German railway (DB) passenger-information display buses of the 1990s
- [ ] German clock and time-distribution **bus protocols** of the era. If the
      serial link descends from one, the framing decoded in
      [serial-protocol.md](serial-protocol.md) may be documented somewhere under
      a Telenorma or Bosch part number.
- [ ] RENFE Cercanías platform-signage installation or maintenance notes from
      the era.
- [ ] A slave unit, or photos of one — comparing master and slave firmware would
      settle the addressing question immediately.

## Prior art worth reading

Nothing specific to this unit has been found. General flip-dot reverse
engineering write-ups (Luminator, Mobitec, BRose bus destination signs) are
useful for the *mechanical* drive side — coil pulse widths, H-bridge patterns,
why you must not leave a coil energised — but their protocols are unrelated.

Add anything found here, with a date and a one-line note on why it was useful.

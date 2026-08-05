# Serial protocol

**This is the primary deliverable of the project.** Everything else exists to
make this document correct.

Goal: send bytes into the DB37 connector and get arbitrary text onto the
16-character panel, with no hardware modification.

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Physical layer

| Property | Value | Conf. |
| --- | --- | --- |
| Connector | DB37 female, only 4 conductors populated | **[C]** |
| Wire colours | orange, blue, green, yellow (grey overall shield) | **[C]** |
| Isolation | HP/Agilent 6N139 optocoupler (U7) on the motherboard | **[C]** |
| Signalling | 2 pairs — either RS-422/RS-485 or a 20 mA current loop | **[L]** |
| Pinout | **unknown** | **[?]** |

The 6N139 is the decisive clue: an opto-isolated receiver on a 1980s-90s
Spanish rail installation almost always means either a 20 mA current loop or an
isolated RS-422 link, not plain RS-232. Two pairs is consistent with
full-duplex (TX pair + RX pair). See [../hardware/connectors.md](../hardware/connectors.md)
for the measurement plan.

## Link layer **[C] unless noted**

From the 8251A initialisation (see [io-map.md](io-map.md#0x80-0x81--m82c51a-2-usart)):

| Property | Value |
| --- | --- |
| Mode | asynchronous |
| Data bits | 8 |
| Parity | even |
| Stop bits | 1 |
| Baud factor | x16 |
| Default line rate | **9600 baud [L]** — 81C55 timer divider 20, assuming a 3.072 MHz TIMER IN |

So the default is **9600 8E1**, pending a scope measurement of TIMER IN.

Both the frame format and the divider are overridden from the DIP switches at
power-on (`0x0533` writes the mode word from a register; `0x0474` picks the
timer preload from port B bits 7:5), so a unit in the field may be running
something else — see [dip-switches.md](dip-switches.md).

## Application layer — NOT YET REVERSE ENGINEERED

Nothing below the link layer is established yet. What is known:

* The sticker reads **MAESTRA** ("master"), and the firmware probes for a
  second display board (`IN 0xE7`, `0x040F`). A master/slave addressing scheme
  on the serial bus is therefore likely, meaning frames probably carry a
  **display address** taken from the DIP switches. **[L]**
* `0x03A0` polls a RAM byte against the constants `0x4A` and `0x4E` under
  `DI`/`EI` — the shape of a receive-state machine driven by an ISR. **[L]**
* The main loop at `0x03B2` waits for `0xA07E & 0xC0 == 0x40` and
  `0xA07C & 0x01 == 0` before rendering 0x61 (97) items from a buffer at
  `0xA039`. Those are almost certainly the "message ready" flags. **[L]**

### Where to look next

1. Follow the RX path: `IN 0x80` sites (`0x08D8`, `0x093F`, …) back to the
   buffer they fill, and forward to the state machine at `0x03A0`.
2. Decode the constants `0x4A`/`0x4E` — plausibly framing characters
   (`0x4A` = `'J'`, `0x4E` = `'N'`) or state enum values.
3. Look for the usual framing bytes near the RX code: `STX` `0x02`,
   `ETX` `0x03`, `SOH` `0x01`, `EOT` `0x04`, `ACK` `0x06`, `NAK` `0x15`.
4. Check whether the reply path (`OUT 0x80`, only 2 sites) sends an ACK/NAK —
   if it does, the frame format is much easier to confirm by experiment.

## Protocol scratchpad

Record every hypothesis and every experiment result here, including the ones
that failed — a negative result is data.

| Date | Frame sent (hex) | Expected | Observed | Verdict |
| --- | --- | --- | --- | --- |
| | | | | |

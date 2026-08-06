# Building a replacement controller

Planning notes for the host side. **The electrical standard is not yet
identified** — see
[hardware/connectors.md](../hardware/connectors.md) — so this compares the
candidates rather than picking one.

The good news: every candidate is an ordinary asynchronous UART at a low baud
rate. Only the *physical layer* differs, and that is one small board either way.

## Sourcing, per candidate

| Standard | USB adapter | Pi / Arduino path | Cost | Difficulty |
| --- | --- | --- | --- | --- |
| **RS-485, 2-wire** | Everywhere. Generic CH340/FTDI + MAX485 dongles | MAX485 / SP3485 breakout on the UART pins | very low | trivial |
| **RS-422, 4-wire** | Available but less common; look for "USB to RS-422" rather than RS-485 | MAX490 / MAX3490 breakout (full duplex, separate driver and receiver) | low | trivial |
| **RS-232** | Everywhere, the classic USB-serial cable | MAX3232 level-shifter breakout | very low | trivial |
| **20 mA current loop** | Niche and expensive — industrial RS-232-to-current-loop converters | **DIY**: an optocoupler and two resistors per direction | ~nothing in parts | easy, but you build it |

### If it turns out to be differential (RS-422/485)

The likeliest case: four conductors, full duplex, and the firmware manipulates
RTS with turnaround delays. **Check whether you need 4-wire.** Most cheap
USB-RS485 dongles are *2-wire half duplex* with automatic direction control, and
those cannot drive a 4-wire full-duplex link. For 4-wire you want either a
USB-RS422 adapter or a **MAX490**-class transceiver, which has an independent
driver and receiver rather than a shared bus.

### If it turns out to be a current loop

Do not hunt for a USB adapter. They exist, they are industrial, and they cost
more than the rest of the project. A loop interface is a handful of parts:

* **Driving the display's receive side** — you must *source* the loop current.
  A transistor or optocoupler output switching a current-limited supply
  (e.g. 24 V through a resistor sized for ~20 mA) in series with the display's
  RX pins. Your UART TX drives the optocoupler LED.
* **Reading the display's transmit side** — put a sense resistor in the loop and
  feed the voltage across it into an optocoupler or comparator, whose output goes
  to your UART RX.

The display's own receiver is a 6N139, a high-gain but not especially fast
optocoupler, which tells you the design never intended high speed. At 300-9600
baud almost any optocoupler works; a 6N137 or 6N139 is a safe match.

## Two gotchas that will cost an evening each

### Parity

The link uses **even parity** by default, and the port B switches can select odd
or none. That rules out some obvious-looking hardware:

* **Raspberry Pi**: on Pi 3 and later, `/dev/ttyS0` is the *mini-UART*, which
  **does not support parity at all**. You need the PL011 on `/dev/ttyAMA0` —
  either disable Bluetooth (`dtoverlay=disable-bt`) or, much simpler, ignore the
  GPIO UART and use a USB-serial dongle.
* **Arduino**: `SoftwareSerial` does not support parity. Use a *hardware* UART
  with `SERIAL_8E1`. On an Uno that means giving up the USB console, so a board
  with a spare UART (Mega, ESP32) is easier.

`pyserial` handles all of this on the host side without complaint
(`serial.Serial(port, baud, parity=serial.PARITY_EVEN)`).

### Isolation

The display's interface is opto-isolated for a reason: it is mains-powered, its
fluorescent ballast lives inside the enclosure, and it was designed to run down a
long cable in a station. **Preserve that isolation.** Either buy an isolated
adapter (RS-485 modules built on ADM2587E-class parts) or keep the optocouplers
on your own board. Do not tie a laptop's ground to the display chassis.

## What to buy before the standard is known

A hedge that covers every outcome for very little:

1. **A plain USB-to-TTL-serial adapter** (FT232R, CP2102 or CH340). Useful
   regardless, supports arbitrary parity, and becomes the UART behind whatever
   physical layer turns out to be needed.
2. **A MAX490 (full duplex) and a MAX485 (half duplex) breakout.** Covers both
   differential cases.
3. **A few optocouplers** (6N137/6N139) and assorted resistors. Covers the
   current loop case.
4. **A bench supply with current limiting.** Needed to power the unit safely
   anyway, and it is what sources the loop current if it is a current loop.

Total is roughly the price of one industrial converter, and it works whichever
way the measurement goes.

## Notes for whatever you build

* **The display transmits continuously.** Its status frame goes out forever with
  no prompting, so the host must tolerate a constant inbound stream and should
  not treat unexpected bytes as an error.
* **Allow at least 4 ms before the first reply byte.** The firmware runs a fixed
  line-turnaround delay of 3.9 ms before answering, then the frame itself takes
  5.7 ms at 9600 baud and 183 ms at 300. Budget generously — a `SUB` reply at
  300 baud takes roughly 1.8 seconds. See
  [serial-protocol.md](serial-protocol.md#timing-c).
* **`SUB` is the bring-up command.** `1D 7F 1A` — broadcast address, identify.
  It needs no prior state and returns a known-answer string, so it confirms the
  whole chain in one shot. See
  [serial-protocol.md](serial-protocol.md#query-commands-the-unit-reports-back-c).
* **Baud rate comes from the DIP switches**, anywhere from 300 to 9600. If the
  rate is unknown, sweep it while sending `1D 7F 1A` and watch for a reply.
* **Send text as CP437** (`text.encode("cp437")`), avoiding `$` and `%` — see
  [font.md](font.md).
* **The countdown is yours to compute.** The display does no arithmetic; the
  controller sends the whole 16-character line and re-sends it as the ETA
  changes. See [device.md](device.md).

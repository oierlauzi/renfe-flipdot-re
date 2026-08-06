# Building a replacement controller

Planning notes for the host side. **The electrical standard is not yet
identified** — see
[hardware/connectors.md](../hardware/connectors.md) — so this compares the
candidates rather than picking one.

The good news: every candidate is an ordinary asynchronous UART at a low baud
rate. Only the *physical layer* differs, and that is one small board either way.

## Choosing a host

| Host | UARTs with parity | Networking | Logic level | Verdict for this project |
| --- | --- | --- | --- | --- |
| **ESP32** | 3 hardware UARTs, `SERIAL_8E1` supported | WiFi + BT built in | **3.3 V** | **Best fit.** Replaces the original controller outright |
| Raspberry Pi | PL011 only; the mini-UART has **no parity** | WiFi on most models | 3.3 V | Fine, but heavier than the job needs |
| Arduino (AVR) | Hardware UART only; `SoftwareSerial` has **no parity** | none without a shield | 5 V | Workable; an Uno costs you the USB console |
| PC + USB dongle | any | n/a | n/a | Best for *bring-up and experimentation* |

### Why the ESP32 suits this device particularly well

Not just "it also works" — it maps onto the problem almost exactly:

* **The display computes nothing.** Destination text, the minute countdown and
  the layout were all the upstream controller's job ([device.md](device.md)). An
  ESP32 can be that controller in full: fetch or accept data over WiFi, get the
  time from NTP, compute the ETA, format 16 characters, and send the frame — with
  no PC involved.
* **Parity is free.** Three hardware UARTs, and `Serial1.begin(9600, SERIAL_8E1,
  rx, tx)` just works. This is the single thing that trips up both of the obvious
  alternatives.
* **It fits inside the enclosure**, powered from the existing 24 V rail through a
  small buck converter, so the finished object stays a self-contained sign rather
  than a display tethered to a laptop.
* **If the link turns out to be 2-wire RS-485**, the ESP32's UART peripheral has a
  native half-duplex mode (`UART_MODE_RS485_HALF_DUPLEX`) that drives the DE pin
  in hardware, removing the usual turnaround headache.
* The **3.9 ms reply turnaround and the continuous status frame**
  ([serial-protocol.md](serial-protocol.md#timing-c)) are trivial for an
  interrupt-driven hardware UART.

### The ESP32 catch: 3.3 V logic

Everything on the display side is 5 V-era CMOS, so check levels at every boundary.

**Transceivers.** Use genuine 3.3 V parts — **MAX3485** (half duplex) or
**MAX3490** (full duplex), not the 5 V MAX485/MAX490 that most cheap breakout
modules carry. A 5 V module will usually *accept* 3.3 V as a logic high, but its
receiver output swings to 5 V straight into a 3.3 V pin. That is the classic way
to damage an ESP32 on this kind of project.

**Optocoupler drive**, if it turns out to be a current loop. The original board
uses 560 Ω from 5 V for about 6.5 mA. From 3.3 V that same resistor gives only
3.5 mA, so scale it:

| Resistor | from 3.3 V | from 5 V |
| --- | --- | --- |
| 180 Ω | 10.8 mA | 20.3 mA |
| **220 Ω** | **8.9 mA** | 16.6 mA |
| **330 Ω** | **5.9 mA** | 11.1 mA |
| 560 Ω | 3.5 mA | 6.5 mA (the original) |

220-330 Ω from an ESP32 GPIO reproduces the original's drive current, and stays
well inside the ~12 mA per-pin continuous figure.

**Other ESP32 specifics:** avoid the strapping pins (GPIO0, 2, 12, 15) for UART;
keep UART0 for the USB console and use UART1 or UART2 for the display; and give
it a supply that tolerates WiFi transmit bursts, or it will brown out
mid-transmission.

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
* **Arduino (AVR)**: `SoftwareSerial` does not support parity. Use a *hardware*
  UART with `SERIAL_8E1`. On an Uno that means giving up the USB console, so a
  board with a spare UART (Mega) is easier.
* **ESP32**: no problem — three hardware UARTs, all supporting `SERIAL_8E1`.
  This is the main reason it is the recommended host above.

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
5. **An ESP32 dev board**, for the permanent installation once the link works.
   If buying transceivers specifically for it, get the **3.3 V** MAX3485/MAX3490
   rather than the 5 V parts.

Total is roughly the price of one industrial converter, and it works whichever
way the measurement goes.

A sensible division of labour: **bring the link up on a PC with a USB adapter**,
where you can iterate quickly and log everything, then **move the working
protocol onto an ESP32** for the finished object.

## Plan B: drive the panel directly

Worth knowing this exists, because it sidesteps the unresolved physical layer
entirely.

The flip-dot panel's ribbon carries **26 conductors**, and the drive format is
fully decoded: 16 per-character select lines, a shift clock, a serial data line
(active low), and 74 bits per character — see
[flipdot-interface.md](flipdot-interface.md#the-panel-is-a-serial-shift-register-c-from-the-rom).

An ESP32 with a shift register or port expander can produce that without
difficulty. No serial link, no current loop, no DB37, and **no modification to the
display** — only a different cable onto the existing ribbon header.

The trade-off is real, though: doing this bypasses the original firmware, so the
unit stops being a working PI-T100 and becomes a flip-dot panel with new
electronics. The stated goal is to drive it *through* its own firmware, which
preserves the machine as it was. Treat this as the fallback if the electrical
layer proves intractable, not the first choice.

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

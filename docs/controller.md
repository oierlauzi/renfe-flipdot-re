# Building a replacement controller

Planning notes for the host side.

**The electrical standard is settled: opto-isolated RS-485 / RS-422.** Daughter 1
carries two **TI SN75176BP** differential transceivers and three **6N139**
optocouplers
([hardware/connectors.md](../hardware/connectors.md#the-physical-layer-opto-isolated-rs-422rs-485-c)).
That is the cheapest and most ordinary outcome of the ones this document used to
hedge between — an off-the-shelf adapter talks to it.

Above the physical layer it is an ordinary asynchronous UART at a low baud rate,
8E1 by default, with the format and rate set by DIP switches.

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
* The **3.9 ms reply turnaround** ([serial-protocol.md](serial-protocol.md#timing-c))
  is trivial for an interrupt-driven hardware UART. Note the unit **replies but
  never announces itself**, so the host drives the exchange throughout.

### The ESP32 catch: 3.3 V logic

Everything on the display side is 5 V-era CMOS, so check levels at every boundary.

**Transceivers.** Use a genuine 3.3 V part — **MAX3490** (full duplex) — not the
5 V MAX490 that most cheap breakout modules carry. A 5 V module will usually *accept* 3.3 V as a logic high, but its
receiver output swings to 5 V straight into a 3.3 V pin. That is the classic way
to damage an ESP32 on this kind of project.

**Optocoupler drive**, if you build your own isolation rather than buying an
isolated module. The original board uses 560 Ω from 5 V into a 6N139, about
6.5 mA. From 3.3 V the same resistor gives only 3.5 mA, so scale it:

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

## Sourcing

| Part | Why | Cost |
| --- | --- | --- |
| USB-RS-422 adapter, or isolated RS-485 | Bring-up and experimentation on a PC | low |
| MAX3490 (full duplex, 3.3 V) | The permanent ESP32 build | very low |
| ADM2587E-class isolated module | If you would rather buy isolation than build it | moderate |

**It is 4-wire — settled by tracing**: each cable pair lands on a different
SN75176BP ([hardware/connectors.md](../hardware/connectors.md#it-is-4-wire-one-pair-per-transceiver-c)).
So buy a **4-wire RS-422** adapter. Most cheap USB-RS485 dongles are 2-wire half
duplex with automatic direction control and **will not drive this link**.

### Wiring

| Your adapter | Display wire | Display transceiver pin |
| --- | --- | --- |
| driver output `A` | **green** | `A` (pin 6) |
| driver output `B` | **yellow** | `B` (pin 7) |
| receiver input `A` | **blue** | `A` (pin 6) |
| receiver input `B` | **red** | `B` (pin 7) |
| — | grey overall shield | chassis / drain, one end only |

The display **receives** on green/yellow and **replies** on blue/red. The DB37
pin numbers are still untraced, so identify the conductors by colour at the
connector.

### Is an FT232RNL-based adapter suitable? **Only with a transceiver in front of it**

The FT232RNL is a **USB-to-UART** bridge: it produces logic-level serial, not
differential. The display needs **4-wire RS-422**, so the chip alone is not an
interface to it — but it is a perfectly good UART for the job, and everything the
protocol needs is within its range.

| Requirement | FT232RNL |
| --- | --- |
| 300-9600 baud | yes — the part goes from 300 baud upward |
| 8 data bits, **even parity**, 1 stop | yes, ordinary UART settings |
| Differential drive | **no — add a transceiver** |
| Isolation | **no** — see below |

**Pair it with a full-duplex transceiver: `MAX3490` (3.3 V) or `MAX490` (5 V).**
Not `MAX485`/`MAX3485` — those are half duplex, with driver and receiver sharing
one pair, which is the wrong shape for this link.

**The dongle trap.** Ready-made "FT232 + RS485" adapters are nearly all **2-wire
half duplex**, built as FT232 + MAX485 with automatic direction control. They
cannot drive a 4-wire link. Buy one sold explicitly as **RS-422** or **4-wire**,
or build it from a bare FT232RNL board and a MAX3490.

#### Five things that will each cost an evening

* **Set `VCCIO` deliberately.** It sets the I/O pin voltage, and in a bus-powered
  configuration it can end up as low as 1.8-2.8 V — below what a 5 V transceiver
  reads as a logic high. Match it to the transceiver: 3.3 V for a `MAX3490`, 5 V
  for a `MAX490`.
* **Turn the latency timer down.** FTDI's default read latency is **16 ms**. The
  display answers after a 3.9 ms line turnaround, so at 9600 baud the driver's
  latency dominates every exchange. Set it to 1-2 ms.
* **`DE` can be tied high.** As the master you drive a pair nobody else drives, so
  the driver stays enabled. `CBUS2` defaults to **`TXDEN`** if you ever do want
  hardware transmit-enable, but this link does not need it.
* **Bias the receive pair, or expect garbage.** Between replies the display's
  driver may be tri-stated, leaving your receiver's inputs floating — and a
  floating differential pair picks up noise, which a UART reads as framing errors
  and phantom bytes. This is very likely what the 2026-08 scope session was
  looking at ([../hardware/connectors.md](../hardware/connectors.md#this-probably-explains-the-scope-transitions-l)).
  Add fail-safe bias at your end: roughly 680 Ω from `A` to `VCC` and `B` to
  ground on the pair you receive on.
* **Termination is optional on a bench.** 120 Ω across each pair matters for long
  cables; over a metre at 9600 baud it is unnecessary, and the display carries
  none itself.

#### Isolation: less urgent than it looks, but do not skip it for a permanent build

The FT232RNL provides none. That is tolerable on the bench **because the display
already isolates**: its line side is separated from the mains-powered logic by the
three 6N139s and the Melcher converter, so a non-isolated adapter bonds your PC
ground to a **floating line side**, not to the display's chassis.

Two caveats:

* **There is no signal ground in the cable** — four conductors and a shield, no
  return. RS-422 receivers need the common mode within about ±7 V, and with the
  display's line side floating there is nothing defining it. Over a short bench
  lead this usually settles harmlessly, but **it is the first thing to suspect if
  the link is intermittent**. The grey overall shield is the candidate return.
* For anything permanent down a station-length cable, use an isolated adapter
  (ADM2587E-class) and put the isolation back.

**On `A`/`B` naming, which is a genuine trap.** The labels are inconsistent in the
field: TIA-485 and most transceiver datasheets disagree about which of `A`/`B` is
the inverting line, and modules variously silkscreen them `A`/`B`, `D+`/`D−` or
`TX+`/`TX−`. The display's parts are unambiguous on their own terms — an
SN75176B drives `A` high and `B` low for a logic-high input, and its receiver
outputs high when `A` > `B` — so **match `A` to `A` and `B` to `B` by the pin
function, not by a `+`/`−` label**.

If the link comes up silent, **swap one pair before assuming anything is broken**.
Reversed polarity is the single most common RS-485 wiring fault, it presents
exactly as silence, and it cannot damage anything.

Note also that the idle polarity *through the isolation barrier* has not been
traced — the 6N139s may invert — so do not predict the line's resting state from
the UART's. Try it, and swap if needed.

## Parity, and the hosts it rules out

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

Covered above under
[the FT232RNL notes](#isolation-less-urgent-than-it-looks-but-do-not-skip-it-for-a-permanent-build):
the display isolates its own line side, so a non-isolated adapter is tolerable on
the bench, but a permanent installation should put the isolation back.

## What to buy

Now that the standard is known, the list is short:

1. **A USB-to-RS-422 adapter — 4-wire.** Not a 2-wire RS-485 dongle; the link is
   confirmed 4-wire. An isolated 4-wire module (ADM2587E-class) is ideal. A bare
   FT232RNL board plus a `MAX3490` does the same job — see
   [the FT232RNL notes](#is-an-ft232rnl-based-adapter-suitable-only-with-a-transceiver-in-front-of-it).
2. **An ESP32 dev board** plus a **MAX3490** (3.3 V, full duplex, independent
   driver and receiver) for the permanent installation.
3. **A bench supply with current limiting**, to power the unit safely.

**Keep the isolation.** The display's interface is opto-isolated because it is
mains-powered, has a fluorescent ballast inside the enclosure, and was built to
run down a long station cable. Buy an isolated adapter or keep optocouplers on
your own board; do not tie a laptop's ground to the display chassis.

**Termination:** the unit carries no 120 Ω, which is normal for a mid-bus node.
Your end of the link will need it.

> **Removed: the hedging shopping list.** This section used to buy for every
> outcome — a MAX490 *and* a MAX485, plus 6N137/6N139 optocouplers and resistors
> "to cover the current loop case", plus a bench supply "to source the loop
> current". There is no current loop.

A sensible division of labour: **bring the link up on a PC with a USB adapter**,
where you can iterate quickly and log everything, then **move the working
protocol onto an ESP32** for the finished object.

## Plan B: drive the panel directly

Worth knowing this exists, because it sidesteps the unresolved physical layer
entirely.

The flip-dot panel's ribbon carries **26 conductors**, and the drive format is
largely decoded: an encoded module address on port B, a shift clock, a serial data
line (active low), and 74 bits per character — see
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

* **The display only speaks when spoken to.** An earlier note here said its
  status frame "goes out forever with no prompting"; that is retracted — the
  firmware transmits nothing unprompted. Still allow for unexpected inbound bytes,
  because a shared bus may carry another unit's replies, but do not rely on a
  constant stream to prove the link is alive.
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

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

### Choosing a USB adapter

**On the FT232RNL specifically:** it is a **USB-to-UART** bridge producing
logic-level serial, not differential. The chip *alone* is not an interface to this
display — but it is a perfectly good UART for the job, and a dongle that pairs one
with an RS-422 transceiver is exactly right. Everything the protocol needs is
within its range.

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

#### Three specific dongles, checked

All three are FTDI-based and all three claim 4-wire RS-422, so all three should
work. They differ in what they document and in how they handle the reference
problem below.

| | [Jhoinrch B0FX2PTXQ7](https://www.amazon.es/dp/B0FX2PTXQ7) | [DSD TECH SH-U11](https://www.deshide.com/product-details_SH-U11.html) | [Waveshare B0BTBR71PC](https://www.waveshare.com/usb-to-rs485-422.htm) |
| --- | --- | --- | --- |
| USB bridge | FT232RNL | FT232RL | FT232RL |
| Transceiver | undocumented | undocumented | SP485EEN |
| RS-422 4-wire | claimed | **yes** | **yes** |
| Terminal names | undocumented | `TxD+ TxD- RxD+ RxD- GND` | `TA TB RA RB PE` |
| Isolation | **no** — "TVS/ESD/OC" is protection, not isolation | **no** (the `SH-U11F` variant is) | **yes**, power and digital |
| 120 Ω termination | undocumented | undocumented | **jumper-selectable** |
| Vendor documentation | none found | product page | wiki + manual |

**Pick:** the **Waveshare** for anything permanent — isolation, surge protection
and selectable termination are all worth having on a station-length cable. The
**SH-U11** is the better bench tool for a first attempt, for one specific reason
given below. The **Jhoinrch** would probably work; it is only last because nothing
about it is documented.

If you want both properties, the **`SH-U11F`** is the isolated version of the
DSD TECH part and keeps the `GND` terminal.

##### The Waveshare's terminal names are an advantage here

`TA`/`TB` and `RA`/`RB` are named `A`/`B`, which maps **directly** onto the
display's transceiver pins with no `+`/`−` ambiguity:

| Waveshare | Display wire |
| --- | --- |
| `TA` | **green** (display RX `A`) |
| `TB` | **yellow** (display RX `B`) |
| `RA` | **blue** (display TX `A`) |
| `RB` | **red** (display TX `B`) |
| `PE` | grey shield / drain |

On the DSD TECH the mapping is the same idea through its `+`/`−` labels — start
with `TxD+` → green, `TxD-` → yellow, `RxD+` → blue, `RxD-` → red, and swap a pair
if the link is silent.

##### The reference problem, and why it favours the cheap one for bring-up

The display's line side **floats** (it is isolated from its own mains-powered
logic), and **the cable has no signal ground** — four conductors and a shield.
RS-422 receivers need the common mode inside roughly ±7 V, so something has to
define it.

* **A non-isolated adapter with a `GND` terminal** — the SH-U11 — defines it for
  you: its ground is your PC's, and tying it to the cable shield references the
  display's floating line side. This is safe here *because the display provides
  the isolation*, so you are bonding to a floating line side, not to a
  mains-powered chassis.
* **An isolated adapter** — the Waveshare — leaves **both** sides floating, with
  nothing tying their common mode together. Usually fine over a short lead, and
  `PE` to the cable shield is the intended remedy, but it is the more likely of
  the two to produce an intermittent link on a bench.

##### What none of them documents

**Fail-safe bias on the receive pair.** The display's driver may be tri-stated
between replies, and an unbiased receiver input then floats and delivers noise to
the UART as framing errors and phantom bytes. None of the three lists bias
resistors. If you see garbage between valid replies, that is the cause — add
roughly 680 Ω from `RA` to `VCC` and `RB` to ground, or filter at the software
layer.

**Termination default.** Waveshare's own page and manual disagree about whether
the 120 Ω is enabled by default. At 9600 baud over a bench lead it makes no
practical difference; leave it as shipped and only think about it for a long
cable.

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
[Choosing a USB adapter](#isolation-less-urgent-than-it-looks-but-do-not-skip-it-for-a-permanent-build):
the display isolates its own line side, so a non-isolated adapter is tolerable on
the bench, but a permanent installation should put the isolation back.

## What to buy

Now that the standard is known, the list is short:

1. **A USB-to-RS-422 adapter — 4-wire.** Not a 2-wire RS-485 dongle; the link is
   confirmed 4-wire. Three concrete candidates are compared under
   [Choosing a USB adapter](#choosing-a-usb-adapter); the Waveshare isolated unit
   is the pick for a permanent build, the DSD TECH `SH-U11` for bench bring-up.
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

## Bring-up: `tools/bringup.py`

A staged script for the first power-up, written against the SH-U11's wiring.
Each stage prints what a failure *means* rather than just failing.

```sh
pip install pyserial
python3 tools/bringup.py --port /dev/ttyUSB0        # every stage, in order
python3 tools/bringup.py --port /dev/ttyUSB0 --stage line
make bringup-selftest                               # run it against the emulator
```

| Stage | What it does | What a failure tells you |
| --- | --- | --- |
| `adapter` | device node exists; reads the FTDI **latency timer** | 16 ms default will dominate every exchange — it prints the `echo 1 \| sudo tee` fix |
| `quiet` | listens 3 s **sending nothing** | anything received is *not* the display, which never announces itself — almost certainly a floating, unbiased receive pair |
| `line` | sweeps baud x format sending broadcast `SUB`, looking for `AK0052.139U1` | silence everywhere means reversed pair polarity, or the unit is on handler A or C rather than the command protocol |
| `address` | sweeps `GS 0-31` to find the unit's own address | the DIP switch sense is unresolved, so the address is genuinely unknown until this runs |
| `demo` | identify, cursor, status, fill, write, read back | a mismatch after an `ACK` means the message was stored but not painted — `RS` releases the repaint interlock |

**The `line` stage is the one that earns its keep.** The switch *sense* is still
unknown ([dip-switches.md](dip-switches.md)), so the recorded switch positions give
two different answers — 4800 8N1 or 2400 7E1 — and the sweep simply tries both
first, then everything else. `SUB` is the right probe because it needs no prior
state, works under broadcast addressing, and returns a string known in advance.

`make bringup-selftest` runs the address and demo stages against the emulator, so
the script is debugged before the adapter arrives. It takes about 25 seconds,
most of it emulating the panel repaint.

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

# Driving the display from a modern host

The link is opto-isolated **4-wire RS-422**, at whatever line settings the DIP
switches select. This unit runs **1200 baud, 7 data bits, even parity, 1 stop, at
address 1**. Above the wire it is an ordinary asynchronous UART.

## What you need

1. **A USB-to-RS-422 adapter, 4-wire.** Not a 2-wire RS-485 dongle: most cheap
   ones are half duplex with automatic direction control and cannot drive this
   link. Candidates are compared [below](#adapters).
2. **`pyserial`**, and the tools in [../tools/](../tools/).
3. For a permanent build, an **ESP32** and a **MAX3490** — see
   [the ESP32 notes](#a-permanent-controller).

## Wiring

| Adapter | Display wire | Display transceiver pin |
| --- | --- | --- |
| driver output `A` | **green** | `A` (pin 6) |
| driver output `B` | **yellow** | `B` (pin 7) |
| receiver input `A` | **blue** | `A` (pin 6) |
| receiver input `B` | **red** | `B` (pin 7) |
| — | grey overall shield | chassis / drain, one end only |

The display **receives** on green/yellow and **replies** on blue/red. The DB37
pin numbers have not been traced, so identify the conductors by colour at the
connector.

**On `A`/`B` naming.** The labels are inconsistent in the field: TIA-485 and most
transceiver datasheets disagree about which of `A`/`B` is the inverting line, and
modules silkscreen them variously `A`/`B`, `D+`/`D−` or `TX+`/`TX−`. The display's
parts are unambiguous on their own terms — an SN75176B drives `A` high and `B` low
for a logic-high input, and its receiver outputs high when `A` > `B` — so match by
pin function rather than by a `+`/`−` label. If the link comes up silent, swap one
pair before assuming a fault: reversed polarity is the commonest RS-485 wiring
error, presents exactly as silence, and cannot damage anything.

The idle polarity *through* the isolation barrier has not been traced — the 6N139s
may invert — so the line's resting state cannot be predicted from the UART's.

## Adapters

Three FTDI-based units, checked against manufacturer documentation:

| | [Jhoinrch B0FX2PTXQ7](https://www.amazon.es/dp/B0FX2PTXQ7) | [DSD TECH SH-U11](https://www.deshide.com/product-details_SH-U11.html) | [Waveshare B0BTBR71PC](https://www.waveshare.com/usb-to-rs485-422.htm) |
| --- | --- | --- | --- |
| USB bridge | FT232RNL | FT232RL | FT232RL |
| Transceiver | undocumented | undocumented | SP485EEN |
| RS-422 4-wire | claimed | yes | yes |
| Terminal names | undocumented | `TxD+ TxD- RxD+ RxD- GND` | `TA TB RA RB PE` |
| Isolation | no | no; the `SH-U11F` variant is isolated | yes, power and signal |
| 120 Ω termination | undocumented | undocumented | jumper-selectable |

The **SH-U11** is what this project used, and it worked first time. Its `GND`
terminal is genuinely useful here: the display's line side floats, and the cable
carries no signal ground, so nothing otherwise defines the common mode. RS-422
receivers need it inside roughly ±7 V. Tying the adapter's `GND` to the cable
shield defines it, and that is safe because the display supplies the isolation —
you are bonding to a floating line side, not to a mains-powered chassis.

The **Waveshare** is the better choice for a permanent installation: isolation and
surge protection earn their keep on a station-length cable, and its terminals are
named `A`/`B`, which maps directly onto the display's transceiver pins with no
`+`/`−` ambiguity. Note that an isolated adapter leaves *both* ends floating, so
tie `PE` to the cable shield. The `SH-U11F` gives isolation and a `GND` terminal
together.

None of the three documents fail-safe bias, which is the failure most likely to
bite — see below.

### Settings that matter

* **Turn the FTDI latency timer down.** The default is 16 ms, against a 3.9 ms
  line turnaround, so it dominates every exchange. On Linux:
  `echo 1 | sudo tee /sys/bus/usb-serial/devices/ttyUSB0/latency_timer`.
* **Bias the receive pair** if you see garbage between valid replies. The display
  may tri-state its driver between replies, and an unbiased receiver input then
  floats and hands the UART noise as framing errors and phantom bytes. Roughly
  680 Ω from the receive `A` to VCC and `B` to ground fixes it.
* **Termination is optional on a bench.** 120 Ω across each pair matters for long
  cables; over a metre at 1200 baud it is unnecessary, and the display carries
  none itself — normal for a node that is not at the end of the bus.
* **Set `VCCIO` deliberately** if you are building from a bare FT232 board. It
  can sit at 1.8-2.8 V in a bus-powered configuration, below a 5 V transceiver's
  logic high.

## The tools

```sh
pip install pyserial
```

**[../tools/bringup.py](../tools/bringup.py)** — staged first power-up. Each
stage prints what a failure means rather than just failing.

```sh
python3 tools/bringup.py --port /dev/ttyUSB0        # every stage, in order
python3 tools/bringup.py --port /dev/ttyUSB0 --stage line
make bringup-selftest                               # run it against the emulator
```

| Stage | What it does |
| --- | --- |
| `adapter` | checks the device node and reports the FTDI latency timer |
| `quiet` | listens for 3 s sending nothing; anything received is line noise, not the display |
| `line` | sweeps baud and format sending broadcast `SUB`, looking for `AK0052.139U1` |
| `address` | sweeps `GS 0-31`, deselecting before each, to find the unit's address |
| `demo` | identify, cursor, status, fill, write, read back |

The `line` stage is what makes an unknown unit tractable: the switch mapping is
not established, so the line settings genuinely have to be discovered.

**[../tools/say.py](../tools/say.py)** — everyday use.

```sh
tools/say.py                      # type lines; each appears on the panel
tools/say.py CHAMARTIN            # one line and exit
echo ATOCHA | tools/say.py        # or from a pipe
tools/say.py --fill               # every dot on
tools/say.py --clear              # every dot off
tools/say.py --cycle 3            # fill/clear repeatedly, to find stuck dots
tools/say.py --identify           # version and config, as a health check
```

Defaults are this unit's configuration, overridable with
`--port/--baud/--bits/--parity/--address`, or `--sim` to drive the emulator.

**[../tools/pit100.py](../tools/pit100.py)** is the driver both are built on, and
implements the whole command set.

## Notes for whatever you build

* **The display answers; it never announces.** Nothing arrives unless you asked
  for it. Allow for unexpected inbound bytes, because a shared bus may carry
  another unit's replies, but do not rely on a stream to prove the link is alive.
* **Start every transaction with `RS`.** A unit left mid-command by an earlier
  session consumes the next byte as a parameter, which puts the following text at
  a stale write pointer and produces a spurious overflow `NAK`.
* **End every transaction with `RS`.** `ACK` means stored, not displayed; the
  repaint is held off while the unit is selected.
* **Allow at least 4 ms before the first reply byte**, plus the frame time — 5.7 ms
  at 9600 baud, 183 ms at 300. A `SUB` reply at 300 baud takes about 1.8 seconds.
* **Reply lengths vary.** Drain until the line goes quiet rather than reading a
  fixed count, or every read will fall one reply behind.
* **`SUB` is the bring-up command.** `1D 7F 1A` — broadcast, identify. No prior
  state, known answer.
* **Send text as CP437**, avoiding `$` and `%`, which are `ñ` and `Ñ` on this
  panel — see [font.md](font.md).
* **The countdown is yours to compute.** The display does no arithmetic on message
  content; send the whole line and re-send it as the ETA changes.

## A permanent controller

| Host | UARTs with parity | Networking | Logic level | Verdict |
| --- | --- | --- | --- | --- |
| **ESP32** | 3 hardware UARTs, `SERIAL_8E1` | WiFi and BT built in | 3.3 V | Best fit; replaces the original controller outright |
| Raspberry Pi | PL011 only; the mini-UART has no parity | WiFi on most models | 3.3 V | Workable, heavier than the job needs |
| Arduino AVR | hardware UART only; `SoftwareSerial` has no parity | none without a shield | 5 V | Workable; an Uno costs you the USB console |
| PC and USB dongle | any | — | — | Best for bring-up and experimentation |

The ESP32 maps onto the problem closely. The display computes nothing, so an
ESP32 can be the whole upstream controller: accept data over WiFi, get the time
from NTP, compute the ETA, format the line and send it. Parity is free — a
`uart_config_t` with `UART_DATA_7_BITS` and `UART_PARITY_EVEN` — which is the
thing that trips up both obvious alternatives. And it fits inside the enclosure,
powered from the existing 24 V rail through a small buck converter, so the
finished object stays a self-contained sign.

**This exists: [../controller/](../controller/).** An ESP-IDF project that drives
the panel over the original link, configured from a plain-HTML web UI, with modes
for a message, a Spanish date-and-time clock, a countdown, a count-up and a
playlist. Its encoder is checked against [../tools/pit100.py](../tools/pit100.py)
on the host with `make controller-check`, so the CP437 table and the accent fold
do not have to be trusted twice.

Two design points there are worth repeating here, because they are properties of
the display rather than of the firmware. A single task owns the UART, so the
repaint rate is bounded in one place no matter what asks for a frame; and a frame
identical to what is already displayed is never sent, because the panel is a
mechanical resource and silence is the cheapest thing to send it.

**The 3.3 V catch.** Everything on the display side is 5 V-era CMOS. Use a genuine
3.3 V transceiver — **MAX3490**, full duplex — not the 5 V MAX490 most cheap
breakout modules carry: a 5 V module usually accepts 3.3 V as a logic high, but
its receiver output swings to 5 V straight into a 3.3 V pin.

If you build your own isolation rather than buying an isolated module, the
original board uses 560 Ω from 5 V into a 6N139, about 6.5 mA. From 3.3 V the same
resistor gives 3.5 mA, so scale it:

| Resistor | from 3.3 V | from 5 V |
| --- | --- | --- |
| 180 Ω | 10.8 mA | 20.3 mA |
| **220 Ω** | **8.9 mA** | 16.6 mA |
| **330 Ω** | **5.9 mA** | 11.1 mA |
| 560 Ω | 3.5 mA | 6.5 mA, the original |

220-330 Ω from an ESP32 GPIO reproduces the original drive current and stays well
inside the roughly 12 mA per-pin continuous figure.

Other ESP32 specifics: avoid the strapping pins (GPIO0, 2, 12, 15) for the UART;
keep UART0 for the USB console and use UART1 or UART2 for the display; and give it
a supply that tolerates WiFi transmit bursts.

## Parity, and the hosts it rules out

The link uses even parity by default and the switches can select odd or none.
That rules out some obvious-looking hardware:

* **Raspberry Pi**: on Pi 3 and later `/dev/ttyS0` is the mini-UART, which does
  not support parity at all. Use the PL011 on `/dev/ttyAMA0` — either disable
  Bluetooth with `dtoverlay=disable-bt`, or ignore the GPIO UART and use a USB
  dongle.
* **Arduino AVR**: `SoftwareSerial` does not support parity. Use a hardware UART
  with `SERIAL_7E1`; on an Uno that means giving up the USB console.
* **ESP32**: no problem, three hardware UARTs all supporting parity.

`pyserial` handles all of it on the host side:
`serial.Serial(port, baud, parity=serial.PARITY_EVEN, bytesize=serial.SEVENBITS)`.

## Driving the panel directly

Worth knowing this exists. The flip-dot ribbon carries 26 conductors and the drive
format is decoded: an encoded module address on port B, a shift clock, a serial
data line, and 74 bits per character — see
[flipdot-interface.md](flipdot-interface.md). An ESP32 can produce that directly,
bypassing the original electronics entirely, with no modification to the display —
only a different cable onto the existing ribbon header.

It gives up the firmware's glyph table, effects and addressing, and it needs the
[safety note](flipdot-interface.md#safety-note) about bounded coil pulses taken
seriously. It is an alternative, not the route this project took.

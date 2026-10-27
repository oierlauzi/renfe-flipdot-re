# 2. Talking to it

The sign has one connector to the outside world, a DB37 with four conductors in
it. Those four carry an opto-isolated RS-422 link. Above the wire it is an
ordinary asynchronous serial port.

## The line

| Property | Value |
| --- | --- |
| Signalling | RS-422 differential, 4-wire full duplex, opto-isolated |
| Speed and format | DIP switch selectable: 300 to 9600 baud, 7 or 8 bits, odd, even or no parity, 1 or 2 stop bits |
| **This unit** | **1200 baud, 7 data bits, even parity, 1 stop, address 1** |

Another installation may be set differently. There is no way to know without
reading its switches or sweeping the line settings, and `tools/bringup.py --stage
line` does the sweep for you.

A 7-bit link strips the top bit of every byte. That is why the ROM carries a
second copy of `ñ` and `Ñ` down at `0x24` and `0x25`, inside the 7-bit range.
See [5-the-panel.md](5-the-panel.md#the-font).

## Wiring

| Adapter | Display wire | Transceiver pin |
| --- | --- | --- |
| driver output `A` | **green** | `A` (pin 6) |
| driver output `B` | **yellow** | `B` (pin 7) |
| receiver input `A` | **blue** | `A` (pin 6) |
| receiver input `B` | **red** | `B` (pin 7) |
| ground or shield | grey overall shield | chassis, one end only |

The display receives on green and yellow, and replies on blue and red. The DB37
pin numbers were never traced, so identify the conductors by colour at the
connector.

**On `A` and `B` naming.** The labels are inconsistent in the field. Modules are
silkscreened `A`/`B`, `D+`/`D-` or `TX+`/`TX-`, and the standards do not agree
about which line is the inverting one. The display's own parts are unambiguous,
so match by pin function. If the link comes up silent, swap one pair before
suspecting a fault: reversed polarity is the commonest wiring error here, it
looks exactly like silence, and it cannot damage anything.

## What you need

**A 4-wire RS-422 adapter, not a 2-wire RS-485 dongle.** Most cheap dongles are
half duplex with automatic direction control and cannot drive this link. This
project used a DSD TECH SH-U11, which worked first time. Its `GND` terminal
matters: the display's line side floats and the cable carries no signal ground,
so nothing else defines the common mode, and RS-422 receivers need it inside
roughly ±7 V. Tie the adapter's `GND` to the cable shield. That is safe because
the isolation is on the display's side.

For a permanent installation an isolated adapter is better, but then both ends
float, so bond its earth terminal to the shield instead.

Three settings are worth knowing:

* **Turn the FTDI latency timer down.** The default 16 ms dominates every
  exchange, against a 3.9 ms line turnaround. On Linux:
  `echo 1 | sudo tee /sys/bus/usb-serial/devices/ttyUSB0/latency_timer`.
* **Bias the receive pair** if you see rubbish between valid replies. About
  680 Ω from receive `A` to VCC and from `B` to ground.
* **Termination is optional on a bench.** 120 Ω across each pair matters on a
  long cable. Over a metre at 1200 baud it is not needed, and the display
  carries none itself.

### Hosts that will not work

The link uses parity, which rules out some obvious hardware:

* **Raspberry Pi**: on Pi 3 and later `/dev/ttyS0` is the mini-UART, which has
  no parity support at all. Use `/dev/ttyAMA0`, or a USB dongle.
* **Arduino AVR**: `SoftwareSerial` has no parity. Use a hardware UART with
  `SERIAL_7E1`.

`pyserial` handles it all on the host side.

## The scripts

```sh
pip install pyserial
```

**[../tools/say.py](../tools/say.py)** puts text on the panel. This is the one
you want.

```sh
tools/say.py CHAMARTIN            # one line and exit
tools/say.py                      # type lines, each appears on the panel
echo ATOCHA | tools/say.py        # or from a pipe
tools/say.py --fill               # every dot on
tools/say.py --clear              # every dot off
tools/say.py --cycle 3            # fill and clear, to find stuck dots
tools/say.py --identify           # version and configuration, as a health check
```

It defaults to this unit's settings. Override with
`--port/--baud/--bits/--parity/--address`, or use `--sim` to drive the emulator
instead of a serial port.

**[../tools/bringup.py](../tools/bringup.py)** is for a unit you have never
powered up. It works through five stages and each one tells you what a failure
means.

```sh
python3 tools/bringup.py --port /dev/ttyUSB0          # every stage in order
python3 tools/bringup.py --port /dev/ttyUSB0 --stage line
```

| Stage | What it does |
| --- | --- |
| `adapter` | Checks the device node and the FTDI latency timer |
| `quiet` | Listens for 3 s sending nothing. Anything heard is noise, not the sign |
| `line` | Sweeps baud and format sending a broadcast identify, looking for `AK0052.139U1` |
| `address` | Sweeps all 32 addresses to find the unit's own |
| `demo` | Identify, cursor, status, fill, write, read back |

The `line` stage is what makes an unknown unit tractable, because the switch
settings have to be discovered before anything else works.

A successful first run looks like this:

```
[quiet]   silent. Correct: the unit replies, it never announces.
[address] address 1 (GS byte 0x31) answers
[demo]    version : AK0052.139U1
          cursor  : row 0, col 0
          writing 'HOLA MUNDO'  -> ACK
          read back: 'HOLA MUNDO' -- MATCHES
```

**[../tools/pit100.py](../tools/pit100.py)** is the driver the other two are
built on. It implements the whole command set and runs against a serial port or
against the emulator.

## Five rules for anything you write

These were all learned the hard way on the real unit.

1. **Start every transaction with `RS`.** A unit left mid-command by an earlier
   session treats the next byte as a parameter. The text that follows then lands
   at a stale write pointer and you get a confusing overflow error.
2. **End every transaction with `RS`.** `ACK` means stored, not displayed. A
   selected unit never repaints, so `RS` is what commits the update.
3. **Drain replies until the line goes quiet.** Reply lengths vary, so a reader
   that counts bytes will leave the rest buffered and fall one reply behind for
   ever.
4. **Wait at least 4 ms for the first reply byte**, plus the time the frame takes
   on the wire. At 300 baud an identify reply takes about 1.8 seconds.
5. **The display answers, it never announces.** Nothing arrives unless you asked
   for it, so do not use incoming traffic to prove the link is alive.

And when composing text: send CP437, and avoid `$` and `%`, because on this
panel they are `ñ` and `Ñ`.

## A permanent controller

[../controller/](../controller/) is an ESP32 that replaces the sign's original
upstream controller. It does the whole job: WiFi, NTP, a configuration page, and
the line formatting the display will never do for itself. Its README has the
wiring.

That is the permanent installation. A USB adapter and the scripts above are
still the right way to bring an unknown unit up, because the line settings have
to be found before anything can be built around them.

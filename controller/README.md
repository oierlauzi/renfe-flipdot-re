# The controller

An ESP32 that replaces the sign's original upstream controller. It takes its
configuration over WiFi, gets the time from NTP, formats all 16 characters and
sends them down the original RS-422 link. Nothing in the display is modified.

The display computes nothing, so everything interesting happens here. The
protocol it speaks is [../docs/3-the-commands.md](../docs/3-the-commands.md).
For driving the same link from a PC instead, which is still the right way to
bring an unknown unit up, see
[../docs/2-talking-to-it.md](../docs/2-talking-to-it.md).

An ESP32 earns its place on one detail: the link is 7 data bits with even
parity, and it has three hardware UARTs that all support parity. That is exactly
what rules out a Raspberry Pi's mini-UART and an Arduino's `SoftwareSerial`.

## Wiring

| ESP32 | | Display |
| --- | --- | --- |
| `GPIO17` (TX) | to MAX3490 `DI` | |
| `GPIO16` (RX) | from MAX3490 `RO` | |
| | MAX3490 `Y`/`Z` | **green** / **yellow**, the pair the display receives on |
| | MAX3490 `A`/`B` | **blue** / **red**, the pair it replies on |
| | | grey overall shield to chassis, one end only |

**MAX3490, not MAX490.** The MAX490 is a 5 V part. It will usually accept 3.3 V
as a logic high, but its receiver output swings to 5 V straight into a 3.3 V
pin.

Both GPIOs are `idf.py menuconfig` options. The defaults avoid the strapping
pins, but **GPIO16 and 17 are wired to PSRAM on WROVER modules**, so use 32 and
33 there.

If the link comes up silent, swap one pair before suspecting a fault. Reversed
polarity is the commonest wiring error here, it looks exactly like silence, and
it cannot damage anything. If you see rubbish between valid replies, bias the
receive pair with about 680 Ω from `A` to 3.3 V and `B` to ground.

Keep UART0 for the USB console, and give the board a supply that tolerates WiFi
transmit bursts.

**Never drive the display from the ESP32 and a USB adapter at the same time.**
Two drivers on one pair is a contention fault. Unplug one.

### The reset button

A momentary push-button between **GPIO23** and ground. Nothing else: the pin
uses the internal pull-up, so the button just shorts it low. Both the pin and
the hold time are `menuconfig` options, and setting the GPIO to `-1` disables
the feature.

**Hold it for 5 seconds** and the board erases all stored configuration, shows
`CONFIG BORRADA` on the panel, and reboots onto its own access point. Releasing
early does nothing.

Avoid GPIO34 to 39 for this. They are input-only and have no internal pull-up at
all, so the pin floats and reads as a button held down, and the board would
erase itself on every boot. The firmware warns about this at startup if you
configure one anyway.

### Building your own isolation

An isolated transceiver module is easier, but the original board's approach
works from an ESP32 too. It drives a 6N139 through 560 Ω from 5 V, about
6.5 mA. The same resistor from 3.3 V gives only 3.5 mA, so scale it: 220 Ω gives
8.9 mA and 330 Ω gives 5.9 mA, both well inside an ESP32 pin's continuous
rating.

## Building

Needs ESP-IDF v5.5. Activate it per shell, then:

```sh
. ~/esp/esp-idf/export.sh
idf.py set-target esp32          # first time only
idf.py build
```

`CMakeLists.txt` also accepts `-DIDF_PATH=...` for configuring the tree without
touching the shell.

The first build fetches `espressif/mdns` from Espressif's registry, so it needs
network access. Later builds do not. Dropping that dependency costs only the
`flipdot.local` name.

Flashing with both a dev board and a USB-RS-422 adapter plugged in needs the
stable device name, because `/dev/ttyUSB0` depends on plug order:

```sh
ls -l /dev/serial/by-id/
idf.py -p /dev/serial/by-id/usb-Silicon_Labs_CP2102_..._-if00-port0 flash monitor
```

## Testing without hardware

```sh
make controller-check            # from the repository root
```

This compiles the CP437 encoder with the host compiler and checks it against
`../tools/pit100.py` over the samples in `test/samples.txt`. The two are
independent implementations of code that has actually driven the panel, and a
typo in either would stay invisible until someone sent an `Ñ`. Nothing generated
is committed.

`test/test_stopwatch.c` checks the minute rounding at exact boundaries, one
millisecond before a number changes and the one it changes on, which cannot be
tested on the board without waiting for them.

## Configuration

Connect to `http://flipdot.local/`, or to the IP the console prints. With no
credentials stored, the board serves the same page from its own open access
point, `FLIPDOT-XXXX` at `192.168.4.1`.

**It will not fall back to that access point by itself.** Once credentials are
stored the board retries for ever, because giving up cannot tell a mistyped
passphrase from a router rebooting, and answering the second case by leaving the
network is exactly wrong. The internal clock keeps running through an outage, so
the sign just reconnects when the router returns. The cost is that a wrong
passphrase has no software escape, which is what the reset button is for.

Plain HTML forms, no JavaScript, and **the page is in Spanish**, because it is
the interface to a Spanish sign used by whoever is standing in front of it. The
code, its comments and the serial console stay in English. The status page shows
the exact 16 characters that will be sent, under a column ruler, so a truncation
is visible before it reaches the panel:

```
     0123456789012345
    "LUN 13 SEP 18h24"  -> min
```

### Modes

| On the page | | Line |
| --- | --- | --- |
| *Mensaje* | message | Free text, truncated at 16 |
| *Fecha y hora* | date and time | `LUN 13 SEP 18h24` |
| *Tiempo hasta* | time until | Label, then minutes to the next `HH:MM` |
| *Tiempo desde* | time since | Label, then minutes since the last `HH:MM` |
| *Cuenta atrás* | countdown | Label, then minutes left. Start, pause, reset |
| *Cronómetro* | count-up | Label, then minutes elapsed. Start, pause, reset |
| *Lista* | playlist | One line at a time, 30 s minimum each |
| *En blanco* | blank | Nothing |

The clock template is editable, with presets in a dropdown. There is no seconds
directive, on purpose.

**`|` is an elastic gap.** It expands to fill the 16 columns and holds the field
after it against the right edge, where the `min` sticker on the sign continues
it. `ATOCHA|7` gives `ATOCHA         7`, which reads as `7 min` on the assembled
sign, exactly how it worked in service.

**Until and since** run off the wall clock and need NTP. **Countdown and
count-up** are stopwatches on the monotonic clock, so they work before NTP has
synced and are undisturbed when it does, or when the clocks change in October.
Both show whole minutes, and the rounding differs on purpose: a count-up rounds
down, so it reads `0` until a full minute has passed, and a countdown rounds up,
so a fresh 10-minute timer reads `10` and `0 min` always means finished. Neither
survives a reboot, because the monotonic clock restarts with no way to know how
long the board was off.

### The wear rule

Every dot is a coil throwing a physical disc, and the panel was built to repaint
when a train's ETA changed. That is about once a minute, differentially.

So: **no scrolling and no alternation, in any mode.** A marquee flips most of
the 560 dots on every step. The playlist dwell is floored at 30 s, a global
settle time bounds the repaint rate whatever a mode asks for, and a frame
identical to what is already displayed is never sent at all.

The *Prueba de puntos* buttons are the deliberate exception. They exist to move
every dot and find stuck ones: a button pressed on purpose, never a mode left
running.

## The code

| File | |
| --- | --- |
| `main/pit100.c` | The command protocol, a port of `../tools/pit100.py` |
| `main/panel.c` | The task that owns the UART: one writer, coalescing, settle |
| `main/cp437.c` | UTF-8 to CP437 to 7-bit, and the line layout |
| `main/modes.c` | The mode table and the scheduler |
| `main/mode_*.c` | The modes themselves |
| `main/stopwatch.c` | Start, pause, reset and the minute rounding. No ESP-IDF in it |
| `main/timesrc.c` | Timezone, SNTP, internal clock |
| `main/netcfg.c` | WiFi station, the provisioning access point, mDNS |
| `main/button.c` | The reset button |
| `main/webui.c` | The HTTP server, form parsing and pages |
| `main/config.c` | Stored settings, with defaults |

Five things the protocol port has to keep, each learned on hardware and
commented where it happens:

* **`RS` first**, then flush. A unit left mid-command eats the next byte as a
  parameter and the text lands at a stale pointer.
* **`RS` last.** A selected unit never repaints. `ACK` means stored, not
  displayed.
* **Drain replies until the line goes quiet**, never a fixed count.
* **`$` and `%` are `ñ` and `Ñ`** on this panel, and are refused.
* **Settle after a repaint.** The dots are mechanical.

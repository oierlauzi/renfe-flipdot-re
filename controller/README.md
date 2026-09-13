# A permanent controller

An ESP32 that replaces the original upstream controller outright: it accepts
configuration over WiFi, gets the time from NTP, formats all 16 characters and
sends them down the original RS-422 link. No modification to the display.

The display computes nothing — no clock, no countdown, no arithmetic on message
content — so everything interesting happens here. See
[../docs/serial-protocol.md](../docs/serial-protocol.md) for the protocol this
implements, and [../docs/controller.md](../docs/controller.md) for driving the
same link from a PC instead, which is still the better way to bring an unknown
unit up.

The ESP32 earns its place on one detail: the link is 7 data bits with even
parity, and it has three hardware UARTs that all support parity. That is exactly
what rules out a Raspberry Pi's mini-UART and an Arduino's `SoftwareSerial`.

## Hardware

| ESP32 | | Display |
| --- | --- | --- |
| `GPIO17` (TX) | → MAX3490 `DI` | |
| `GPIO16` (RX) | ← MAX3490 `RO` | |
| | MAX3490 `Y`/`Z` (driver out) | **green** / **yellow** — the display receives |
| | MAX3490 `A`/`B` (receiver in) | **blue** / **red** — the display replies |
| | | grey overall shield → chassis, one end only |

**MAX3490, not MAX490.** The MAX490 is a 5 V part: it will usually accept 3.3 V
as a logic high, but its receiver output swings to 5 V straight into a 3.3 V pin.

Both GPIOs are `idf.py menuconfig` options under *Flip-dot controller*. The
defaults avoid the strapping pins (GPIO0, 2, 12, 15), but **GPIO16/17 are wired
to PSRAM on WROVER modules** — use 32/33 there.

If the link comes up silent, swap one pair before suspecting a fault: reversed
polarity is the commonest RS-422 wiring error, presents as exact silence, and
cannot damage anything. If you see garbage between valid replies, bias the
receive pair — roughly 680 Ω from `A` to 3.3 V and `B` to ground.

Keep UART0 for the USB console, and give the board a supply that tolerates WiFi
transmit bursts.

### The reset button

A momentary push-button between **GPIO23** and **ground**. Nothing else: the pin
is configured as an input with the ESP32's internal pull-up, so the button just
shorts it low. Both the pin and the hold time are `menuconfig` options, and
setting the GPIO to `-1` disables the feature.

**Hold it for 5 seconds** and the board erases all stored configuration, shows
`CONFIG BORRADA` on the panel, and reboots onto its own access point. Releasing
early does nothing. Half way through, the console says it has started counting.

Avoid GPIO34–39 for this. They are input-only and have **no internal pull-up at
all**, so the pin floats and reads as a button held down — the board would erase
itself on every boot. The firmware warns about this at startup if you configure
one anyway.

### If you build your own isolation

An isolated transceiver module is easier, but the original board's approach works
from an ESP32 too. It drives a 6N139 through 560 Ω from 5 V, about 6.5 mA. The
same resistor from 3.3 V gives only 3.5 mA, so scale it:

| Resistor | from 3.3 V | from 5 V |
| --- | --- | --- |
| 180 Ω | 10.8 mA | 20.3 mA |
| **220 Ω** | **8.9 mA** | 16.6 mA |
| **330 Ω** | **5.9 mA** | 11.1 mA |
| 560 Ω | 3.5 mA | 6.5 mA, the original |

220–330 Ω reproduces the original drive current and stays well inside the roughly
12 mA per-pin continuous figure for an ESP32 GPIO.

## Building

The toolchain is ESP-IDF v5.5 at `/home/oier/esp/esp-idf`. It is not on `PATH`
by default, so activate it per shell:

```sh
. /home/oier/esp/esp-idf/export.sh
idf.py set-target esp32          # first time only
idf.py build
```

`CMakeLists.txt` also accepts `-DIDF_PATH=...` for configuring the tree without
touching the shell, though `idf.py` itself still needs the toolchain on `PATH`.

The first build fetches `espressif/mdns` from Espressif's component registry, so
it needs network access; later builds do not. Removing the dependency from
`main/idf_component.yml` costs only the `flipdot.local` name.

Flashing, with both a dev board and the USB-RS-422 adapter plugged in, needs the
stable device name — `/dev/ttyUSB0` depends on plug order:

```sh
ls -l /dev/serial/by-id/
idf.py -p /dev/serial/by-id/usb-Silicon_Labs_CP2102_..._-if00-port0 flash monitor
```

**Never drive the display from the ESP32 and the USB adapter at once.** Two
drivers on one RS-422 pair is a contention fault. Unplug one.

## Testing without hardware

```sh
make -C test            # or: make controller-check, from the repo root
```

This compiles `main/cp437.c` with the host compiler and checks it against
`../tools/pit100.py` over the samples in `test/samples.txt` — the CP437 table and
the accent fold are a second implementation of code that has actually driven the
panel, and a typo in either would be invisible until someone sent an `Ñ`. Nothing
generated is committed; the expectations are recomputed from the Python each run.

The layout rules and the 7-bit rules are asserted in `test/test_cp437.c` instead,
because that is where this encoder deliberately differs from the Python.

## Configuration

Connect to `http://flipdot.local/`, or the IP the console prints. With no
credentials stored, the board serves the same page from its own open access
point, `FLIPDOT-XXXX` at `192.168.4.1`.

**It will not fall back there by itself.** Once credentials are stored the board
retries for ever and never gives up on the network, because giving up
automatically cannot distinguish a mistyped passphrase from a router rebooting —
and answering the second case by going off the network is exactly wrong. The
internal RTC keeps running through an outage, so the clock stays right and the
sign simply reconnects when the router returns.

The cost of that is that a wrong passphrase has no software escape. That is what
the reset button is for.

Plain HTML forms, no JavaScript. The status page shows the exact 16 characters
that will be sent, under a column ruler, so a truncation is visible before it
reaches the panel:

```
     0123456789012345
    "LUN 13 SEP 18h24"  → min
```

### Modes

| Mode | Line | Repaints |
| --- | --- | --- |
| Custom message | free text, truncated at 16 | only when you change it |
| Date and time | `LUN 13 SEP 18h24` | on the minute |
| Countdown | label + minutes to an `HH:MM` | on the minute |
| Count-up | label + minutes since an `HH:MM` | on the minute |
| Playlist | one line at a time | on dwell expiry, 30 s minimum |
| Blank | nothing | never |

The clock's template is editable, with presets in a dropdown:
`%a %A %b %B %d %e %m %Y %y %H %I %M`, and `|` for the gap. There is no seconds
directive, on purpose.

**`|` is an elastic gap** that expands to fill the 16 columns, and the field
after it is held against the right edge — where the `min` vinyl sticker on the
sign continues it. `ATOCHA|7` gives `ATOCHA         7`, which reads as
`7 min` on the assembled sign — exactly how it worked in service.

### The wear rule

Every dot is an electromagnetic coil throwing a physical disc, and the panel was
built to repaint when a train's ETA changed — about once a minute, and
differentially, so only the characters that actually changed move.

So: **no scrolling and no alternation, in any mode.** A marquee flips most of the
560 dots on every step. The playlist dwell is floored at 30 s, `panel.settle`
(default 3 s) bounds the repaint rate globally in the panel task whatever a mode
asks for, and a frame identical to what is displayed is never sent at all.

The fill/clear/cycle buttons are the deliberate exception — they exist to move
every dot and find stuck ones. A button pressed on purpose, never a mode left
running.

## What the code is

| File | |
| --- | --- |
| `main/pit100.c` | the command protocol, a port of `../tools/pit100.py` |
| `main/panel.c` | the task that owns the UART: one writer, coalescing, settle, statistics |
| `main/cp437.c` | UTF-8 → CP437 → 7-bit, and `layout()` |
| `main/modes.c` | the mode table and the scheduler |
| `main/mode_*.c` | one file per mode |
| `main/timesrc.c` | timezone, SNTP, internal RTC, and the seam for a DS3231 |
| `main/netcfg.c` | WiFi station, the provisioning AP, mDNS |
| `main/button.c` | the reset button: hold to erase the configuration |
| `main/webui.c` | the HTTP server, form parsing and pages |
| `main/config.c` | NVS, with defaults |

Five things the protocol port must keep, each learned on hardware and commented
where it happens:

* **`RS` first**, then flush. A unit left mid-command eats the next byte as a
  parameter, so `STX` never resets the write pointer and the text lands stale —
  which shows up as a spurious overflow `NAK`.
* **`RS` last.** The repaint task spins at ROM `0x130B` while the unit is
  selected, so an ACKed message stays invisible until `RS`. *ACK means stored,
  not displayed.*
* **Drain replies to an idle gap, never a fixed count.** Reply lengths vary.
* **`$` and `%` are `ñ` and `Ñ`** on this panel, and are refused.
* **Settle after a repaint.** The dots are mechanical.

## Adding the real-time feed

The mode table is the seam. A `mode_realtime.c` needs one `panel_mode_t`, one
line in `MODES[]` in `modes.c`, one line in `main/CMakeLists.txt`, and a task
holding an `esp_websocket_client` to `wss://info.adif.es/InfoStation` — reusing
the `JoinInfo` / `GetLastMessage` / `ReceiveMessage` sequence and the
departure→panel mapping already written in
[../web-replica/realtime.js](../web-replica/realtime.js). Its `render()` formats
the newest departure with the same `mode_counter_line()` the two counter modes
use, because a destination and an ETA is the same shape as a label and a number.

The real cost is TLS: `esp_crt_bundle` plus the SignalR framing. The partition
table already leaves 2 MB for the app so that arriving does not force a
repartition — the firmware is currently about 900 KB, which would not have left
room in the default 1 MB.

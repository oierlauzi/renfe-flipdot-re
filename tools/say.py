#!/usr/bin/env python3
"""Put text on the panel. A minimal front end for pit100.py.

    tools/say.py                        # type lines; each one appears on the panel
    tools/say.py HOLA MUNDO             # send one line and exit
    echo HOLA | tools/say.py            # or from a pipe
    tools/say.py --fill                 # every dot on
    tools/say.py --clear                # every dot off
    tools/say.py --cycle 3              # fill/clear three times: exercises every dot
    tools/say.py --identify             # version and config, as a health check

Defaults are the configuration this unit was found to be running on 2026-09-06:
/dev/ttyUSB0, 1200 7E1, address 1. Override with --port/--baud/--bits/--parity
/--address, or use --sim to drive the emulator instead.

Two habits this script has, both learned the hard way on hardware:

* **It deselects first.** A unit left mid-command by an earlier session eats the
  next byte as a parameter, and the text then lands at a stale write pointer --
  which shows up as a spurious "buffer overflow" NAK. `RS` first costs nothing.
* **It deselects last.** The repaint task is held off while a unit is selected,
  so text that is written and ACKed stays invisible until `RS` releases it.
"""

import argparse
import sys

from pit100 import PIT100, SerialTransport, SimTransport, BUFFER_COLS, PANEL_CHARS

DEFAULTS = dict(port="/dev/ttyUSB0", baud=1200, bits=7, parity="E", address=1)


def open_panel(args):
    """A connected, selected PIT100. Deselects first so the state is known."""
    if args.sim:
        dev = SimTransport(address=args.address)
    else:
        dev = SerialTransport(args.port, args.baud, args.bits, args.parity)
    d = PIT100(dev, args.address)
    d.deselect()                       # RS: drop whatever the last session left
    if hasattr(dev, "flush_input"):
        dev.flush_input()
    d.select()
    return d, dev


def send(d, text, seven_bit, quiet=False):
    """One line onto the panel. Returns True on ACK."""
    if len(text) > BUFFER_COLS:
        print(f"  {len(text)} characters; the buffer holds {BUFFER_COLS}. Not sent.",
              file=sys.stderr)
        return False
    try:
        d.clear()
        ok, err = d.message(text, fold=True, seven_bit=seven_bit)
    except ValueError as e:                       # un-encodable character
        print(f"  {e}", file=sys.stderr)
        return False
    d.deselect()                                  # release the repaint interlock
    d.select()                                    # ...and take it back for the next
    if not quiet:
        if ok:
            extra = ("" if len(text) <= PANEL_CHARS else
                     f"  (only the first {PANEL_CHARS} are on the flip-dot line)")
            print(f"  ok{extra}")
        else:
            print(f"  NAK: {err}", file=sys.stderr)
    return bool(ok)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("words", nargs="*", help="text to display; omit to read stdin")
    ap.add_argument("--fill", action="store_true", help="set every dot")
    ap.add_argument("--clear", action="store_true", help="clear every dot")
    ap.add_argument("--cycle", type=int, metavar="N",
                    help="fill and clear N times, to exercise every dot")
    ap.add_argument("--identify", action="store_true")
    ap.add_argument("--sim", action="store_true", help="drive the emulator")
    ap.add_argument("--port", default=DEFAULTS["port"])
    ap.add_argument("--baud", type=int, default=DEFAULTS["baud"])
    ap.add_argument("--bits", type=int, choices=(7, 8), default=DEFAULTS["bits"])
    ap.add_argument("--parity", choices=("E", "O", "N"), default=DEFAULTS["parity"])
    ap.add_argument("--address", type=lambda s: int(s, 0), default=DEFAULTS["address"])
    ap.add_argument("--settle", type=float, default=3.0,
                    help="seconds to let the dots finish moving (default 3)")
    args = ap.parse_args()

    d, dev = open_panel(args)
    try:
        if args.identify:
            info = d.identify()
            print(f"version : {info['version']}")
            print(f"config  : {info['config'].hex(' ')}")
            print("cursor  : row %d, col %d" % d.cursor())
            return 0

        if args.fill:
            d.fill_test_pattern()
            d.deselect()
            dev.settle(args.settle)
            return 0

        if args.clear:
            d.clear()
            d.deselect()
            dev.settle(args.settle)
            return 0

        if args.cycle:
            for n in range(args.cycle):
                print(f"  {n + 1}/{args.cycle} fill", flush=True)
                d.fill_test_pattern(); d.deselect(); dev.settle(args.settle)
                d.select()
                print(f"  {n + 1}/{args.cycle} clear", flush=True)
                d.clear(); d.deselect(); dev.settle(args.settle)
                d.select()
            return 0

        # 7-bit links strip bit 7, so map n-tilde onto its 0x24/0x25 duplicate.
        seven_bit = args.bits == 7

        if args.words:
            return 0 if send(d, " ".join(args.words), seven_bit) else 1

        interactive = sys.stdin.isatty()
        if interactive:
            print(f"Type a line to display it. Ctrl-D to quit. "
                  f"{PANEL_CHARS} characters fit the flip-dot line.")
        while True:
            try:
                line = input("panel> " if interactive else "")
            except (EOFError, KeyboardInterrupt):
                print()
                return 0
            if line.strip() or line == "":
                send(d, line, seven_bit, quiet=not interactive)
    finally:
        d.deselect()                    # never leave the unit selected


if __name__ == "__main__":
    sys.exit(main())

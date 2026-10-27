#!/usr/bin/env python3
"""Staged bring-up for a real PI-T100-M over a USB-RS-422 adapter.

Everything above the wire is already decoded and validated in emulation, so the
only unknowns on a first power-up are physical: is the wiring right, and which
line settings did the DIP switches select. This walks those out in order, and
each stage prints what a failure means rather than just failing.

    python3 tools/bringup.py --port /dev/ttyUSB0          # every stage, in order
    python3 tools/bringup.py --port /dev/ttyUSB0 --stage line
    python3 tools/bringup.py --sim                        # exercise the script itself

`--sim` runs the stages that make sense against tools/pit100_sim.py, so the script
can be debugged before any hardware exists. It cannot exercise the line sweep,
which is about real UART settings.

Wiring for the DSD TECH SH-U11 (docs/2-talking-to-it.md):

    TxD+ -> green      TxD- -> yellow      (the display's receive pair)
    RxD+ -> blue       RxD- -> red         (the display's transmit pair)
    GND  -> cable shield / drain

If nothing answers, swap one pair before suspecting anything else. Reversed
polarity is the commonest RS-485 wiring fault and presents as exact silence.
"""

import argparse
import sys
import time

from pit100 import (PIT100, SimTransport, SerialTransport, BROADCAST, GS, SUB,
                    address_byte)

VERSION_STRING = b"AK0052.139U1"      # ROM 0x0064 -- the known answer


def pyserial():
    """Import pyserial, with an actionable message when it is missing."""
    try:
        import serial
    except ImportError:
        sys.exit("This needs pyserial:  pip install pyserial\n"
                 "(or apt install python3-serial). Use --sim to run the stages\n"
                 "that do not touch a serial port.")
    return serial


def open_port(serial, port, baud, bits, parity, timeout=0.2):
    """A configured port, or None with the reason printed."""
    try:
        return serial.Serial(
            port, baud, timeout=timeout,
            bytesize=serial.EIGHTBITS if bits == 8 else serial.SEVENBITS,
            parity={"E": serial.PARITY_EVEN, "O": serial.PARITY_ODD,
                    "N": serial.PARITY_NONE}[parity],
            stopbits=serial.STOPBITS_ONE)
    except (OSError, serial.SerialException) as e:
        print(f"          cannot open {port}: {e}")
        return None

# Ordered by likelihood, not exhaustively. The first two are what the recorded
# switch positions give under each polarity of the unresolved switch sense; see
# docs/4-inside-the-firmware.md. 9600 8E1 is the hardwired setting used when port C bit 5
# is clear -- but that also selects handler C, which does not speak this protocol,
# so a hit there would be surprising.
LIKELY = [(4800, 8, "N"), (2400, 7, "E"), (9600, 8, "E")]
BAUDS = (9600, 4800, 2400, 1200, 600, 300)
FORMATS = ((8, "E"), (8, "N"), (7, "E"), (8, "O"), (7, "O"), (7, "N"))


def line_settings():
    """Every candidate, likely ones first, without repeats."""
    seen = []
    for cand in LIKELY:
        seen.append(cand)
    for baud in BAUDS:
        for bits, parity in FORMATS:
            if (baud, bits, parity) not in seen:
                seen.append((baud, bits, parity))
    return seen


def frame_seconds(nbytes, baud, bits, parity):
    """Wire time for a reply, plus the firmware's 3.9 ms turnaround."""
    per_byte = 1 + bits + (0 if parity == "N" else 1) + 1
    return 0.0039 + nbytes * per_byte / baud


# --------------------------------------------------------------------- stage 1
def stage_adapter(port):
    """Check the device node, and report the FTDI latency timer."""
    import os
    print(f"[adapter] {port}")
    if not os.path.exists(port):
        print(f"          NOT FOUND. Is the adapter plugged in? Try: ls /dev/ttyUSB*")
        return False
    node = os.path.basename(port)
    lat = f"/sys/bus/usb-serial/devices/{node}/latency_timer"
    try:
        with open(lat) as f:
            ms = int(f.read().strip())
        print(f"          latency_timer = {ms} ms", end="")
        if ms > 2:
            print("  <-- too high")
            print(f"          The display answers after a 3.9 ms turnaround, so a")
            print(f"          {ms} ms read latency dominates every exchange. Fix with:")
            print(f"              echo 1 | sudo tee {lat}")
        else:
            print("  (good)")
    except OSError:
        print("          latency_timer not readable (not an FTDI device, or not Linux)")
    return True


# --------------------------------------------------------------------- stage 2
def stage_quiet(port, baud, bits, parity, seconds=3.0):
    """Listen with nothing sent. The firmware never transmits unprompted."""
    serial = pyserial()
    print(f"[quiet]   listening {seconds:.0f}s at {baud} {bits}{parity}1, sending nothing")
    s = open_port(serial, port, baud, bits, parity)
    if s is None:
        return False
    with s:
        s.reset_input_buffer()
        seen = bytearray()
        deadline = time.time() + seconds
        while time.time() < deadline:
            seen += s.read(256)
    if not seen:
        print("          silent. Correct -- the unit replies, it never announces.")
        return True
    print(f"          {len(seen)} unexpected bytes: {bytes(seen[:32]).hex(' ')}"
          + (" ..." if len(seen) > 32 else ""))
    print("          The firmware transmits nothing unprompted, so this is not the")
    print("          display talking. Most likely your receive pair is floating:")
    print("          the display tri-states its driver between replies, and an")
    print("          unbiased RS-422 input then delivers noise to the UART.")
    print("          Add fail-safe bias -- about 680R from RxD+ to VCC and RxD- to")
    print("          ground -- or ignore it and rely on the known-answer test.")
    return False


# --------------------------------------------------------------------- stage 3
def probe_once(port, baud, bits, parity, verbose=False):
    """Broadcast SUB at one line setting. True if the version string comes back."""
    serial = pyserial()
    s = open_port(serial, port, baud, bits, parity)
    if s is None:
        return False, b""
    raw = bytearray()
    try:
        with s:
            s.reset_input_buffer()
            s.write(bytes((GS, BROADCAST, SUB)))
            s.flush()
            budget = frame_seconds(48, baud, bits, parity) * 3 + 0.2
            deadline = time.time() + budget
            while time.time() < deadline:
                chunk = s.read(64)
                if chunk:
                    raw += chunk
                    if VERSION_STRING in bytes(raw):
                        break
    except (OSError, serial.SerialException) as e:
        print(f"          serial error: {e}")
        return False, b""
    if verbose and raw:
        print(f"          raw: {bytes(raw[:48]).hex(' ')}")
    return VERSION_STRING in bytes(raw), bytes(raw)


def stage_line(port, verbose=False):
    """Sweep line settings until the unit identifies itself."""
    print("[line]    sweeping baud x format, broadcast SUB, looking for "
          f"{VERSION_STRING.decode()!r}")
    for baud, bits, parity in line_settings():
        print(f"          {baud:>5} {bits}{parity}1 ... ", end="", flush=True)
        hit, raw = probe_once(port, baud, bits, parity, verbose)
        if hit:
            print("MATCH")
            print(f"\n          The unit is at {baud} baud, {bits} data bits, "
                  f"parity {parity}, 1 stop.")
            return (baud, bits, parity)
        print(f"{'garbage' if raw else 'silence'}")
    print("\n          Nothing answered at any setting. Three explanations, in the")
    print("          order worth checking:")
    print("          1. Pair polarity. Swap TxD+/TxD-, then RxD+/RxD-. Harmless,")
    print("             and it is the commonest RS-485 wiring fault.")
    print("          2. The unit is running a different protocol. DIP switch port")
    print("             C bit 5 clear routes reception to handler C, a binary")
    print("             protocol this script does not speak; port A bit 6 clear")
    print("             selects handler A, which is not the command set either.")
    print("             See docs/4-inside-the-firmware.md.")
    print("          3. No common-mode reference. The cable has no signal ground,")
    print("             so tie the adapter's GND to the cable shield.")
    return None


# --------------------------------------------------------------------- stage 4
def stage_address(dev, verbose=False):
    """Find the unit's own address by trying each in turn without broadcasting.

    **Deselect before every attempt.** A selected unit answers everything, so
    without the RS the first address tried appears to work -- and if a broadcast
    ran earlier, every address does. That bug reported address 15 on real
    hardware when the true address was 1.
    """
    print("[address] sweeping GS 0-31, deselecting before each")
    for addr in range(32):
        d = PIT100(dev, addr)
        d.deselect()                  # RS: drop any existing selection
        if hasattr(dev, "flush_input"):
            dev.flush_input()
        d.select()
        try:
            info = d.identify()
        except Exception:
            info = None
        d.deselect()
        if info and info["version"].startswith("AK0052"):
            print(f"          address {addr} (GS byte {address_byte(addr):#04x}) answers")
            return addr
        if verbose:
            print(f"          {addr:>2}: no")
    print("          No address answered, though broadcast did. Odd -- check")
    print("          whether ROM 0x007A (cfg_addr_mode) makes this unit unaddressed.")
    return None


# --------------------------------------------------------------------- stage 5
def stage_demo(dev, address, text="HOLA MUNDO"):
    """Prove the display end to end: identify, clear, fill, write, read back."""
    d = PIT100(dev, address)
    d.select()

    info = d.identify()
    print(f"[demo]    version : {info['version']}")
    print(f"          config  : {info['config'].hex(' ')}")
    print("          cursor  : row %d, col %d" % d.cursor())
    try:
        print(f"          status  : {d.status():#04x}")
    except Exception as e:
        print(f"          status  : unavailable ({e})")
    d.deselect()

    print("          filling with the solid block (every dot should move)")
    d.select(); d.fill_test_pattern(); d.deselect()
    dev.settle(3.0)

    print(f"          writing {text!r}")
    ok, err = d.show(text)
    print("          ->", "ACK" if ok else f"NAK ({err})" if ok is False else f"({err})")
    dev.settle(4.0)          # 64 characters have to be repainted back to spaces

    # SI reads the shadow buffer *from the cursor*, and show() left it past the
    # end of the message -- so home the cursor first or you read the blank tail.
    d.select()
    d.goto(0, 0)
    shown = d.read_screen(len(text))
    d.deselect()
    print(f"          read back: {shown.decode('cp437', 'replace')!r}")
    if shown.startswith(text.encode("cp437")):
        print("          MATCHES -- the panel is showing what was sent.")
        return True
    print("          Does not match. The message was stored but the panel may not")
    print("          have repainted; RS is what releases the repaint interlock.")
    return False


# ---------------------------------------------------------------------- driver
STAGES = ("adapter", "quiet", "line", "address", "demo")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="serial device, e.g. /dev/ttyUSB0")
    ap.add_argument("--sim", action="store_true",
                    help="run against the emulator, to exercise this script")
    ap.add_argument("--stage", choices=STAGES, help="run one stage only")
    ap.add_argument("--baud", type=int, help="skip the sweep and use this rate")
    ap.add_argument("--bits", type=int, choices=(7, 8), default=8)
    ap.add_argument("--parity", choices=("E", "O", "N"), default="E")
    ap.add_argument("--address", type=lambda s: int(s, 0),
                    help="skip the address sweep")
    ap.add_argument("--text", default="HOLA MUNDO")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    if not args.sim and not args.port:
        sys.exit("need --port /dev/ttyUSB0, or --sim to exercise the script")

    want = (args.stage,) if args.stage else STAGES

    if args.sim:
        skipped = [s for s in want if s in ("adapter", "quiet", "line")]
        if skipped:
            print(f"[sim]     skipping {', '.join(skipped)} -- "
                  "these are about a real UART")
        dev = SimTransport(address=args.address if args.address is not None else 5)
        address = args.address if args.address is not None else 5
        if "address" in want:
            found = stage_address(dev, args.verbose)
            address = found if found is not None else address
        if "demo" in want:
            return 0 if stage_demo(dev, address, args.text) else 1
        return 0

    if "adapter" in want and not stage_adapter(args.port):
        return 1

    baud, bits, parity = args.baud, args.bits, args.parity
    if "quiet" in want:
        stage_quiet(args.port, baud or 9600, bits, parity)

    if "line" in want and not args.baud:
        found = stage_line(args.port, args.verbose)
        if not found:
            return 1
        baud, bits, parity = found
    if baud is None:
        baud = 9600

    dev = SerialTransport(args.port, baud, bits, parity)
    address = args.address
    if "address" in want and address is None:
        address = stage_address(dev, args.verbose)
    if "demo" in want:
        return 0 if stage_demo(dev, address, args.text) else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

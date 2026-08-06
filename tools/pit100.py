#!/usr/bin/env python3
"""Host-side driver for the PAGE Iberica / Telenorma PI-T100-M flip-dot display.

Implements the command protocol documented in docs/serial-protocol.md. Everything
here is exercised against the emulator in tools/pit100_sim.py, so it can be
developed and tested with no hardware attached; swap the transport for a serial
port when the physical layer is known.

    from pit100 import PIT100, SimTransport
    d = PIT100(SimTransport(address=5), address=5)
    d.select()
    print(d.identify())
    d.message("HOLA")

Two things a caller must know, both from docs/:

* Text is **CP437**, not Latin-1. `Ñ` is 0xA5. See encode().
* The display **transmits a status frame continuously**, unprompted, forever. Any
  reply reader has to skip those or it will mistake them for responses. The
  transports below do that.
"""

import argparse
import sys
import time

# ---------------------------------------------------------------- command bytes
NUL, SOH, STX, ETX, EOT, ENQ, ACK, BEL = range(8)
BS, TAB, LF, VT, FF, CR, SO, SI = range(8, 16)
DLE, DC1, DC2, DC3, DC4, NAK, SYN, ETB = range(16, 24)
CAN, EM, SUB, ESC, FS, GS, RS, US = range(24, 32)

BROADCAST = 0x7F                 # GS parameter that every unit answers to

STATUS_FRAME_PREFIX = bytes((CAN, CAN, RS, NAK))   # 18 18 1E 15, then <addr>
STATUS_FRAME_LEN = 5

BUFFER_COLS = 64                 # docs/serial-protocol.md: 0xE001-0xE040
PANEL_CHARS = 16

# Error codes returned after a NAK. 0x30 and 0x32 arrive bare; anything else is
# followed by two detail bytes.
ERRORS = {0x30: "buffer overflow", 0x32: "unspecified failure"}
BARE_ERRORS = (0x30, 0x32)


class ProtocolError(Exception):
    pass


# CP437 carries lowercase accented vowels but almost no uppercase ones: only
# E-acute, A/O/U-umlaut and N-tilde exist. Station names are usually uppercase, so
# this bites immediately. Map the missing ones to their unaccented forms.
FOLD = str.maketrans({"Á": "A", "Í": "I", "Ó": "O", "Ú": "U", "À": "A",
                      "È": "E", "Ì": "I", "Ò": "O", "Ù": "U", "Â": "A",
                      "Ê": "E", "Î": "I", "Ô": "O", "Û": "U", "Ï": "I"})


def encode(text, fold=False):
    """Encode text for the panel: CP437, with the panel's quirks guarded.

    Two things differ from plain cp437:

    * ROM 0x24 and 0x25 hold accented capitals, not '$' and '%'. Sending either
      produces the wrong glyph, so refuse rather than surprise the caller.
    * CP437 has no uppercase A/I/O/U-acute. `Ñ` and `É` are fine, the rest are
      not, which matters because station names are usually uppercase. Pass
      fold=True to strip those accents instead of raising.

    See docs/font.md.
    """
    if fold:
        text = text.translate(FOLD)
    for bad in "$%":
        if bad in text:
            raise ValueError(
                f"{bad!r} has no glyph on this panel - ROM 0x24/0x25 hold "
                f"accented capitals instead. See docs/font.md")
    try:
        return text.encode("cp437")
    except UnicodeEncodeError as exc:
        ch = exc.object[exc.start:exc.end]
        hint = ""
        if any(ord(c) in FOLD for c in ch):
            hint = (" - CP437 has no uppercase accented vowels except E-acute; "
                    "pass fold=True to drop the accent")
        raise ValueError(f"{ch!r} is not in CP437{hint}") from None


def address_byte(address):
    """Encode a display address the way the firmware does at 0x0F69 / 0x1078.

    Bits 4:0 of the DIP switches become printable ASCII: 0x30-0x3F when bit 4 is
    clear, 0x40-0x4F when set. 0x7F is the broadcast address.
    """
    if address is None or address == BROADCAST:
        return BROADCAST
    if not 0 <= address <= 0x1F:
        raise ValueError("address must be 0-31, or None to broadcast")
    return (0x40 | (address & 0x0F)) if address & 0x10 else (0x30 | address)


class PIT100:
    """The command protocol. Transport supplies write() and read_reply()."""

    def __init__(self, transport, address=None):
        self.t = transport
        self.address = address

    # ------------------------------------------------------------- addressing
    def select(self):
        """GS <addr>. Until this is sent the unit accepts only EOT, VT and GS."""
        self.t.write(bytes((GS, address_byte(self.address))))

    def deselect(self):
        """RS. Ignored by a unit whose address is 0.

        **Not optional if you want to see anything.** The repaint task spins at
        0x130B for as long as `selected` (0xA040) is set, so a selected unit
        accepts and ACKs text but never moves a dot. RS releases the interlock
        and the panel updates. See docs/serial-protocol.md.
        """
        self.t.write(bytes((RS,)))

    def reset(self):
        """DLE - jumps the firmware to its fatal/init path. A remote reboot."""
        self.t.write(bytes((DLE,)))

    # ---------------------------------------------------------------- writing
    def message(self, text, expect_reply=True):
        """STX <text> ETX. Returns (ok, error) once the unit answers.

        The ACK means "stored", not "displayed" -- nothing is painted until the
        unit is deselected. Use show(), or call deselect() yourself.
        """
        payload = encode(text)
        if len(payload) > BUFFER_COLS:
            raise ValueError(
                f"{len(payload)} bytes exceeds the {BUFFER_COLS}-byte buffer; "
                f"the unit would NAK with error 0x30")
        self.t.write(bytes((STX,)) + payload + bytes((ETX,)))
        return self.read_ack() if expect_reply else (None, None)

    def goto(self, row, col):
        """VT <row+0x30> <col+0x30>. Both parameters are ASCII digits."""
        if not 0 <= col < BUFFER_COLS:
            raise ValueError(f"column must be 0-{BUFFER_COLS - 1}")
        self.t.write(bytes((VT, 0x30 + row, 0x30 + col)))

    def clear(self):
        """CAN - fill the buffer with spaces and repaint."""
        self.t.write(bytes((CAN,)))

    def fill_test_pattern(self):
        """FF - fill the buffer with '#'. Useful for checking every dot moves."""
        self.t.write(bytes((FF,)))

    def erase_to_end_of_line(self):
        self.t.write(bytes((ETB,)))

    def erase_to_end_of_display(self):
        self.t.write(bytes((EM,)))

    def set_attribute(self, value):
        """BEL <param>. The low 3 bits select one of 8 display effects.

        What each of the eight looks like on a real panel is not yet known -- see
        docs/flipdot-interface.md.
        """
        if not 0 <= value <= 7:
            raise ValueError("attribute is 3 bits: 0-7")
        self.t.write(bytes((BEL, value)))

    # ---------------------------------------------------------------- queries
    def identify(self):
        """SUB - 45 bytes from ROM 0x0064: version string then the config block.

        The best bring-up command: no prior state needed, works under broadcast
        addressing, and the answer is known in advance.
        """
        self.t.write(bytes((SUB,)))
        raw = self.t.read_reply(48)
        raw = raw.lstrip(bytes((SYN,)))          # strip any SYN preamble
        version = raw[:12].decode("ascii", "replace")
        return {"version": version, "config": raw[12:], "raw": raw}

    def cursor(self):
        """TAB - reply is two ASCII digits, row then column."""
        self.t.write(bytes((TAB,)))
        raw = self.t.read_reply(2)
        if len(raw) < 2:
            raise ProtocolError(f"short cursor reply: {raw.hex(' ')}")
        return raw[0] - 0x30, raw[1] - 0x30

    def read_screen(self, length=BUFFER_COLS):
        """SI - read the buffer contents back from the cursor."""
        self.t.write(bytes((SI,)))
        return self.t.read_reply(length)

    # ----------------------------------------------------------------- replies
    def read_ack(self):
        """Parse <ACK|NAK> <addr> [errcode [p1 p2]]. Returns (ok, error-or-None)."""
        raw = self.t.read_reply(5)
        if not raw:
            return None, "no reply"
        if raw[0] == ACK:
            return True, None
        if raw[0] == NAK:
            code = raw[2] if len(raw) > 2 else None
            if code is None:
                return False, "NAK, no error code"
            detail = ERRORS.get(code, f"code 0x{code:02X}")
            if code not in BARE_ERRORS and len(raw) >= 5:
                detail += f" (params {raw[3]:#04x}, {raw[4]:#04x})"
            return False, detail
        raise ProtocolError(f"expected ACK or NAK, got {raw.hex(' ')}")

    # -------------------------------------------------------------- convenience
    def show(self, text):
        """Select, clear, write one line, confirm, then deselect. The everyday
        operation.

        The trailing deselect is what actually makes the dots move: while the
        unit is selected the repaint task is held off, so text written and ACKed
        during a selection stays invisible until RS. Verified in emulation --
        without it the render pass runs zero times.
        """
        self.select()
        self.clear()
        ok, err = self.message(text)
        self.deselect()
        return ok, err


def strip_status_frames(data):
    """Remove the continuously transmitted 18 18 1E 15 <addr> frames.

    Any real controller needs this: the unit talks unprompted, so a naive reader
    sees status bytes interleaved with genuine replies.
    """
    out, i = bytearray(), 0
    while i < len(data):
        if data[i:i + len(STATUS_FRAME_PREFIX)] == STATUS_FRAME_PREFIX:
            i += STATUS_FRAME_LEN
            continue
        out.append(data[i])
        i += 1
    return bytes(out)


class SimTransport:
    """Talks to the emulated firmware, so the protocol can be tested with no
    hardware. Switch settings mirror docs/dip-switches.md defaults."""

    def __init__(self, address=5, port_c=0xFF, xram="board", boot_steps=200_000):
        from pit100_sim import PIT100 as Machine
        port_a = 0x40 | (address & 0x1F)      # bit 6 set -> command protocol
        self.m = Machine(open("original/firmware.bin", "rb").read(),
                         port_a, 0xFF, port_c, xram=xram)
        self.m.run(boot_steps)
        self.m.tx_bytes.clear()

    def write(self, data):
        self.m.feed(data)
        self.m.run(400_000)

    def read_reply(self, _n):
        raw = strip_status_frames(bytes(self.m.tx_bytes))
        self.m.tx_bytes.clear()
        return raw


class SerialTransport:
    """The real thing, once the physical layer is known.

    Defaults reflect the switches as found: see docs/dip-switches.md. The reply
    timeout allows for the firmware's fixed 3.9 ms turnaround plus the frame time,
    which is 183 ms for five bytes at 300 baud.
    """

    def __init__(self, port, baud=1200, bits=7, parity="E", timeout=1.0):
        import serial                                    # pyserial
        self.s = serial.Serial(
            port, baud, timeout=timeout,
            bytesize=serial.SEVENBITS if bits == 7 else serial.EIGHTBITS,
            parity={"E": serial.PARITY_EVEN, "O": serial.PARITY_ODD,
                    "N": serial.PARITY_NONE}[parity],
            stopbits=serial.STOPBITS_ONE)

    def write(self, data):
        self.s.write(data)
        self.s.flush()

    def read_reply(self, n, settle=0.05):
        time.sleep(0.01)                     # the firmware's turnaround delay
        raw = bytearray()
        deadline = time.time() + self.s.timeout
        while time.time() < deadline:
            chunk = self.s.read(max(1, n))
            if chunk:
                raw += chunk
                if len(strip_status_frames(bytes(raw))) >= n:
                    break
            else:
                break
        return strip_status_frames(bytes(raw))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sim", action="store_true",
                    help="drive the emulator instead of a serial port")
    ap.add_argument("--port", help="serial device, e.g. /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=1200)
    ap.add_argument("--bits", type=int, choices=(7, 8), default=7)
    ap.add_argument("--parity", choices=("E", "O", "N"), default="E")
    ap.add_argument("--address", type=lambda s: int(s, 0), default=5,
                    help="display address 0-31, or 127 to broadcast")
    ap.add_argument("--text", help="message to display")
    ap.add_argument("--identify", action="store_true")
    ap.add_argument("--cursor", action="store_true")
    ap.add_argument("--clear", action="store_true")
    args = ap.parse_args()

    if args.sim:
        transport = SimTransport(address=args.address)
    elif args.port:
        transport = SerialTransport(args.port, args.baud, args.bits, args.parity)
    else:
        sys.exit("need --sim or --port")

    d = PIT100(transport, None if args.address == BROADCAST else args.address)
    d.select()

    if args.identify:
        info = d.identify()
        print(f"version : {info['version']}")
        print(f"config  : {info['config'].hex(' ')}")
    if args.cursor:
        print("cursor  : row %d, col %d" % d.cursor())
    if args.clear:
        d.clear()
        print("cleared")
    if args.text:
        ok, err = d.message(args.text)
        print(f"message : {args.text!r} -> " +
              ("ACK" if ok else f"NAK ({err})" if ok is False else f"({err})"))


if __name__ == "__main__":
    main()

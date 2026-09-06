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
* The display **answers, it never announces**. Nothing arrives unless you asked
  for it, so a reader that sees unprompted bytes is looking at line noise, not at
  the display -- see tools/bringup.py.
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


def escape_controls(payload):
    """Wrap every byte below 0x20 in the EOT '3' literal escape.

    Handler B's dispatcher consumes 0x00-0x1F as commands, so a bare control byte
    never reaches the buffer -- it is silently eaten, not stored. `EOT '3' <byte>`
    stores the next byte verbatim whatever it is, which is the only way to display
    CP437's control-code glyphs (0x01-0x1F: smileys, card suits, arrows, note
    symbols). Verified in emulation, including mid-message and for 0x00.

    See docs/serial-protocol.md.
    """
    out = bytearray()
    for b in payload:
        if b < 0x20:
            out += bytes((EOT, 0x33))
        out.append(b)
    return bytes(out)


# The ROM duplicates n-tilde and N-tilde below 0x80, at exactly the codes a 7-bit
# link delivers them to: 0xA4 & 0x7F = 0x24, 0xA5 & 0x7F = 0x25. See docs/font.md.
SEVEN_BIT = {0xA4: 0x24, 0xA5: 0x25}


def encode(text, fold=False, seven_bit=False):
    """Encode text for the panel: CP437, with the panel's quirks guarded.

    Two things differ from plain cp437:

    * ROM 0x24 and 0x25 are 'n' and 'N' with tildes, not '$' and '%'. Sending
      either produces the wrong glyph, so refuse rather than surprise the caller.
    * CP437 has no uppercase A/I/O/U-acute. `Ñ` and `É` are fine, the rest are
      not, which matters because station names are usually uppercase. Pass
      fold=True to strip those accents instead of raising.

    Control codes are *not* escaped here, because the escape only makes sense in
    a command stream -- use escape_controls() for that, or message(), which
    applies it.

    seven_bit=True maps n-tilde and N-tilde to their duplicates at 0x24/0x25.
    Strictly this is belt and braces: a 7-bit link strips bit 7 in hardware and
    lands on the same codes. It matters when you want the bytes on the wire to be
    unambiguous, or when the link turns out to be 8-bit after all and you still
    want the alternate glyphs.

    See docs/font.md.
    """
    if fold:
        text = text.translate(FOLD)
    for bad in "$%":
        if bad in text:
            raise ValueError(
                f"{bad!r} has no glyph on this panel - ROM 0x24/0x25 hold "
                f"n-tilde and N-tilde instead. See docs/font.md")
    try:
        out = text.encode("cp437")
        return bytes(SEVEN_BIT.get(b, b) for b in out) if seven_bit else out
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
        self.t.write(bytes((STX,)) + escape_controls(payload) + bytes((ETX,)))
        return self.read_ack() if expect_reply else (None, None)

    def literal(self, data):
        """Store bytes verbatim, bypassing the command dispatcher.

        The only way to place CP437 glyphs 0x01-0x1F on the panel. Writes at the
        current cursor, so combine with goto().
        """
        self.t.write(escape_controls(bytes(data)))

    def block_write(self, data, offset=0):
        """EOT '2' <offset-hi> <offset-lo> <count-hi> <count-lo> <data...>.

        A general block load into the buffer at 0xE084, which is the **second
        display's** message buffer (its shadow follows at 0xE0C6, the same 0x42
        stride as the primary pair). Offset and count are both 16-bit, so this can
        reach well past the buffer into paged expansion memory. Confirmed in
        emulation for offset 4, count 6.

        **Nothing renders it in this build.** The render path only switches to the
        second pair when 0xA057 is set, and that flag's only writer sits behind an
        always-taken jump -- the feature is patched out. So this writes real bytes
        to real memory that nothing will ever display. Kept because it is a clean
        way to prove a link works end to end without disturbing the panel, and
        because a different ROM build may well enable it.
        See docs/io-map.md.
        """
        n = len(data)
        self.t.write(bytes((EOT, 0x32, offset >> 8, offset & 0xFF,
                            n >> 8, n & 0xFF)) + bytes(data))

    def block_write_default(self, data):
        """EOT '1' <64 bytes>: the fixed form -- always 64 bytes at 0xE084."""
        if len(data) != 64:
            raise ValueError("EOT '1' transfers exactly 64 bytes")
        self.t.write(bytes((EOT, 0x31)) + bytes(data))

    def goto(self, row, col):
        """VT <row+0x30> <col+0x30>. Both parameters are ASCII digits.

        Row is 0-based and each row is 64 bytes: the firmware computes
        0xE001 + row * 0x40 at 0x0CD8, then adds the column.
        """
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
        """BEL <param>. Stores param & 7 in 0xA014, the display attribute.

        On this panel family only **bits 1:0 reach the hardware**: they are driven
        out on port C bits 1:0 as two attribute lines, giving four states, not
        eight. Verified by sweeping 0-7 in emulation. Values 4-7 therefore behave
        identically to 0-3 here.

        Bit 2 is not a third line. Under receive handler A it marks effect 0 as
        permanent, exempting it from the countdown that otherwise clears the bit;
        under handler B (this driver) the ageing logic is skipped entirely, so
        bit 2 has no effect at all.

        What the two lines physically do is unknown. Not brightness (the dots are
        mechanical) and not the fluorescent tube (it is electrically independent
        of the electronics). The leading candidate is that they target the LED
        matrix further along the same ribbon. See docs/flipdot-interface.md.
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
        """TAB - reply is (0x30 + row, 0x30 + column). Returns (row, col).

        **Not two ASCII digits**, though it looks like them for columns 0-9. The
        firmware adds 0x30 to the raw value, so column 10 comes back as ':' and
        column 63 as 'o'. Confirmed on hardware by walking the cursor: 16 -> '@',
        32 -> 'P', 63 -> 'o', and 64 wraps to row 1 column 0.
        """
        self.t.write(bytes((TAB,)))
        raw = self.t.read_reply(2)
        if len(raw) < 2:
            raise ProtocolError(f"short cursor reply: {raw.hex(' ')}")
        return raw[0] - 0x30, raw[1] - 0x30

    def read_screen(self, length=BUFFER_COLS):
        """SI - read back what is currently DISPLAYED, from the cursor onward.

        It reads the *shadow* buffer at cursor+0x42, not the message buffer, so it
        reports the panel's actual contents rather than what was last written.
        Text sent but not yet committed with RS therefore does not appear here --
        which makes this the natural way to confirm an update landed.

        The firmware plants a 0xFF sentinel at 0xE083 to bound the reply, so the
        last shadow byte is clobbered and character 63 will be repainted on the
        next pass. Harmless, but it means SI is not perfectly side-effect free.
        """
        self.t.write(bytes((SI,)))
        return self.t.read_reply(length)

    def status(self):
        """BS - reply is BS then one status byte. Returns the status byte.

        The byte is a sticky accumulator of I/O port 0xDE, an unidentified 8-bit
        hardware status input. The RST7.5 tick samples the port, inverts it (it is
        active low) and ORs it in, so a bit set means "active at some point since
        the last query". Reading **clears** the accumulator, which makes this
        edge-triggered: a fault that came and went is still reported once, and
        polling repeatedly returns 0 while the condition persists.

        With config byte 0x008B non-zero the firmware would report the
        instantaneous port state instead; this build has it at 0.

        What each bit means needs the hardware. See docs/serial-protocol.md.
        """
        self.t.write(bytes((BS,)))
        raw = self.t.read_reply(2)
        if len(raw) < 2 or raw[0] != BS:
            raise ProtocolError(f"unexpected status reply: {raw.hex(' ')}")
        return raw[1]

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
    """Remove any 18 18 1E 15 <addr> status frame from a reply.

    An earlier version of this file said the unit emitted these "continuously,
    unprompted, forever". **It does not** -- the firmware transmits nothing it was
    not asked for, and the frame belongs to handler C. Kept because the frame can
    still appear inside a reply, and stripping it is harmless.
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

    def __init__(self, address=5, port_c=0xFF, xram="board", boot_steps=300_000,
                 port_b=0xFE):
        from pit100_sim import PIT100 as Machine
        port_a = 0x40 | (address & 0x1F)      # bit 6 set -> command protocol
        # port B bit 0 clear: pad with spaces rather than solid blocks, and use
        # the paged render pass. See docs/dip-switches.md.
        self.m = Machine(open("original/firmware.bin", "rb").read(),
                         port_a, port_b, port_c, xram=xram)
        self.m.run(boot_steps, tick=2000)
        self.m.tx_bytes.clear()

    def write(self, data):
        self.m.feed(data)
        self.m.run(600_000, tick=2000)

    def read_reply(self, _n):
        raw = strip_status_frames(bytes(self.m.tx_bytes))
        self.m.tx_bytes.clear()
        return raw

    def flush_input(self):
        self.m.tx_bytes.clear()

    def settle(self, seconds=3.0):
        """Give the render pass time to finish.

        A full 64-character repaint is 64 frames of 74 clocked bits with delays
        between them, which needs far more emulated cycles than a command
        exchange. Roughly a million steps per second of wall clock keeps the
        emulator's sense of time in the same neighbourhood as the real unit's.
        """
        self.m.run(int(seconds * 1_000_000), tick=2000)
        self.m.tx_bytes.clear()


class SerialTransport:
    """The real thing, once the physical layer is known.

    Defaults reflect the switches as found: see docs/dip-switches.md. The reply
    timeout allows for the firmware's fixed 3.9 ms turnaround plus the frame time,
    which is 183 ms for five bytes at 300 baud.
    """

    def __init__(self, port, baud=1200, bits=7, parity="E", timeout=1.0):
        import serial                                    # pyserial
        self.timeout = timeout
        self.bits = bits
        # One character time on the wire: start + data + parity + stop.
        self.char_time = (1 + bits + (0 if parity == "N" else 1) + 1) / baud
        self.s = serial.Serial(
            port, baud, timeout=0.02,          # short: read_reply owns the timing
            bytesize=serial.SEVENBITS if bits == 7 else serial.EIGHTBITS,
            parity={"E": serial.PARITY_EVEN, "O": serial.PARITY_ODD,
                    "N": serial.PARITY_NONE}[parity],
            stopbits=serial.STOPBITS_ONE)

    def write(self, data):
        self.s.write(data)
        self.s.flush()

    def settle(self, seconds=3.0):
        """Wait for the panel to finish repainting. Dots are mechanical."""
        time.sleep(seconds)

    def read_reply(self, n=0, idle=None):
        """Read until the line goes quiet, rather than counting bytes.

        Counting was a bug. Reply lengths are not fixed -- the SYN preamble is
        build-configurable (ROM 0x008A bit 1) and the error tail is conditional --
        so a read for exactly `n` bytes leaves the remainder in the OS buffer, and
        every later read is then one reply behind. On real hardware that showed up
        as `TAB` returning the previous reply's trailing SYNs and `BS` returning
        the first two bytes of the version string.

        Draining to an idle gap is immune to that: the unit sends a reply as one
        burst, so any silence longer than a few character times means it is done.
        """
        if idle is None:
            idle = max(0.03, 4 * self.char_time)
        raw = bytearray()
        deadline = time.time() + self.timeout
        last = None
        while time.time() < deadline:
            chunk = self.s.read(256)
            if chunk:
                raw += chunk
                last = time.time()
            elif last is not None and time.time() - last >= idle:
                break
        return strip_status_frames(bytes(raw))

    def flush_input(self):
        """Drop anything still buffered, so a command starts from a clean line."""
        self.s.reset_input_buffer()


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

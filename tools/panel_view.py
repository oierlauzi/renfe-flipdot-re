#!/usr/bin/env python3
"""Decode the flip-dot serial bit stream and render what the panel would show.

Watches the emulated firmware drive I/O port 0xE3, recovers the clocked serial
data, and reconstructs the 5x7 glyphs. That turns the emulator into a virtual
display: send a message, see the dots.

The panel interface is a shift register, not a parallel matrix -- see
docs/flipdot-interface.md for the derivation and, importantly, for the list of
assumptions this rests on.

    PYTHONPATH=tools python3 tools/panel_view.py --text HOLA
"""

import argparse
import re
import sys

from pit100_sim import PIT100 as Machine

CLOCK = 0x10          # port C bit 4
DATA = 0x20           # port C bit 5, active low: low = dot ON
IDLE_BITS = 37        # DATA-high padding before each frame's marker
GLYPH_BITS = 35       # 5 columns x 7 rows
FRAME_BITS = IDLE_BITS + 1 + GLYPH_BITS + 1


def capture(port_c_type=0x03, address=5, xram="board", cmds=b"",
            settle=250_000, run=900_000, tick=2000, port_b=0xFE):
    """Run the firmware and return the bits it clocked out of port 0xE3.

    port_b bit 0 must be CLEAR for text to be legible: set, it makes the pad
    character the solid block 0x23, and the alternator at 0x1348 then repaints
    every column with blocks -- which is what earlier captures were showing.
    """
    rom = open("original/firmware.bin", "rb").read()
    m = Machine(rom, 0x40 | (address & 0x1F), port_b, 0x20 | port_c_type,
                xram=xram)
    cpu = m.cpu
    bits, prev, original = [], [0], cpu.io_out

    def spy(port, value):
        if port == 0xE3:
            if (value & CLOCK) and not (prev[0] & CLOCK):    # rising edge
                bits.append(1 if value & DATA else 0)        # raw line level
            prev[0] = value
        original(port, value)

    cpu.io_out = spy
    for n in range(settle):
        m.poll_interrupts()
        if n % tick == 0:
            cpu.request(7.5)
        cpu.step()
    bits.clear()
    if cmds:
        m.feed(cmds)
    for n in range(run):
        m.poll_interrupts()
        if n % tick == 0:
            cpu.request(7.5)
        cpu.step()
    return bits, m


def frames(bits):
    """Split the stream into frames at the DATA-low marker.

    Each frame is IDLE_BITS ones, one zero (the marker), then GLYPH_BITS of data.
    A blank glyph is all ones, so the marker is the only reliable anchor -- do not
    try to find frames by looking for runs of idle, because a blank glyph *is*
    idle.
    """
    s = "".join(map(str, bits))
    for m in re.finditer("0", s):
        start = m.start()
        if start < IDLE_BITS:
            continue
        if s[start - IDLE_BITS:start] != "1" * IDLE_BITS:
            continue
        glyph = s[start + 1:start + 1 + GLYPH_BITS]
        if len(glyph) == GLYPH_BITS:
            yield start, glyph


def render(glyph):
    """35 bits -> 7 rows of 5. DATA low means the dot is ON, so invert."""
    cols = [glyph[k * 7:(k + 1) * 7] for k in range(5)]
    return ["".join("#" if cols[k][r] == "0" else "." for k in range(5))
            for r in range(7)]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--text", help="message to send before capturing")
    ap.add_argument("--fill", action="store_true",
                    help="send FF instead: fill the panel with the solid glyph")
    ap.add_argument("--address", type=lambda s: int(s, 0), default=5)
    ap.add_argument("--type", type=lambda s: int(s, 0), default=0x03,
                    help="port C bits 4:0, which select the panel driver "
                         "(default 0x03 -> 0xA009 = 0x11)")
    ap.add_argument("--port-b", type=lambda s: int(s, 0), default=0xFE,
                    help="81C55 port B switches. Bit 0 set makes the pad "
                         "character a solid block instead of a space "
                         "(default 0xFE = space)")
    ap.add_argument("--xram", choices=("none", "board", "alias"), default="board")
    ap.add_argument("--max", type=int, default=16, help="glyphs to show")
    args = ap.parse_args()

    from pit100 import GS, RS, STX, ETX, FF, address_byte, encode
    cmds = bytes((GS, address_byte(args.address)))
    if args.fill:
        cmds += bytes((FF,))
    elif args.text:
        cmds += bytes((STX,)) + encode(args.text, fold=True) + bytes((ETX,))
    # RS is mandatory: the repaint task is held off while the unit is selected,
    # so without it the text is stored, ACKed, and never drawn.
    cmds += bytes((RS,))

    bits, _ = capture(args.type, args.address, args.xram, cmds,
                      port_b=args.port_b)
    found = list(frames(bits))
    print(f"{len(bits)} clocked bits, {len(found)} frames "
          f"({FRAME_BITS} bits each)\n")

    shown = [render(g) for _, g in found[:args.max]]
    if not shown:
        print("no frames decoded - see docs/flipdot-interface.md for the "
              "conditions the render path needs")
        return
    for row in range(7):
        print("   " + "  ".join(g[row] for g in shown))
    blank = sum(1 for _, g in found if g == "1" * GLYPH_BITS)
    print(f"\n{blank} of {len(found)} frames were blank")


if __name__ == "__main__":
    main()

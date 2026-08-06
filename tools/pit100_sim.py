#!/usr/bin/env python3
"""Run the real PI-T100-M firmware against stub peripherals.

Feeds candidate frames into the emulated 8251A and reports what the firmware
does with them: which receive handler ran, what landed in the message buffer,
what it wrote to the flip-dot ports. Protocol hypotheses become experiments that
cost a second and cannot damage the panel.

Everything the model asserts about the hardware is documented in docs/io-map.md.
Where the hardware is unknown the stub says so rather than inventing behaviour --
see UNKNOWN_PORTS and the --xram flag.

Port C bit 5 must be SET or the unit never receives anything -- the RST5.5
handler returns immediately. Handler B additionally requires the unit to be
addressed with GS first. Both traps are documented in docs/dip-switches.md.

Examples:
    python3 tools/pit100_sim.py --boot
    python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xFF --xram board \
        --send '1D 35 1A'                       # GS '5' SUB -> identify
    python3 tools/pit100_sim.py --port-a 0x45 --port-c 0xFF --xram board \
        --send '1D 7F 02 48 4F 4C 41 03'        # broadcast, STX HOLA ETX
"""

import argparse
import sys
from collections import deque

from emu8085 import CPU8085

ROM_PATH = "original/firmware.bin"

# 6.144 MHz crystal, halved internally by the 8085. Lets cycle counts be
# reported as real time -- see docs/dip-switches.md.
CPU_HZ = 3_072_000

# 8251A status register bits.
TXRDY, RXRDY, TXEMPTY, PE, OE, FE, SYNDET, DSR = (
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80)

# Ports whose hardware is genuinely unidentified. Reads return 0xFF (floating
# bus); every access is counted so the report can say what was touched.
# 0xDE is an active-low hardware status input, accumulated into 0xA024 on every
# RST7.5 tick and reported by the BS command. Reading 0xFF here inverts to 0x00,
# i.e. "no faults", which is the right default for a stub.
UNKNOWN_PORTS = set(range(0xC0, 0xD0)) | {0xDE}


class PIT100:
    def __init__(self, rom, port_a=0xFF, port_b=0xFF, port_c=0xFF,
                 xram="none", trace_io=False):
        self.rom = rom
        self.ram = bytearray(256)              # 81C55, 0xA000-0xA0FF
        # What answers at 0xE000-0xFFFF is an open hardware question:
        #   none  - nothing there; writes vanish, reads float high
        #   board - the expansion board is fitted, 8 KiB of its own
        #   alias - incomplete address decoding makes 0xE0xx hit the same
        #           81C55 bytes as 0xA0xx (plausible if A14 is not decoded)
        # See docs/io-map.md; the three modes exist to be compared.
        self.xram_mode = xram
        self.xram = bytearray(0x2000) if xram == "board" else None
        self.has_xram = xram != "none"

        self.switches = {0xA1: port_a, 0xA2: port_b, 0xA3: port_c}
        self.trace_io = trace_io

        # Display ports read back what was last written. The firmware depends on
        # this: the glyph shift loop does IN 0xE3 / ANI 0x03 to *preserve* bits
        # 1:0, which carry the display attribute (docs/flipdot-interface.md).
        # Returning a floating 0xFF instead made those bits read as 11 forever,
        # silently discarding the attribute on every clocked bit.
        self.disp_latch = {0xE1: 0x00, 0xE2: 0x00, 0xE3: 0x00}

        # 8251A state
        self.rx_queue = deque()
        self.rx_data = 0
        self.status = TXRDY | TXEMPTY
        self.uart_expect = "mode"              # mode -> command after a reset
        self.uart_mode = None
        self.uart_cmds = []
        self.tx_bytes = bytearray()

        # Observations
        self.io_writes = []                    # (port, value)
        self.unknown_hits = {}
        self.disp_writes = []                  # writes to 0xE1/0xE2/0xE3
        self.lost_writes = []                  # writes into unbacked memory
        self.cpu = CPU8085(self.read, self.write, self.io_in, self.io_out)

    # ----------------------------------------------------------------- memory
    def read(self, addr):
        if addr < 0x8000:
            return self.rom[addr]
        if 0xA000 <= addr <= 0xA0FF:
            return self.ram[addr - 0xA000]
        if addr >= 0xE000:
            if self.xram_mode == "board":
                return self.xram[addr - 0xE000]
            if self.xram_mode == "alias":
                return self.ram[addr & 0xFF]
            return 0xFF
        return 0xFF

    def write(self, addr, value):
        if 0xA000 <= addr <= 0xA0FF:
            self.ram[addr - 0xA000] = value
        elif addr >= 0xE000:
            if self.xram_mode == "board":
                self.xram[addr - 0xE000] = value
            elif self.xram_mode == "alias":
                self.ram[addr & 0xFF] = value
            else:
                self.lost_writes.append((addr, value))
        elif addr < 0x8000:
            pass                               # writes to ROM are ignored
        else:
            self.lost_writes.append((addr, value))

    # -------------------------------------------------------------------- I/O
    def io_in(self, port):
        if port == 0x80:                       # 8251A RX data
            self.status &= ~RXRDY
            v = self.rx_data
            self._pump_rx()
            return v
        if port == 0x81:                       # 8251A status
            return self.status
        if port in self.switches:              # 81C55 ports A/B/C
            return self.switches[port]
        if port in self.disp_latch:            # display ports read back
            return self.disp_latch[port]
        if port == 0xE7:                       # expansion board capability
            return 0x00 if self.xram_mode == "board" else 0xFF
        if port in UNKNOWN_PORTS:
            self.unknown_hits[port] = self.unknown_hits.get(port, 0) + 1
            return 0xFF
        return 0xFF

    def io_out(self, port, value):
        self.io_writes.append((port, value))
        if self.trace_io:
            print(f"  OUT 0x{port:02X} <- 0x{value:02X}", file=sys.stderr)
        if port == 0x80:                       # 8251A TX data
            self.tx_bytes.append(value)
        elif port == 0x81:
            self._uart_control(value)
        elif port in (0xE1, 0xE2, 0xE3):
            self.disp_writes.append((port, value))
            self.disp_latch[port] = value
        elif port in UNKNOWN_PORTS:
            self.unknown_hits[port] = self.unknown_hits.get(port, 0) + 1

    def _uart_control(self, value):
        if self.uart_expect == "mode":
            self.uart_mode = value
            self.uart_expect = "command"
        else:
            self.uart_cmds.append(value)
            if value & 0x40:                   # internal reset
                self.uart_expect = "mode"
            if value & 0x10:                   # error reset
                self.status &= ~(PE | OE | FE)

    # ------------------------------------------------------------------- UART
    def feed(self, data):
        self.rx_queue.extend(data)
        self._pump_rx()

    def _pump_rx(self):
        if not (self.status & RXRDY) and self.rx_queue:
            self.rx_data = self.rx_queue.popleft()
            self.status |= RXRDY

    def poll_interrupts(self):
        """RxRDY drives RST5.5 -- see docs/serial-protocol.md.

        RST5.5 is *level* sensitive on the 8085, so this must track RxRDY in both
        directions. Latching it (request-only, never clear) makes the handler
        re-enter indefinitely on stale data once the mask is lifted, which looks
        exactly like a protocol timeout and is not one.
        """
        self.cpu.pending[5.5] = bool(self.status & RXRDY)

    # -------------------------------------------------------------------- run
    def run(self, max_steps, stop_pc=None, tick=None):
        """Step the CPU. `tick` requests RST7.5 every that many steps.

        Without a tick the idle loop at 0x1675 never clears its flag and the
        display is never repainted -- fine for protocol replies, useless for
        anything to do with the panel. The rate is arbitrary; see assumption A2
        in docs/flipdot-interface.md.
        """
        cpu = self.cpu
        seen_pc = set()
        for n in range(max_steps):
            self.poll_interrupts()
            if tick and n % tick == 0:
                cpu.request(7.5)
            if stop_pc is not None and cpu.pc == stop_pc and n > 1000:
                return n, "reached stop-pc"
            seen_pc.add(cpu.pc)
            try:
                cpu.step()
            except NotImplementedError as exc:
                return n, f"unimplemented: {exc}"
            except Exception as exc:                     # noqa: BLE001
                return n, f"{type(exc).__name__}: {exc}"
        return max_steps, "step limit"

    # ----------------------------------------------------------------- report
    def which_handler(self):
        sel = self.ram[0x0C]
        return ("rx_handler_b (0x0ACB)" if sel == 1
                else "rx_handler_a (0x08B7)") + f"  [0xA00C = 0x{sel:02X}]"


def parse_bytes(text):
    text = text.replace(",", " ")
    return bytes(int(t, 16) for t in text.split())


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=ROM_PATH)
    ap.add_argument("--steps", type=int, default=3_000_000)
    ap.add_argument("--boot", action="store_true",
                    help="just boot and report where the firmware settles")
    ap.add_argument("--send", metavar="HEX",
                    help="bytes to feed the UART, e.g. '02 41 42 03'")
    ap.add_argument("--send-str", metavar="TEXT",
                    help="text to feed, wrapped in --start/--end if given")
    ap.add_argument("--start", type=lambda s: int(s, 0),
                    help="start-of-frame byte to prepend to --send-str")
    ap.add_argument("--end", type=lambda s: int(s, 0),
                    help="end-of-frame byte to append to --send-str")
    ap.add_argument("--port-a", type=lambda s: int(s, 0), default=0xFF,
                    help="81C55 port A DIP switches (default 0xFF)")
    ap.add_argument("--port-b", type=lambda s: int(s, 0), default=0xFF)
    ap.add_argument("--port-c", type=lambda s: int(s, 0), default=0xFF)
    ap.add_argument("--xram", choices=("none", "board", "alias"), default="none",
                    help="what answers at 0xE000: nothing (default), a fitted "
                         "expansion board, or the 81C55 aliased by incomplete "
                         "address decoding")
    ap.add_argument("--trace-io", action="store_true")
    ap.add_argument("--show-buffer", action="store_true",
                    help="dump the receive buffer after running")
    ap.add_argument("--show-ram", action="store_true")
    args = ap.parse_args()

    with open(args.rom, "rb") as fh:
        rom = fh.read()

    m = PIT100(rom, args.port_a, args.port_b, args.port_c,
               xram=args.xram, trace_io=args.trace_io)
    print(f"config: port A=0x{args.port_a:02X} B=0x{args.port_b:02X} "
          f"C=0x{args.port_c:02X}  0xE000={args.xram}")

    # Boot far enough to finish initialisation and settle in the main loop.
    steps, why = m.run(200_000)
    print(f"boot: {steps} steps, {why} "
          f"({m.cpu.cycles / CPU_HZ * 1e3:.1f} ms simulated at {CPU_HZ/1e6:.3f} MHz)")
    print(f"  {m.cpu.state()}")
    print(f"  UART mode=0x{m.uart_mode:02X} commands="
          f"{[hex(c) for c in m.uart_cmds]}" if m.uart_mode is not None
          else "  UART was never initialised")
    print(f"  receive handler selected: {m.which_handler()}")

    if args.boot:
        report(m, args)
        return

    payload = b""
    if args.send:
        payload += parse_bytes(args.send)
    if args.send_str:
        body = args.send_str.encode("latin-1")
        if args.start is not None:
            body = bytes([args.start]) + body
        if args.end is not None:
            body += bytes([args.end])
        payload += body

    if not payload:
        print("\nnothing to send; use --send or --send-str (or --boot)")
        report(m, args)
        return

    print(f"\nfeeding {len(payload)} bytes: {payload.hex(' ')}")
    before = len(m.disp_writes)
    m.feed(payload)
    steps, why = m.run(args.steps)
    print(f"run: {steps} steps, {why}")
    print(f"  {m.cpu.state()}")
    print(f"  display port writes during/after: {len(m.disp_writes) - before}")
    report(m, args)


def summarise_tx(data, probe=16):
    """Transmit traffic is usually one frame repeated. Show the period, not the flood."""
    head = data[:64].hex(" ")
    for period in range(1, probe + 1):
        if len(data) >= period * 4 and all(
                data[i] == data[i % period] for i in range(len(data))):
            return (f", period {period}: {data[:period].hex(' ')} x"
                    f"{len(data) // period}")
    # Not perfectly periodic: report the distinct frames instead.
    for period in range(2, probe + 1):
        frames = {bytes(data[i:i + period]) for i in range(0, len(data) - period, period)}
        if len(frames) <= 4:
            listed = ", ".join(sorted(f.hex(" ") for f in frames))
            return f", {len(frames)} distinct {period}-byte frames: {listed}"
    return f"\n  first 64: {head}"


def report(m, args):
    print(f"\nmailbox 0xA012 = 0x{m.ram[0x12]:02X}"
          "   (0x4A = message complete, 0x4E = idle/receiving)")
    print(f"frame delimiters: start = 0x{m.ram[0x06]:02X}, "
          f"end = 0x{m.ram[0x07]:02X}, blank-as-space = 0x{m.ram[0x0B]:02X}")
    print(f"attribute byte 0xA014 = 0x{m.ram[0x14]:02X}")
    wptr = m.ram[0x0E] | (m.ram[0x0F] << 8)
    print(f"buffer write pointer 0xA00E = 0x{wptr:04X}")

    if m.tx_bytes:
        print(f"\nUART transmitted {len(m.tx_bytes)} bytes"
              f"{summarise_tx(m.tx_bytes)}")
    else:
        print("\nUART transmitted: nothing")

    if m.unknown_hits:
        hits = ", ".join(f"0x{p:02X}x{n}" for p, n in sorted(m.unknown_hits.items()))
        print(f"unidentified ports touched: {hits}")

    if m.lost_writes:
        lo = min(a for a, _ in m.lost_writes)
        hi = max(a for a, _ in m.lost_writes)
        print(f"\n** {len(m.lost_writes)} writes went to unbacked memory "
              f"(0x{lo:04X}-0x{hi:04X}) **")
        print("   The firmware expects memory that this model does not provide.")
        print("   Re-run with --xram to fit the expansion board and compare.")

    if args.show_buffer and m.has_xram:
        buf = m.xram[:64] if m.xram_mode == "board" else m.ram[:64]
        text = bytes(b if 32 <= b < 127 else 0x2E for b in buf)
        print(f"\nbuffer at 0xE000: {buf[:32].hex(' ')}")
        print(f"         as text: {text[:32].decode('ascii')}")

    if args.show_ram:
        print("\nRAM 0xA000:")
        for row in range(0, 256, 16):
            chunk = m.ram[row:row + 16]
            print(f"  A0{row:02X}  {chunk.hex(' ')}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Intel 8085 disassembler for the PI-T100 ROM, with symbol annotation.

Three modes, all reading ghidra/symbols.csv so output uses the project's names:

    python3 tools/disasm.py 0x1440-0x1470        # disassemble a range
    python3 tools/disasm.py --xref 0xA00A        # who touches this address/port
    python3 tools/disasm.py --calls 0x136A       # who calls this routine

The xref scan disassembles linearly from 0x0000, so it can lose sync inside data
regions and miss or invent a reference. It cross-checks against a raw byte search
and warns when the two disagree, which is the signal to look manually.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "original" / "firmware.bin"
SYMBOLS = ROOT / "ghidra" / "symbols.csv"

R = ("B", "C", "D", "E", "H", "L", "M", "A")
RP = ("B", "D", "H", "SP")
CC = ("NZ", "Z", "NC", "C", "PO", "PE", "P", "M")
ALU = ("ADD", "ADC", "SUB", "SBB", "ANA", "XRA", "ORA", "CMP")
ALUI = ("ADI", "ACI", "SUI", "SBI", "ANI", "XRI", "ORI", "CPI")

# Opcodes that carry a 16-bit address (as opposed to any 16-bit immediate).
ADDR16 = {0x22, 0x2A, 0x32, 0x3A,
          0xC2, 0xC3, 0xC4, 0xCA, 0xCC, 0xCD, 0xD2, 0xD4, 0xDA, 0xDC,
          0xE2, 0xE4, 0xEA, 0xEC, 0xF2, 0xF4, 0xFA, 0xFC}
CALLS = {0xC4, 0xCC, 0xCD, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC}


def build_table():
    """opcode -> (mnemonic template, operand length in bytes)."""
    t = {}
    for op in range(256):
        t[op] = ("DB   0x%02x" % op, 0)          # placeholder for undocumented

    t[0x00] = ("NOP", 0)
    t[0x20] = ("RIM", 0)
    t[0x30] = ("SIM", 0)
    t[0x76] = ("HLT", 0)
    t[0xE3] = ("XTHL", 0)
    t[0xE9] = ("PCHL", 0)
    t[0xEB] = ("XCHG", 0)
    t[0xF3] = ("DI", 0)
    t[0xF9] = ("SPHL", 0)
    t[0xFB] = ("EI", 0)
    t[0xC9] = ("RET", 0)
    t[0xC3] = ("JMP  {a}", 2)
    t[0xCD] = ("CALL {a}", 2)
    t[0xD3] = ("OUT  {p}", 1)
    t[0xDB] = ("IN   {p}", 1)

    for i, m in enumerate(("RLC", "RRC", "RAL", "RAR", "DAA", "CMA", "STC", "CMC")):
        t[0x07 + i * 8] = (m, 0)
    for i, m in enumerate(("STAX B", "STAX D", "SHLD {a}", "STA  {a}")):
        t[0x02 + i * 0x10] = (m, 2 if "{a}" in m else 0)
    for i, m in enumerate(("LDAX B", "LDAX D", "LHLD {a}", "LDA  {a}")):
        t[0x0A + i * 0x10] = (m, 2 if "{a}" in m else 0)

    for i in range(4):
        t[0x01 + i * 0x10] = ("LXI  %s,{w}" % RP[i], 2)
        t[0x03 + i * 0x10] = ("INX  %s" % RP[i], 0)
        t[0x0B + i * 0x10] = ("DCX  %s" % RP[i], 0)
        t[0x09 + i * 0x10] = ("DAD  %s" % RP[i], 0)
    for r in range(8):
        t[0x04 + r * 8] = ("INR  %s" % R[r], 0)
        t[0x05 + r * 8] = ("DCR  %s" % R[r], 0)
        t[0x06 + r * 8] = ("MVI  %s,{b}" % R[r], 1)
    for d in range(8):
        for s in range(8):
            if not (d == 6 and s == 6):
                t[0x40 + d * 8 + s] = ("MOV  %s,%s" % (R[d], R[s]), 0)
    for g in range(8):
        for s in range(8):
            t[0x80 + g * 8 + s] = ("%-4s %s" % (ALU[g], R[s]), 0)
        t[0xC6 + g * 8] = ("%-4s {b}" % ALUI[g], 1)
        t[0xC7 + g * 8] = ("RST  %d" % g, 0)
        t[0xC0 + g * 8] = ("R%s" % CC[g], 0)
        t[0xC2 + g * 8] = ("J%-3s {a}" % CC[g], 2)
        t[0xC4 + g * 8] = ("C%-3s {a}" % CC[g], 2)
    for i, n in enumerate(("B", "D", "H", "PSW")):
        t[0xC1 + i * 0x10] = ("POP  %s" % n, 0)
        t[0xC5 + i * 0x10] = ("PUSH %s" % n, 0)
    return t


TABLE = build_table()


def load_symbols():
    mem, io = {}, {}
    if not SYMBOLS.exists():
        return mem, io
    rows = [l for l in open(SYMBOLS) if not l.startswith("#")]
    for r in csv.DictReader(rows):
        (io if r["space"] == "io" else mem)[int(r["addr"], 16)] = r["name"]
    return mem, io


MEM, IO = load_symbols()


def sym(addr):
    name = MEM.get(addr)
    return "%s{0x%04x}" % (name, addr) if name else "0x%04x" % addr


def port(p):
    name = IO.get(p)
    return "%s{0x%02x}" % (name, p) if name else "0x%02x" % p


def decode(rom, pc):
    """Return (text, length, target) for the instruction at pc."""
    op = rom[pc]
    tmpl, n = TABLE[op]
    lo = rom[pc + 1] if n >= 1 and pc + 1 < len(rom) else 0
    hi = rom[pc + 2] if n >= 2 and pc + 2 < len(rom) else 0
    word = lo | (hi << 8)
    target = word if op in ADDR16 else None
    text = (tmpl.replace("{a}", sym(word))
                .replace("{w}", "0x%04x" % word)
                .replace("{b}", "0x%02x" % lo)
                .replace("{p}", port(lo)))
    return text, 1 + n, target


def disassemble(rom, start, end):
    pc = start
    while pc < min(end, len(rom)):
        text, n, _ = decode(rom, pc)
        raw = " ".join("%02x" % b for b in rom[pc:pc + n])
        label = MEM.get(pc)
        if label:
            print("\n%s:" % label)
        print("%04x  %-9s %s" % (pc, raw, text))
        pc += n


def scan(rom):
    """Linear disassembly of the whole image, yielding (pc, text, length)."""
    pc = 0
    limit = len(rom)
    while pc < limit:
        text, n, _ = decode(rom, pc)
        yield pc, text, n
        pc += n


def xref(rom, needle, only_calls=False):
    """Report instructions referring to `needle`, with a raw-byte cross-check."""
    hits = []
    for pc, text, n in scan(rom):
        op = rom[pc]
        if only_calls:
            if op in CALLS and (rom[pc + 1] | rom[pc + 2] << 8) == needle:
                hits.append((pc, text))
            continue
        if op in ADDR16 and (rom[pc + 1] | rom[pc + 2] << 8) == needle:
            hits.append((pc, text))
        elif op in (0xD3, 0xDB) and needle < 0x100 and rom[pc + 1] == needle:
            hits.append((pc, text))
        elif op in (0x01, 0x11, 0x21) and (rom[pc + 1] | rom[pc + 2] << 8) == needle:
            hits.append((pc, text))          # LXI: a pointer being set up

    raw = rom.count(bytes((needle & 0xFF, needle >> 8))) if needle >= 0x100 else None
    for pc, text in hits:
        print("%04x  %s" % (pc, text))
    print("\n%d reference%s" % (len(hits), "" if len(hits) == 1 else "s"), end="")
    if raw is not None:
        print(", %d raw occurrence%s of the little-endian bytes"
              % (raw, "" if raw == 1 else "s"), end="")
        if raw > len(hits):
            print("\nNOTE: more raw hits than decoded ones -- the linear scan may "
                  "have lost sync in a data region. Check manually.", end="")
    print()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("range", nargs="?", help="e.g. 0x1440-0x1470 or 0x1440+0x30")
    ap.add_argument("--xref", type=lambda s: int(s, 0),
                    help="find references to a memory address or I/O port")
    ap.add_argument("--calls", type=lambda s: int(s, 0),
                    help="find CALLs to a routine")
    args = ap.parse_args()

    rom = ROM.read_bytes()
    if args.xref is not None:
        return xref(rom, args.xref)
    if args.calls is not None:
        return xref(rom, args.calls, only_calls=True)
    if not args.range:
        ap.error("give a range, --xref or --calls")

    m = re.fullmatch(r"(0x[0-9a-fA-F]+|\d+)\s*([-+])\s*(0x[0-9a-fA-F]+|\d+)",
                     args.range)
    if not m:
        ap.error("bad range %r" % args.range)
    start = int(m.group(1), 0)
    n = int(m.group(3), 0)
    end = n if m.group(2) == "-" else start + n
    disassemble(rom, start, end)


if __name__ == "__main__":
    sys.exit(main())

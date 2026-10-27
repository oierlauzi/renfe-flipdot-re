#!/usr/bin/env python3
"""Intel 8085 disassembler for the PI-T100 ROM, with symbol annotation.

Four modes, all reading ghidra/symbols.csv so output uses the project's names:

    python3 tools/disasm.py 0x1440-0x1470        # disassemble a range
    python3 tools/disasm.py --xref 0xA00A        # who touches this address/port
    python3 tools/disasm.py --calls 0x136A       # who calls this routine
    python3 tools/disasm.py --asm > src/firmware.asm    # full listing for ASL

The xref scan disassembles linearly from 0x0000, so it can lose sync inside data
regions and miss or invent a reference. It cross-checks against a raw byte search
and warns when the two disagree, which is the signal to look manually.

--asm emits Macro Assembler AS syntax for the whole used image. It self-checks
that the bytes it accounts for reconstruct the ROM exactly, which proves the
listing is a faithful decode. It does **not** prove that AS re-encodes each
mnemonic to the same bytes -- only running AS does that. See `make verify`.
"""

import argparse
import bisect
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
    # {w} is any 16-bit immediate, usually a constant -- name it only when the
    # symbol table independently says that value is an address.
    text = (tmpl.replace("{a}", sym(word))
                .replace("{w}", sym(word) if word in MEM else "0x%04x" % word)
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


# ------------------------------------------------------------------ ASL listing

USED_END = 0x3048           # rom_info.py asserts everything above this is 0xFF

# Regions that are data, not code: (start, end_exclusive, label, note, width).
# `width` is bytes per emitted line -- set it to the entry stride so one line is
# one table entry. Getting a region wrong costs readability, not correctness: a
# mis-decoded stretch still re-assembles to the same bytes.
DATA_REGIONS = [
    # 0x0040-0x0063 was listed here as an "unclassified table". It is not data:
    # it is twelve 3-byte JMP trampolines, ten to fatal_or_init and one each to
    # isr_rst55 and isr_rst75 -- a second copy of the vector table, in a layout
    # that does not match the 8085's. Left as code so it reads as what it is.
    (0x0064, 0x0090, "id_block",
     "SUB reply: version string then the config block", 8),
    (0x00D2, 0x00E6, "reply_affixes",
     "per-command reply prefixes/suffixes, 0xFF-terminated", 8),
    (0x1C0D, 0x210D, "font_table",
     "256 glyphs x 5 columns, one per line -- see docs/5-the-panel.md", 5),
    (0x23CF, 0x24BB, "font_4x7",
     "59 glyphs x 4 columns x 7 rows, codes 0x20-0x5A, columns stored right to "
     "left. Found via the linear search at 0x234B -- see docs/5-the-panel.md", 4),
    (0x2692, 0x2792, "alt_char_table",
     "128 pointers indexed by a 7-bit character code; 86 share the blank", 2),
    (0x2792, 0x2915, "alt_char_data",
     "43 entries x 9 bytes: 03h, then two 30-bit groups of 3 columns in "
     "10-bit fields. Glyph = top 5 bits of each field, so 3x5. Only 0-9 "
     ":;<=>? and A-Z are non-blank. See docs/5-the-panel.md", 9),
]


# Characters allowed in the ASCII gutter of a `db` line. A trailing backslash is
# a line continuation in AS, so an unsanitised comment can silently swallow the
# next line -- which cost 5 bytes and a confusing 5-byte label shift once.
SAFE_COMMENT = set(
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "
    "!#$%&()*+,-./:<=>?@[]^_{|}~")


def gutter(chunk):
    """Printable rendering of data bytes, safe to put after a `;`."""
    return "".join(chr(b) if 32 <= b < 127 and chr(b) in SAFE_COMMENT else "."
                   for b in chunk)


def asl_hex(value, width=2):
    """Intel-style hex: a leading zero when it would otherwise start with a letter."""
    text = f"{value:0{width}X}"
    return ("0" + text if text[0].isalpha() else text) + "h"


MEM_OPS = {0x22, 0x2A, 0x32, 0x3A}      # SHLD, LHLD, STA, LDA


def ref_kind(op):
    """How an instruction reaches its 16-bit operand."""
    if op in CALLS:
        return "call"
    if op in MEM_OPS:
        return "data"
    return "jump"                        # JMP and the conditional jumps


POPS = {0xC1, 0xD1, 0xE1, 0xF1}


def is_epilogue(rom, addr):
    """True when addr starts a run of POPs ending in RET, and nothing else."""
    while rom[addr] in POPS:
        addr += 1
    return rom[addr] == 0xC9


def name_labels(rom, labels, refs, boundaries):
    """Fill in every label the symbol table does not name.

    The generated name says where the label sits and how it is reached:
    `<routine>_<role><n>`, where the routine is the nearest preceding entry
    point -- a named symbol or a CALL target -- and the role is one of

        ret   it sits on an epilogue -- POPs and a RET -- so it is a shared exit
        loop  something branches back to it from a higher address
        ptr   only SHLD/LHLD/STA/LDA refer to it, so it is data, not code
        br    a plain forward branch target

    None of that is a claim about meaning: it is read straight off the control
    flow. Naming a routine in ghidra/symbols.csv renames everything inside it,
    which is the point -- understanding one entry point pays off across its whole
    body. The ordinals are per routine and per role, so adding a name reshuffles
    the numbering below it; that churn is confined to the generated listing.
    """
    starts = sorted({a for a in boundaries if a in MEM}
                    | {a for a, rs in refs.items()
                       if a in boundaries and any(k == "call" for _, k in rs)}
                    | {0x0000})
    entry = {a: MEM.get(a) or f"sub_{a:04X}" for a in starts}

    used = set(MEM.values())
    counters = {}
    for addr in sorted(a for a in labels if labels[a] is None):
        if addr in entry:                            # an unnamed entry point
            labels[addr] = entry[addr]
            used.add(entry[addr])
            continue
        owner = starts[bisect.bisect_right(starts, addr) - 1]
        kinds = {k for _, k in refs.get(addr, ())}
        if is_epilogue(rom, addr):
            role = "ret"
        elif any(src > addr for src, _ in refs.get(addr, ())):
            role = "loop"
        elif kinds == {"data"}:
            role = "ptr"
        else:
            role = "br"
        counters[owner, role] = counters.get((owner, role), 0) + 1
        name = f"{entry[owner]}_{role}{counters[owner, role]}"
        if name in used:
            raise SystemExit(f"generated label {name} collides with a name in "
                             f"{SYMBOLS.name}")
        labels[addr] = name
        used.add(name)


def build_listing(rom):
    """Return (lines, covered_bytes). Raises if coverage does not match the ROM."""
    data_starts = {s: (lbl, note, w) for s, _, lbl, note, w in DATA_REGIONS}
    in_data = set()
    for start, end, _, _, _ in DATA_REGIONS:
        in_data.update(range(start, end))

    # Pass 1: instruction boundaries, and every reference with its source.
    boundaries, refs, pc = set(), {}, 0
    while pc < USED_END:
        if pc in in_data:
            pc += 1
            continue
        boundaries.add(pc)
        op = rom[pc]
        _, n, target = decode(rom, pc)
        if target is not None:
            refs.setdefault(target, []).append((pc, ref_kind(op)))
        # LXI operands are NOT added: most are constants, not addresses.
        # `LXI H,01F4h` is the delay count 500, and minting a label for it would
        # invent a code reference that does not exist. LXI still *renders* as a
        # label when the target is independently known -- see operand().
        pc += n

    # A label may only sit where an instruction or a data region starts.
    placeable = boundaries | set(data_starts)
    labels = {}
    for addr in sorted(set(refs) | set(MEM) | set(data_starts)):
        if addr >= USED_END or addr not in placeable:
            continue
        labels[addr] = MEM.get(addr) or data_starts.get(addr, (None,))[0]
    name_labels(rom, labels, refs, boundaries)

    def operand(addr):
        """Prefer a label inside the image, then an equate for RAM and I/O."""
        if addr in labels:
            return labels[addr]
        if addr >= USED_END and addr in MEM:
            return MEM[addr]
        return asl_hex(addr, 4)

    lines = [
        "; PAGE Iberica / Telenorma PI-T100-M -- matching disassembly",
        ";",
        "; Generated by tools/disasm.py --asm from original/firmware.bin.",
        "; Assemble with Macro Assembler AS and verify with `make verify`, which",
        "; compares the result byte for byte against the original ROM.",
        ";",
        f"; Covers 0000h-{USED_END - 1:04X}h. Everything above is erased 0FFh;",
        "; tools/rom_info.py asserts that separately.",
        ";",
        "; Names come from ghidra/symbols.csv. Anything it does not name is called",
        "; <routine>_<role><n>, where the routine is the nearest preceding entry",
        "; point and the role is read off the control flow: loop for a backward",
        "; branch target, ret for a shared POP/RET epilogue, ptr for an address only",
        "; SHLD/LHLD/STA/LDA reach, br for a plain forward branch. Those names carry",
        "; no claim about meaning -- naming a routine in symbols.csv renames its",
        "; whole body on the next `make disasm`.",
        "",
        "\tcpu\t8085",
        "\tpage\t0",
        "",
        "; ---------------------------------------------------------- I/O ports",
    ]
    for port in sorted(IO):
        lines.append(f"{IO[port]}\tequ\t{asl_hex(port)}")
    lines += [
        "",
        "; --------------------------------------------- RAM and expansion window",
    ]
    for addr in sorted(a for a in MEM if a >= USED_END):
        lines.append(f"{MEM[addr]}\tequ\t{asl_hex(addr, 4)}")

    # Named addresses that fall *inside* a data region cannot carry a label, so
    # define them as equates instead. Without this, referring to one by name is
    # an undefined symbol.
    interior = sorted(a for a in MEM if a < USED_END and a not in labels)
    if interior:
        lines += ["", "; ------------------- names pointing inside data regions"]
        for addr in interior:
            lines.append(f"{MEM[addr]}\tequ\t{asl_hex(addr, 4)}")
    lines += ["", "\torg\t0000h", ""]

    covered = bytearray()
    pc = 0
    while pc < USED_END:
        if pc in labels:
            lines.append(f"{labels[pc]}:")
        if pc in data_starts:
            start = pc
            end = next(e for s, e, _, _, _ in DATA_REGIONS if s == pc)
            _, note, width = data_starts[pc]
            lines.append(f"\t; {note}  ({end - start} bytes)")
            for row in range(start, end, width):
                chunk = rom[row:min(row + width, end)]
                lines.append("\tdb\t" + ",".join(asl_hex(b) for b in chunk)
                             + f"\t; {row:04X}  {gutter(chunk)}")
            covered += rom[start:end]
            pc = end
            continue

        text, n, target = decode(rom, pc)
        op = rom[pc]
        if target is not None:
            mnem = text.split(None, 1)[0]
            text = f"{mnem}\t{operand(target)}"
        elif op in (0x01, 0x11, 0x21) and n == 3:
            # Only render as a name when the target is independently known to be
            # an address; otherwise it is a constant and stays numeric.
            word = rom[pc + 1] | (rom[pc + 2] << 8)
            known = MEM.get(word) or (labels.get(word) if word in data_starts
                                      else None)
            text = f"LXI\t{RP[(op >> 4) & 3]},{known or asl_hex(word, 4)}"
        elif op in (0xD3, 0xDB):
            name = IO.get(rom[pc + 1])
            text = f"{'OUT' if op == 0xD3 else 'IN'}\t{name or asl_hex(rom[pc + 1])}"
        else:
            parts = text.split(None, 1)
            text = parts[0] + ("\t" + parts[1].strip() if len(parts) > 1 else "")
            text = re.sub(r"0x([0-9a-fA-F]+)",
                          lambda m: asl_hex(int(m.group(1), 16),
                                            len(m.group(1))), text)
        lines.append(f"\t{text}")
        covered += rom[pc:pc + n]
        pc += n

    if bytes(covered) != rom[:USED_END]:
        raise SystemExit("listing does not reconstruct the ROM -- refusing to emit")
    lines += ["", "\tend", ""]
    return lines, len(covered)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("range", nargs="?", help="e.g. 0x1440-0x1470 or 0x1440+0x30")
    ap.add_argument("--xref", type=lambda s: int(s, 0),
                    help="find references to a memory address or I/O port")
    ap.add_argument("--calls", type=lambda s: int(s, 0),
                    help="find CALLs to a routine")
    ap.add_argument("--asm", action="store_true",
                    help="emit a full Macro Assembler AS listing on stdout")
    args = ap.parse_args()

    rom = ROM.read_bytes()
    if args.asm:
        lines, n = build_listing(rom)
        print("\n".join(lines))
        print(f"; {n} bytes accounted for, reconstructing the ROM exactly.",
              file=sys.stderr)
        return
    if args.xref is not None:
        return xref(rom, args.xref)
    if args.calls is not None:
        return xref(rom, args.calls, only_calls=True)
    if not args.range:
        ap.error("give a range, --xref, --calls or --asm")

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

# Apply the version-controlled symbol table to the current Ghidra program.
#
# Run from Ghidra's Script Manager (Jython) with the firmware program open, or
# headless:
#
#   analyzeHeadless <proj_dir> <proj_name> -process firmware.bin \
#       -scriptPath ghidra/scripts -postScript apply_symbols.py
#
# Reads ghidra/symbols.csv, creates labels/functions and attaches comments. Safe
# to re-run: existing names at the same address are replaced, everything else is
# left alone.
#
# @category RENFE-flipdot
# @runtime Jython

import csv
import os

from ghidra.program.model.symbol import SourceType

# The 8085 has a separate I/O space. Ghidra's 8085 language exposes it as an
# address space; if this import ever fails, check the space name with
# currentProgram.getAddressFactory().getAddressSpaces().
IO_SPACE_CANDIDATES = ("io", "IO", "port", "PORT")


def repo_root():
    # Script lives in <root>/ghidra/scripts/.
    here = os.path.dirname(os.path.abspath(getSourceFile().getAbsolutePath()))
    return os.path.abspath(os.path.join(here, os.pardir, os.pardir))


def io_space(program):
    factory = program.getAddressFactory()
    for name in IO_SPACE_CANDIDATES:
        space = factory.getAddressSpace(name)
        if space is not None:
            return space
    return None


def main():
    program = currentProgram
    listing = program.getListing()
    symtab = program.getSymbolTable()
    default_space = program.getAddressFactory().getDefaultAddressSpace()
    ports = io_space(program)

    path = os.path.join(repo_root(), "ghidra", "symbols.csv")
    print("reading %s" % path)

    applied = skipped = 0
    with open(path) as fh:
        rows = (line for line in fh if not line.startswith("#"))
        for row in csv.DictReader(rows):
            space_kind = (row["space"] or "").strip()
            if space_kind == "io":
                if ports is None:
                    print("  skip io:%s - no I/O address space in this program"
                          % row["addr"])
                    skipped += 1
                    continue
                space = ports
            else:
                space = default_space

            addr = space.getAddress(int(row["addr"].strip(), 16))
            name = row["name"].strip()
            kind = (row["kind"] or "label").strip()
            comment = (row["comment"] or "").strip()

            # Drop any previous name we placed here so re-runs are idempotent.
            for sym in symtab.getSymbols(addr):
                if sym.getSource() == SourceType.USER_DEFINED:
                    sym.delete()

            if kind == "func":
                func = listing.getFunctionAt(addr)
                if func is None:
                    func = createFunction(addr, name)
                if func is not None:
                    func.setName(name, SourceType.USER_DEFINED)
                else:
                    createLabel(addr, name, True)
            else:
                createLabel(addr, name, True)

            if comment:
                setPlateComment(addr, comment)

            applied += 1

    print("applied %d symbol(s), skipped %d" % (applied, skipped))


main()

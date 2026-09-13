#!/usr/bin/env python3
"""Expected panel bytes for each line of stdin, straight from tools/pit100.py.

This is the whole point of the test: the C encoder in controller/main/cp437.c is
a second implementation of a 128-entry table and a 15-entry accent fold, and a
typo in either would be invisible on the panel until someone sent an N-tilde. So
the reference stays the Python that has actually driven the hardware, and nothing
is committed as expected output -- it is recomputed here on every run.

Only the 8-bit path is compared. The 7-bit rules are where the C deliberately
differs (it refuses characters that cannot cross the link, which pit100.py never
had to do), and those are asserted in test_cp437.c instead.
"""

import sys

from pit100 import encode

for line in sys.stdin:
    text = line.rstrip("\n").rstrip("\r")
    try:
        print(encode(text, fold=True).hex(" ").upper())
    except ValueError:
        print("REFUSED")

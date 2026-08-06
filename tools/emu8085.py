#!/usr/bin/env python3
"""A small, complete Intel 8085 CPU core.

Deliberately machine-agnostic: memory and I/O are supplied as callbacks so the
same core can host the PI-T100 model in pit100_sim.py or any other 8085 system.

Accuracy goals, in order: correct instruction semantics, correct flags, correct
interrupt behaviour. Cycle counts are the published 8085 figures and are good
enough to drive timers, but this is not a cycle-exact simulator.
"""

# Flag bit positions in the F register.
S, Z, AC, P, CY = 0x80, 0x40, 0x10, 0x04, 0x01

PARITY = [0] * 256
for _i in range(256):
    PARITY[_i] = P if bin(_i).count("1") % 2 == 0 else 0

# Cycle counts indexed by opcode; conditional instructions use the "not taken"
# figure and add the difference when taken.
CYCLES = [
    4, 10, 7, 6, 4, 4, 7, 4, 4, 10, 7, 6, 4, 4, 7, 4,
    4, 10, 7, 6, 4, 4, 7, 4, 4, 10, 7, 6, 4, 4, 7, 4,
    4, 10, 16, 6, 4, 4, 7, 4, 4, 10, 16, 6, 4, 4, 7, 4,
    4, 10, 13, 6, 10, 10, 10, 4, 4, 10, 13, 6, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    7, 7, 7, 7, 7, 7, 5, 7, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    6, 10, 10, 10, 9, 12, 7, 12, 6, 10, 10, 10, 9, 9, 7, 12,
    6, 10, 10, 10, 9, 12, 7, 12, 6, 10, 10, 10, 9, 9, 7, 12,
    6, 10, 10, 16, 9, 12, 7, 12, 6, 6, 10, 4, 9, 9, 7, 12,
    6, 10, 10, 4, 9, 12, 7, 12, 6, 6, 10, 4, 9, 9, 7, 12,
]


class Halted(Exception):
    """Raised when the CPU executes HLT with interrupts unable to resume it."""


class CPU8085:
    def __init__(self, read, write, io_in, io_out):
        self.read = read            # (addr) -> byte
        self.write = write          # (addr, value)
        self.io_in = io_in          # (port) -> byte
        self.io_out = io_out        # (port, value)

        self.a = 0
        self.f = 0x02               # bit 1 reads as 1 on real silicon
        self.b = self.c = self.d = self.e = self.h = self.l = 0
        self.sp = 0
        self.pc = 0

        self.ie = False             # interrupt enable (EI/DI)
        self.masks = {5.5: True, 6.5: True, 7.5: True}   # True = masked off
        self.pending = {5.5: False, 6.5: False, 7.5: False}
        self.sod = 0                # serial output data pin, used as GPIO here
        self.sid = 0
        self.halted = False
        self.cycles = 0

    # ---------------------------------------------------------------- helpers
    def _rp(self, hi, lo):
        return (hi << 8) | lo

    @property
    def hl(self):
        return (self.h << 8) | self.l

    @hl.setter
    def hl(self, v):
        self.h, self.l = (v >> 8) & 0xFF, v & 0xFF

    def _fetch(self):
        v = self.read(self.pc)
        self.pc = (self.pc + 1) & 0xFFFF
        return v

    def _fetch16(self):
        lo = self._fetch()
        return lo | (self._fetch() << 8)

    def _push(self, v):
        self.sp = (self.sp - 1) & 0xFFFF
        self.write(self.sp, (v >> 8) & 0xFF)
        self.sp = (self.sp - 1) & 0xFFFF
        self.write(self.sp, v & 0xFF)

    def _pop(self):
        lo = self.read(self.sp)
        self.sp = (self.sp + 1) & 0xFFFF
        hi = self.read(self.sp)
        self.sp = (self.sp + 1) & 0xFFFF
        return (hi << 8) | lo

    # ------------------------------------------------------------------ flags
    def _szp(self, v):
        self.f &= ~(S | Z | P)
        if v & 0x80:
            self.f |= S
        if v == 0:
            self.f |= Z
        self.f |= PARITY[v]

    def _add(self, v, carry=0):
        r = self.a + v + carry
        self.f &= ~(AC | CY)
        if ((self.a & 0x0F) + (v & 0x0F) + carry) > 0x0F:
            self.f |= AC
        if r > 0xFF:
            self.f |= CY
        self.a = r & 0xFF
        self._szp(self.a)

    def _sub(self, v, borrow=0):
        r = self.a - v - borrow
        self.f &= ~(AC | CY)
        if ((self.a & 0x0F) - (v & 0x0F) - borrow) >= 0:
            self.f |= AC
        if r < 0:
            self.f |= CY
        self.a = r & 0xFF
        self._szp(self.a)

    def _cmp(self, v):
        r = self.a - v
        self.f &= ~(AC | CY)
        if ((self.a & 0x0F) - (v & 0x0F)) >= 0:
            self.f |= AC
        if r < 0:
            self.f |= CY
        self._szp(r & 0xFF)

    def _ana(self, v):
        # 8085 sets AC from the OR of bit 3 of both operands.
        ac = AC if ((self.a | v) & 0x08) else 0
        self.a &= v
        self.f &= ~(AC | CY)
        self.f |= ac
        self._szp(self.a)

    def _xra(self, v):
        self.a ^= v
        self.f &= ~(AC | CY)
        self._szp(self.a)

    def _ora(self, v):
        self.a |= v
        self.f &= ~(AC | CY)
        self._szp(self.a)

    # --------------------------------------------------------- register files
    def _get_r(self, idx):
        return (self.b, self.c, self.d, self.e, self.h, self.l,
                self.read(self.hl), self.a)[idx]

    def _set_r(self, idx, v):
        v &= 0xFF
        if idx == 0:   self.b = v
        elif idx == 1: self.c = v
        elif idx == 2: self.d = v
        elif idx == 3: self.e = v
        elif idx == 4: self.h = v
        elif idx == 5: self.l = v
        elif idx == 6: self.write(self.hl, v)
        else:          self.a = v

    def _get_rp(self, idx):
        if idx == 0: return self._rp(self.b, self.c)
        if idx == 1: return self._rp(self.d, self.e)
        if idx == 2: return self.hl
        return self.sp

    def _set_rp(self, idx, v):
        v &= 0xFFFF
        if idx == 0:   self.b, self.c = v >> 8, v & 0xFF
        elif idx == 1: self.d, self.e = v >> 8, v & 0xFF
        elif idx == 2: self.hl = v
        else:          self.sp = v

    def _cond(self, idx):
        return (not self.f & Z, self.f & Z, not self.f & CY, self.f & CY,
                not self.f & P, self.f & P, not self.f & S, self.f & S)[idx]

    # ------------------------------------------------------------- interrupts
    def request(self, which):
        """Assert an interrupt line.

        RST7.5 is edge-latched on real silicon, so setting it here is enough and
        it self-clears when serviced. RST5.5 and RST6.5 are **level** sensitive:
        a machine model must drive `pending[5.5]` low again when its source
        deasserts, rather than only ever calling this.
        """
        self.pending[which] = True

    def _service_interrupts(self):
        if not self.ie:
            return False
        for which, vector in ((7.5, 0x003C), (6.5, 0x0034), (5.5, 0x002C)):
            if self.pending[which] and not self.masks[which]:
                if which == 7.5:
                    self.pending[which] = False   # edge triggered, auto-clears
                self.ie = False
                self.halted = False
                self._push(self.pc)
                self.pc = vector
                self.cycles += 12
                return True
        return False

    # ------------------------------------------------------------------- step
    def step(self):
        """Execute one instruction (or take one interrupt). Returns cycles used."""
        start = self.cycles
        if self._service_interrupts():
            return self.cycles - start
        if self.halted:
            self.cycles += 4
            return 4

        op = self._fetch()
        self.cycles += CYCLES[op]

        # MOV r,r / HLT
        if 0x40 <= op <= 0x7F:
            if op == 0x76:
                self.halted = True
            else:
                self._set_r((op >> 3) & 7, self._get_r(op & 7))
            return self.cycles - start

        # ALU r
        if 0x80 <= op <= 0xBF:
            v = self._get_r(op & 7)
            k = (op >> 3) & 7
            (self._add, lambda x: self._add(x, 1 if self.f & CY else 0),
             self._sub, lambda x: self._sub(x, 1 if self.f & CY else 0),
             self._ana, self._xra, self._ora, self._cmp)[k](v)
            return self.cycles - start

        hi = op >> 3

        if op == 0x00 or op in (0x08, 0x10, 0x18, 0x28, 0x38):   # NOP + undocumented
            pass
        elif op & 0xCF == 0x01:                                   # LXI
            self._set_rp((op >> 4) & 3, self._fetch16())
        elif op in (0x02, 0x12):                                  # STAX
            self.write(self._get_rp((op >> 4) & 3), self.a)
        elif op in (0x0A, 0x1A):                                  # LDAX
            self.a = self.read(self._get_rp((op >> 4) & 3))
        elif op & 0xCF == 0x03:                                   # INX
            i = (op >> 4) & 3
            self._set_rp(i, self._get_rp(i) + 1)
        elif op & 0xCF == 0x0B:                                   # DCX
            i = (op >> 4) & 3
            self._set_rp(i, self._get_rp(i) - 1)
        elif op & 0xC7 == 0x04:                                   # INR
            i = hi & 7
            v = (self._get_r(i) + 1) & 0xFF
            self.f = (self.f & ~AC) | (AC if (v & 0x0F) == 0 else 0)
            self._set_r(i, v)
            self._szp(v)
        elif op & 0xC7 == 0x05:                                   # DCR
            i = hi & 7
            old = self._get_r(i)
            v = (old - 1) & 0xFF
            self.f = (self.f & ~AC) | (0 if (old & 0x0F) == 0 else AC)
            self._set_r(i, v)
            self._szp(v)
        elif op & 0xC7 == 0x06:                                   # MVI
            self._set_r(hi & 7, self._fetch())
        elif op & 0xCF == 0x09:                                   # DAD
            r = self.hl + self._get_rp((op >> 4) & 3)
            self.f = (self.f & ~CY) | (CY if r > 0xFFFF else 0)
            self.hl = r & 0xFFFF
        elif op == 0x07:                                          # RLC
            self.a = ((self.a << 1) | (self.a >> 7)) & 0xFF
            self.f = (self.f & ~CY) | (CY if self.a & 1 else 0)
        elif op == 0x0F:                                          # RRC
            self.f = (self.f & ~CY) | (CY if self.a & 1 else 0)
            self.a = ((self.a >> 1) | ((self.a & 1) << 7)) & 0xFF
        elif op == 0x17:                                          # RAL
            c = 1 if self.f & CY else 0
            self.f = (self.f & ~CY) | (CY if self.a & 0x80 else 0)
            self.a = ((self.a << 1) | c) & 0xFF
        elif op == 0x1F:                                          # RAR
            c = 1 if self.f & CY else 0
            self.f = (self.f & ~CY) | (CY if self.a & 1 else 0)
            self.a = ((self.a >> 1) | (c << 7)) & 0xFF
        elif op == 0x22:                                          # SHLD
            addr = self._fetch16()
            self.write(addr, self.l)
            self.write((addr + 1) & 0xFFFF, self.h)
        elif op == 0x2A:                                          # LHLD
            addr = self._fetch16()
            self.l = self.read(addr)
            self.h = self.read((addr + 1) & 0xFFFF)
        elif op == 0x32:                                          # STA
            self.write(self._fetch16(), self.a)
        elif op == 0x3A:                                          # LDA
            self.a = self.read(self._fetch16())
        elif op == 0x27:                                          # DAA
            corr, cy = 0, self.f & CY
            if (self.a & 0x0F) > 9 or self.f & AC:
                corr |= 0x06
            if (self.a >> 4) > 9 or cy or ((self.a >> 4) >= 9 and (self.a & 0x0F) > 9):
                corr |= 0x60
                cy = CY
            self._add(corr)
            self.f = (self.f & ~CY) | cy
        elif op == 0x2F:                                          # CMA
            self.a ^= 0xFF
        elif op == 0x37:                                          # STC
            self.f |= CY
        elif op == 0x3F:                                          # CMC
            self.f ^= CY
        elif op & 0xC7 == 0xC6:                                   # ALU immediate
            v = self._fetch()
            k = hi & 7
            (self._add, lambda x: self._add(x, 1 if self.f & CY else 0),
             self._sub, lambda x: self._sub(x, 1 if self.f & CY else 0),
             self._ana, self._xra, self._ora, self._cmp)[k](v)
        elif op == 0xC3 or op == 0xCB:                            # JMP
            self.pc = self._fetch16()
        elif op & 0xC7 == 0xC2:                                   # Jcc
            addr = self._fetch16()
            if self._cond(hi & 7):
                self.pc = addr
        elif op == 0xCD or op in (0xDD, 0xED, 0xFD):              # CALL
            addr = self._fetch16()
            self._push(self.pc)
            self.pc = addr
        elif op & 0xC7 == 0xC4:                                   # Ccc
            addr = self._fetch16()
            if self._cond(hi & 7):
                self._push(self.pc)
                self.pc = addr
                self.cycles += 9
        elif op == 0xC9 or op == 0xD9:                            # RET
            self.pc = self._pop()
        elif op & 0xC7 == 0xC0:                                   # Rcc
            if self._cond(hi & 7):
                self.pc = self._pop()
                self.cycles += 6
        elif op & 0xC7 == 0xC7:                                   # RST n
            self._push(self.pc)
            self.pc = op & 0x38
        elif op & 0xCF == 0xC5:                                   # PUSH
            i = (op >> 4) & 3
            self._push((self.a << 8) | self.f if i == 3 else self._get_rp(i))
        elif op & 0xCF == 0xC1:                                   # POP
            i = (op >> 4) & 3
            v = self._pop()
            if i == 3:
                self.a, self.f = (v >> 8) & 0xFF, (v & 0xD5) | 0x02
            else:
                self._set_rp(i, v)
        elif op == 0xD3:                                          # OUT
            self.io_out(self._fetch(), self.a)
        elif op == 0xDB:                                          # IN
            self.a = self.io_in(self._fetch())
        elif op == 0xE3:                                          # XTHL
            v = self._pop()
            self._push(self.hl)
            self.hl = v
        elif op == 0xE9:                                          # PCHL
            self.pc = self.hl
        elif op == 0xEB:                                          # XCHG
            self.h, self.d = self.d, self.h
            self.l, self.e = self.e, self.l
        elif op == 0xF9:                                          # SPHL
            self.sp = self.hl
        elif op == 0xF3:                                          # DI
            self.ie = False
        elif op == 0xFB:                                          # EI
            self.ie = True
        elif op == 0x20:                                          # RIM
            v = self.sid & 0x80
            for bit, which in ((0x01, 5.5), (0x02, 6.5), (0x04, 7.5)):
                if self.masks[which]:
                    v |= bit
            if self.ie:
                v |= 0x08
            for bit, which in ((0x10, 5.5), (0x20, 6.5), (0x40, 7.5)):
                if self.pending[which]:
                    v |= bit
            self.a = v
        elif op == 0x30:                                          # SIM
            if self.a & 0x08:                                     # MSE
                self.masks[5.5] = bool(self.a & 0x01)
                self.masks[6.5] = bool(self.a & 0x02)
                self.masks[7.5] = bool(self.a & 0x04)
            if self.a & 0x10:                                     # R7.5
                self.pending[7.5] = False
            if self.a & 0x40:                                     # SDE
                self.sod = 1 if self.a & 0x80 else 0
        else:
            raise NotImplementedError(
                f"opcode 0x{op:02X} at 0x{(self.pc - 1) & 0xFFFF:04X}")

        return self.cycles - start

    # ------------------------------------------------------------------ debug
    def state(self):
        flags = "".join(c if self.f & b else "."
                        for c, b in zip("SZ-A-P-C", (S, Z, 0, AC, 0, P, 0, CY)))
        return (f"PC={self.pc:04X} SP={self.sp:04X} A={self.a:02X} "
                f"BC={self.b:02X}{self.c:02X} DE={self.d:02X}{self.e:02X} "
                f"HL={self.h:02X}{self.l:02X} F={flags}")

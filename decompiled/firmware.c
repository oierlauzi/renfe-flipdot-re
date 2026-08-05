typedef unsigned char   undefined;

typedef unsigned char    undefined1;


undefined1 DAT_ram_a05a;
char DAT_ram_a00c;
byte DAT_ram_a07e;
undefined1 DAT_ram_a080;
undefined1 DAT_ram_a07d;
undefined1 DAT_ram_a027;
char DAT_ram_a023;
byte DAT_ram_a07c;
undefined1 DAT_ram_a07f;
undefined DAT_ram_a0f1;
char DAT_ram_a012;
undefined DAT_ram_a0ef;
undefined DAT_ram_a0ed;
undefined DAT_ram_a039;
byte DAT_ram_a005;
byte DAT_ram_a006;
undefined1 DAT_ram_a007;
undefined1 DAT_ram_a00d;
char DAT_ram_a009;
undefined1 DAT_ram_a00b;
undefined1 DAT_ram_a00c;
undefined DAT_ram_a000;
undefined1 DAT_ram_a001;
undefined1 DAT_ram_a002;
undefined1 DAT_ram_a012;
char DAT_ram_a008;
undefined1 DAT_ram_a023;
undefined1 DAT_ram_a003;
undefined1 DAT_ram_a004;
undefined1 DAT_ram_e03b;
undefined1 DAT_ram_e07d;
char DAT_ram_e07e;
char DAT_ram_e03c;
char DAT_ram_a057;
undefined1 DAT_ram_a040;
undefined1 DAT_ram_a06f;
byte DAT_ram_a03d;
undefined1 DAT_ram_a041;
undefined DAT_ram_a03b;
byte DAT_ram_a007;
undefined DAT_ram_a00e;
byte DAT_ram_a00d;
byte DAT_ram_a00b;
byte DAT_ram_a014;
byte DAT_ram_a015;
undefined DAT_ram_a016;
byte DAT_ram_a026;
undefined DAT_ram_a018;
undefined1 DAT_ram_a005;
undefined UNK_ram_1f40;
undefined DAT_ram_e000;
undefined DAT_ram_e03c;
byte DAT_ram_e000;
byte DAT_ram_a07b;
char DAT_ram_a040;
char DAT_ram_a03f;
char DAT_ram_a05d;
char DAT_ram_a06f;
byte DAT_ram_a044;
char DAT_ram_a043;
byte DAT_ram_a04f;
char DAT_ram_a03a;
byte DAT_ram_a056;
undefined DAT_ram_a05b;
undefined DAT_ram_a06c;
undefined1 DAT_ram_a045;
char DAT_ram_a053;
char DAT_ram_a055;
char DAT_ram_a03e;
char DAT_ram_a04c;
undefined1 DAT_ram_a024;
undefined1 DAT_ram_a03d;
undefined1 DAT_ram_a06e;
undefined1 DAT_ram_a042;
undefined1 DAT_ram_a050;
undefined1 DAT_ram_a051;
undefined1 DAT_ram_a052;
byte DAT_ram_a054;
undefined DAT_ram_a04d;
byte DAT_ram_a041;
undefined1 DAT_ram_0064;
undefined DAT_ram_a046;
undefined1 DAT_ram_a043;
undefined1 DAT_ram_a044;
undefined1 DAT_ram_a039;
undefined1 DAT_ram_a04f;
undefined1 DAT_ram_a03a;
char DAT_ram_a00a;
char DAT_ram_a025;
char DAT_ram_a03d;
char DAT_ram_a045;
undefined DAT_ram_a010;
undefined1 DAT_ram_a058;
char DAT_ram_a022;
undefined1 DAT_ram_a021;
byte DAT_ram_a009;
byte DAT_ram_a00a;
undefined DAT_ram_a081;
byte DAT_ram_a083;
undefined1 DAT_ram_a022;
char DAT_ram_a021;
byte DAT_ram_a002;
undefined1 DAT_ram_1c0d;
undefined1 DAT_ram_1c5d;
char DAT_ram_a007;
undefined1 DAT_ram_a00a;
char DAT_ram_a058;
byte DAT_ram_a01a;
undefined DAT_ram_a01c;
byte DAT_ram_a01b;
byte DAT_ram_a025;
undefined1 DAT_ram_a01a;
char DAT_ram_a06e;
undefined DAT_ram_a068;
char DAT_ram_a063;
undefined DAT_ram_a05e;
undefined DAT_ram_a06a;
undefined1 DAT_ram_a013;
byte DAT_ram_a024;
undefined1 DAT_ram_a01e;
char DAT_ram_a05a;
undefined1 DAT_ram_a059;
undefined1 DAT_ram_23cf;
undefined1 DAT_ram_a008;
char DAT_ram_a026;
char DAT_ram_a00d;
char DAT_ram_a02a;
char DAT_ram_a02b;
char DAT_ram_a027;
byte DAT_ram_a028;
char DAT_ram_a02f;
char DAT_ram_a02e;
char DAT_ram_a02d;
undefined DAT_ram_a02c;
undefined DAT_ram_e043;
char DAT_ram_a089;
byte DAT_ram_a07d;
char DAT_ram_a088;
byte DAT_ram_a080;
byte DAT_ram_a07f;
byte DAT_ram_a085;
byte DAT_ram_a086;
char DAT_ram_a087;
byte DAT_ram_a08c;
undefined DAT_ram_a08a;
byte DAT_ram_a084;
undefined1 DAT_ram_a006;
undefined DAT_ram_a08d;

void RST0(void)

{
  disableMaskableInterrupts();
  thunk_FUN_ram_034b();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void thunk_FUN_ram_034b(void)

{
  byte in_io_000000a3;
  undefined2 in_BC;
  char cVar1;
  
  disableMaskableInterrupts();
  DAT_ram_a05a = 0;
  _DAT_ram_a0f1 = (undefined *)0x356;
  FUN_ram_1994();
  _DAT_ram_a0f1 = (undefined *)0x359;
  FUN_ram_03f2();
  if ((in_io_000000a3 & 0x20) == 0) {
    DAT_ram_a00c = '\0';
    DAT_ram_a07e = 0x20;
    DAT_ram_a080 = 0x20;
    DAT_ram_a07d = 0xa0;
  }
  if (DAT_ram_a00c != '\0') {
    _DAT_ram_a0f1 = (undefined *)0x379;
    FUN_ram_07da();
  }
  setInterruptMask(0x1a);
  enableMaskableInterrupts();
  if ((in_io_000000a3 & 0x20) == 0) {
    do {
      do {
        do {
          _DAT_ram_a0f1 = (undefined *)0x3b5;
          FUN_ram_199f();
        } while ((DAT_ram_a07e & 0xc0) != 0x40);
      } while ((DAT_ram_a07c & 1) != 0);
      _DAT_ram_a0f1 = &DAT_ram_a039;
      in_BC = CONCAT11(DAT_ram_a07f,(char)in_BC);
      do {
        _DAT_ram_a0ed = 0x3db;
        _DAT_ram_a0ef = in_BC;
        FUN_ram_08da(*_DAT_ram_a0f1,0x61);
        _DAT_ram_a0f1 = _DAT_ram_a0f1 + 1;
        cVar1 = (char)((ushort)_DAT_ram_a0ef >> 8) + -1;
        in_BC = CONCAT11(cVar1,(char)_DAT_ram_a0ef);
      } while (cVar1 != '\0');
      DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x20;
      _DAT_ram_a0f1 = (undefined *)0x3ef;
      FUN_ram_124e();
    } while( true );
  }
  DAT_ram_a027 = 6;
  if (DAT_ram_a023 != '\0') {
    do {
      do {
        _DAT_ram_a0f1 = (undefined *)0x39d;
        FUN_ram_199f();
        _DAT_ram_a0f1 = (undefined *)0x3a0;
        FUN_ram_2a62();
        cVar1 = DAT_ram_a012;
        disableMaskableInterrupts();
        DAT_ram_a012 = 'N';
        enableMaskableInterrupts();
      } while (cVar1 != 'J');
      _DAT_ram_a0f1 = (undefined *)0x39a;
      FUN_ram_123e();
    } while( true );
  }
  do {
    _DAT_ram_a0f1 = (undefined *)0x394;
    FUN_ram_123e();
  } while( true );
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_034b(void)

{
  byte in_io_000000a3;
  undefined2 in_BC;
  char cVar1;
  
  disableMaskableInterrupts();
  DAT_ram_a05a = 0;
  _DAT_ram_a0f1 = (undefined *)0x356;
  FUN_ram_1994();
  _DAT_ram_a0f1 = (undefined *)0x359;
  FUN_ram_03f2();
  if ((in_io_000000a3 & 0x20) == 0) {
    DAT_ram_a00c = '\0';
    DAT_ram_a07e = 0x20;
    DAT_ram_a080 = 0x20;
    DAT_ram_a07d = 0xa0;
  }
  if (DAT_ram_a00c != '\0') {
    _DAT_ram_a0f1 = (undefined *)0x379;
    FUN_ram_07da();
  }
  setInterruptMask(0x1a);
  enableMaskableInterrupts();
  if ((in_io_000000a3 & 0x20) == 0) {
    do {
      do {
        do {
          _DAT_ram_a0f1 = (undefined *)0x3b5;
          FUN_ram_199f();
        } while ((DAT_ram_a07e & 0xc0) != 0x40);
      } while ((DAT_ram_a07c & 1) != 0);
      _DAT_ram_a0f1 = &DAT_ram_a039;
      in_BC = CONCAT11(DAT_ram_a07f,(char)in_BC);
      do {
        _DAT_ram_a0ed = 0x3db;
        _DAT_ram_a0ef = in_BC;
        FUN_ram_08da(*_DAT_ram_a0f1,0x61);
        _DAT_ram_a0f1 = _DAT_ram_a0f1 + 1;
        cVar1 = (char)((ushort)_DAT_ram_a0ef >> 8) + -1;
        in_BC = CONCAT11(cVar1,(char)_DAT_ram_a0ef);
      } while (cVar1 != '\0');
      DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x20;
      _DAT_ram_a0f1 = (undefined *)0x3ef;
      FUN_ram_124e();
    } while( true );
  }
  DAT_ram_a027 = 6;
  if (DAT_ram_a023 != '\0') {
    do {
      do {
        _DAT_ram_a0f1 = (undefined *)0x39d;
        FUN_ram_199f();
        _DAT_ram_a0f1 = (undefined *)0x3a0;
        FUN_ram_2a62();
        cVar1 = DAT_ram_a012;
        disableMaskableInterrupts();
        DAT_ram_a012 = 'N';
        enableMaskableInterrupts();
      } while (cVar1 != 'J');
      _DAT_ram_a0f1 = (undefined *)0x39a;
      FUN_ram_123e();
    } while( true );
  }
  do {
    _DAT_ram_a0f1 = (undefined *)0x394;
    FUN_ram_123e();
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x07ac)

void FUN_ram_03f2(void)

{
  byte in_io_000000a1;
  byte in_io_000000a2;
  byte in_io_000000a3;
  char in_io_000000e7;
  byte bVar1;
  char cVar3;
  short sVar2;
  undefined1 *puVar4;
  
  if (in_io_000000e7 != -1) {
    in_io_000000e7 = -0x20;
  }
  if ((in_io_000000a3 & 0x20) == 0) {
    DAT_ram_a005 = 0x7e;
    puVar4 = &DAT_ram_a000;
    cVar3 = -0x10;
    do {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
      cVar3 = cVar3 + -1;
    } while (cVar3 != '\0');
  }
  else {
    puVar4 = &DAT_ram_a000;
    cVar3 = -0x80;
    do {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
      cVar3 = cVar3 + -1;
    } while (cVar3 != '\0');
    bVar1 = in_io_000000a2 & 0xe0;
    if (bVar1 == 0) {
      DAT_ram_a003 = 0x80;
      DAT_ram_a004 = 0x42;
    }
    else if (bVar1 == 0x20) {
      DAT_ram_a003 = 0x40;
      DAT_ram_a004 = 0x41;
    }
    else if (bVar1 == 0x40) {
      DAT_ram_a003 = 0xa0;
      DAT_ram_a004 = 0x40;
    }
    else if (bVar1 == 0x60) {
      DAT_ram_a003 = 0x50;
      DAT_ram_a004 = 0x40;
    }
    else if (bVar1 == 0x80) {
      DAT_ram_a003 = 0x28;
      DAT_ram_a004 = 0x40;
    }
    else {
      DAT_ram_a003 = 0x14;
      DAT_ram_a004 = 0x40;
    }
    if ((in_io_000000a2 & 0x10) == 0) {
      bVar1 = 0xe;
    }
    else {
      bVar1 = 10;
    }
    if ((in_io_000000a2 & 8) != 0) {
      if ((in_io_000000a2 & 0xc) == 8) {
        bVar1 = bVar1 | 0x10;
      }
      else {
        bVar1 = bVar1 | 0x30;
      }
    }
    if ((in_io_000000a2 & 2) == 0) {
      DAT_ram_a005 = bVar1 | 0x40;
    }
    else {
      DAT_ram_a005 = bVar1 | 0xc0;
    }
    if ((in_io_000000a2 & 1) == 0) {
      DAT_ram_a008 = '\0';
    }
    else {
      DAT_ram_a008 = '\x01';
    }
  }
  DAT_ram_a006 = in_io_000000a1 & 0x1f | 0x60;
  DAT_ram_a007 = 3;
  DAT_ram_a00d = 3;
  bVar1 = in_io_000000a3 & 0x1f;
  if (((bVar1 == 0x1f) || (bVar1 == 0)) || (bVar1 == 1)) goto LAB_ram_060f;
  if (bVar1 == 2) {
    DAT_ram_a009 = '\x12';
  }
  else if (bVar1 == 3) {
    DAT_ram_a009 = '\x11';
  }
  else if (((((bVar1 == 4) || (bVar1 == 5)) || ((bVar1 == 6 || ((bVar1 == 7 || (bVar1 == 8)))))) ||
           (bVar1 == 9)) || (((bVar1 == 10 || (bVar1 == 0xb)) || (bVar1 == 0xc)))) {
LAB_ram_060f:
    DAT_ram_a009 = '\0';
  }
  else if (bVar1 == 0xd) {
LAB_ram_0619:
    DAT_ram_a009 = '\x03';
  }
  else {
    if (((bVar1 == 0xe) || (bVar1 == 0xf)) || ((bVar1 == 0x10 || (bVar1 == 0x11))))
    goto LAB_ram_060f;
    if (bVar1 == 0x12) {
      DAT_ram_a009 = '\0';
    }
    else if (bVar1 == 0x13) {
      DAT_ram_a009 = '\x10';
    }
    else if (bVar1 == 0x14) {
      DAT_ram_a009 = '\f';
    }
    else if (bVar1 == 0x15) {
      DAT_ram_a009 = '\r';
    }
    else {
      if (bVar1 == 0x16) goto LAB_ram_060f;
      if (bVar1 == 0x17) {
        DAT_ram_a009 = '\x0e';
      }
      else if (bVar1 == 0x18) {
        DAT_ram_a009 = '\x04';
      }
      else {
        if (bVar1 == 0x19) goto LAB_ram_060f;
        if (bVar1 == 0x1a) {
          DAT_ram_a009 = '\t';
        }
        else if (bVar1 == 0x1b) {
          DAT_ram_a009 = '\a';
        }
        else if (bVar1 == 0x1c) {
          DAT_ram_a009 = '\x02';
        }
        else {
          if (bVar1 == 0x1d) goto LAB_ram_0619;
          if (bVar1 != 0x1e) goto LAB_ram_060f;
          DAT_ram_a009 = '\n';
        }
      }
    }
  }
  if ((in_io_000000a1 & 0x20) == 0) {
    DAT_ram_a00b = 0x20;
  }
  else {
    DAT_ram_a00b = 0x3f;
  }
  if ((in_io_000000e7 == -1) && ((in_io_000000a1 & 0x40) == 0)) {
    DAT_ram_a00c = 0;
  }
  else {
    DAT_ram_a00c = 1;
  }
  DAT_ram_a000 = 0;
  DAT_ram_a001 = 0;
  DAT_ram_a002 = 0;
  if (DAT_ram_a009 == '\0') goto LAB_ram_072c;
  if ((((DAT_ram_a009 == '\x01') || (DAT_ram_a009 == '\x02')) || (DAT_ram_a009 == '\x03')) ||
     ((DAT_ram_a009 == '\x04' || (DAT_ram_a009 == '\x05')))) {
LAB_ram_0707:
    DAT_ram_a001 = 0;
  }
  else {
    if ((DAT_ram_a009 == '\x06') || (DAT_ram_a009 == '\a')) {
      DAT_ram_a000 = 0x80;
      goto LAB_ram_072c;
    }
    if (DAT_ram_a009 == '\b') {
      DAT_ram_a002 = 0xf6;
      DAT_ram_a001 = 0;
      goto LAB_ram_072c;
    }
    if (DAT_ram_a009 != '\t') {
      if (DAT_ram_a009 == '\n') goto LAB_ram_0707;
      if (DAT_ram_a009 != '\v') {
        if (((DAT_ram_a009 == '\x10') || (DAT_ram_a009 == '\x11')) || (DAT_ram_a009 == '\x12')) {
          DAT_ram_a001 = 0x80;
          DAT_ram_a002 = 0x20;
        }
        goto LAB_ram_072c;
      }
    }
    DAT_ram_a001 = 0xff;
  }
LAB_ram_072c:
  FUN_ram_1b95(DAT_ram_a006);
  FUN_ram_1b95();
  FUN_ram_1b95(DAT_ram_a007);
  FUN_ram_1b95();
  sVar2 = 0x40;
  if ((in_io_000000a1 & 0x80) == 0) {
    do {
      FUN_ram_1b95(0x20);
      FUN_ram_1b95(0x23);
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    DAT_ram_a012 = 0x4a;
  }
  else {
    do {
      FUN_ram_1b69();
      FUN_ram_1b95();
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    DAT_ram_a012 = 0x4e;
  }
  if (DAT_ram_a008 != '\0') {
    DAT_ram_e03b = DAT_ram_a007;
    DAT_ram_e07d = DAT_ram_a007;
    DAT_ram_e03c = DAT_ram_a006 + 1;
    DAT_ram_e07e = DAT_ram_e03c;
  }
  DAT_ram_a023 = 0;
  if (((((DAT_ram_a009 != '\x02') && (DAT_ram_a009 != '\b')) && (DAT_ram_a009 != '\t')) &&
      ((DAT_ram_a009 != '\v' && (DAT_ram_a009 != '\f')))) &&
     ((DAT_ram_a009 != '\r' && (DAT_ram_a009 != '\x0e')))) {
    return;
  }
  DAT_ram_a023 = 0xff;
  return;
}



// WARNING: Removing unreachable block (ram,0x0891)
// WARNING: Removing unreachable block (ram,0x07f0)
// WARNING: Removing unreachable block (ram,0x08ad)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_07da(void)

{
  byte in_io_000000a1;
  byte in_io_000000a2;
  undefined1 uVar1;
  char cVar3;
  short sVar2;
  undefined1 *puVar4;
  
  puVar4 = &DAT_ram_a039;
  cVar3 = '7';
  DAT_ram_a00d = 0;
  do {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
    cVar3 = cVar3 + -1;
  } while (cVar3 != '\0');
  if ((in_io_000000a1 & 0x1f) == 0) {
    DAT_ram_a040 = 1;
    DAT_ram_a06f = 1;
  }
  uVar1 = FUN_ram_1994();
  FUN_ram_1b58(uVar1,2);
  FUN_ram_1994();
  if ((in_io_000000a2 & 1) != 0) {
    DAT_ram_a041 = 0x23;
    DAT_ram_a03d = in_io_000000a2 & 1;
  }
  if ((in_io_000000a1 & 0x80) == 0) {
    sVar2 = 0x40;
    do {
      FUN_ram_1b95(0x20);
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    sVar2 = 0x40;
    do {
      FUN_ram_1b95(0x23);
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    FUN_ram_1994();
    if (DAT_ram_a057 == '\x01') {
      sVar2 = 0x40;
      do {
        FUN_ram_1b95(0);
        sVar2 = sVar2 + -1;
      } while (sVar2 != 0);
      sVar2 = 0x40;
      do {
        FUN_ram_1b95(0xff);
        sVar2 = sVar2 + -1;
      } while (sVar2 != 0);
    }
  }
  _DAT_ram_a03b = 0xe001;
  DAT_ram_a007 = 3;
  return;
}



// WARNING: Removing unreachable block (ram,0x0aad)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

byte FUN_ram_08b7(void)

{
  byte in_io_00000080;
  byte in_io_00000081;
  byte in_io_000000a3;
  byte in_io_000000e2;
  byte in_io_000000e3;
  byte bVar1;
  byte bVar2;
  byte *pbVar3;
  
  bVar1 = DAT_ram_a014;
  if ((in_io_00000081 & 0x78) == 8) {
    in_io_00000080 = 0x23;
LAB_ram_08ee:
    if (in_io_00000080 == DAT_ram_a007) {
      if (DAT_ram_a00d == DAT_ram_a007) {
        return DAT_ram_a00d;
      }
      DAT_ram_a00d = DAT_ram_a007;
      *_DAT_ram_a00e = DAT_ram_a007;
      DAT_ram_a012 = 0x4a;
      return 0x4a;
    }
    if (DAT_ram_a00d == DAT_ram_a007) {
      return DAT_ram_a00d;
    }
    bVar1 = in_io_00000080 & 0x80;
    if (bVar1 == 0) {
      if ((in_io_00000080 & 0x60) == 0) {
        return 0;
      }
      if ((in_io_00000080 & 0x60) == 0x60) {
        DAT_ram_a00d = DAT_ram_a007;
        return DAT_ram_a007;
      }
      *_DAT_ram_a00e = in_io_00000080;
      bVar1 = DAT_ram_a00b;
      if (DAT_ram_a00b == in_io_00000080) {
        *_DAT_ram_a00e = 0x20;
      }
    }
    _DAT_ram_a00e = _DAT_ram_a00e + 1;
    return bVar1;
  }
  if ((in_io_00000081 & 0x78) != 0) {
    DAT_ram_a00d = DAT_ram_a007;
    *_DAT_ram_a00e = DAT_ram_a007;
    DAT_ram_a012 = 0x4e;
    if (DAT_ram_a008 != '\0') {
      DAT_ram_a026 = 0x11;
      DAT_ram_a012 = 0x4a;
    }
    return 0x17;
  }
  if (DAT_ram_a008 == '\x01') {
    if (in_io_00000080 != DAT_ram_a006) {
      if (in_io_00000080 == (byte)(DAT_ram_a006 + 1)) {
        pbVar3 = &DAT_ram_e03c;
        goto LAB_ram_091b;
      }
      if (in_io_00000080 == 0x11) {
        DAT_ram_a026 = in_io_00000080;
        return 0x11;
      }
      if (in_io_00000080 != 0x12) {
        if (in_io_00000080 != 0x66) {
          if (in_io_00000080 != 0x65) {
            if (in_io_00000080 == 100) {
              _DAT_ram_a018 = 0x28;
              DAT_ram_a014 = DAT_ram_a014 & 0x7a | 1;
              return DAT_ram_a014;
            }
            goto LAB_ram_08ee;
          }
          _DAT_ram_a016 = &UNK_ram_1f40;
          goto LAB_ram_09ad;
        }
        goto LAB_ram_09a3;
      }
    }
  }
  else if (in_io_00000080 != DAT_ram_a006) {
    if (in_io_00000080 != 0x7d) {
      if (in_io_00000080 == 0x7c) {
        DAT_ram_a014 = DAT_ram_a014 & 0x7f | 5;
        return DAT_ram_a014;
      }
      if (in_io_00000080 != 0x7f) goto LAB_ram_08ee;
      DAT_ram_a014 = DAT_ram_a014 & 0xfa | 0x80;
      DAT_ram_a015 = DAT_ram_a014;
      bVar2 = in_io_000000a3 & 0x1f;
      if (bVar2 == 0x1f) {
        return 0x1f;
      }
      if (bVar2 == 0) {
        return 0;
      }
      if (bVar2 == 1) {
        return 1;
      }
      if ((bVar2 != 2) && (bVar2 != 3)) {
        if (bVar2 == 4) {
          return 4;
        }
        if (bVar2 == 5) {
          return 5;
        }
        if (bVar2 == 6) {
          return 6;
        }
        if (bVar2 == 7) {
          return 7;
        }
        if (bVar2 == 8) {
          return 8;
        }
        if (bVar2 == 9) {
          return 9;
        }
        if (bVar2 == 10) {
          return 10;
        }
        if (bVar2 == 0xb) {
          return 0xb;
        }
        if (bVar2 == 0xc) {
          return 0xc;
        }
        if (bVar2 != 0xd) {
          if (bVar2 == 0xe) {
            return 0xe;
          }
          if (bVar2 == 0xf) {
            return 0xf;
          }
          if (bVar2 == 0x10) {
            return 0x10;
          }
          if (bVar2 == 0x11) {
            return 0x11;
          }
          if ((((bVar2 != 0x12) && (bVar2 != 0x13)) && (bVar2 != 0x14)) && (bVar2 != 0x15)) {
            if (bVar2 == 0x16) {
              return 0x16;
            }
            if (bVar2 != 0x17) {
              if (bVar2 == 0x18) {
LAB_ram_0ab1:
                bVar2 = in_io_000000e2 & 0x7f;
                if ((bVar1 & 2) != 0) {
                  bVar2 = bVar2 | 0x80;
                }
                return bVar2;
              }
              if (bVar2 == 0x19) {
                return in_io_000000e2 & 0xfc | bVar1 & 2;
              }
              if (bVar2 != 0x1a) {
                if (bVar2 == 0x1b) goto LAB_ram_0ab1;
                if (((bVar2 != 0x1c) && (bVar2 != 0x1d)) && (bVar2 != 0x1e)) {
                  return bVar2;
                }
              }
            }
          }
        }
      }
      return in_io_000000e3 & 0xfc | bVar1 & 2;
    }
LAB_ram_09a3:
    if ((in_io_000000a3 & 0x20) == 0) {
      _DAT_ram_a016 = (undefined *)0x7d0;
    }
    else {
      _DAT_ram_a016 = (undefined *)0x50;
    }
LAB_ram_09ad:
    DAT_ram_a014 = DAT_ram_a014 | 2;
    return DAT_ram_a014;
  }
  pbVar3 = &DAT_ram_e000;
  DAT_ram_a026 = in_io_00000080;
LAB_ram_091b:
  DAT_ram_a00d = in_io_00000080;
  *pbVar3 = in_io_00000080;
  _DAT_ram_a00e = pbVar3 + 1;
  return in_io_00000080;
}



// WARNING: Removing unreachable block (ram,0x0aad)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

byte FUN_ram_08da(byte param_1,byte param_2)

{
  byte in_io_000000a3;
  byte in_io_000000e2;
  byte in_io_000000e3;
  byte bVar1;
  byte bVar2;
  byte in_C;
  byte *in_HL;
  
  bVar1 = DAT_ram_a014;
  if (param_1 == param_2) {
    DAT_ram_a026 = param_1;
    DAT_ram_a00d = param_1;
    DAT_ram_e000 = param_1;
    _DAT_ram_a00e = (byte *)0xe001;
    return param_1;
  }
  if (param_1 == 0x7d) {
    if ((in_io_000000a3 & 0x20) == 0) {
      _DAT_ram_a016 = 2000;
    }
    else {
      _DAT_ram_a016 = 0x50;
    }
    DAT_ram_a014 = DAT_ram_a014 | 2;
    return DAT_ram_a014;
  }
  if (param_1 == 0x7c) {
    DAT_ram_a014 = DAT_ram_a014 & 0x7f | 5;
    return DAT_ram_a014;
  }
  if (param_1 != 0x7f) {
    if (param_1 == in_C) {
      if (DAT_ram_a00d != in_C) {
        DAT_ram_a00d = in_C;
        *in_HL = in_C;
        DAT_ram_a012 = 0x4a;
        return 0x4a;
      }
      return DAT_ram_a00d;
    }
    if (DAT_ram_a00d == in_C) {
      return DAT_ram_a00d;
    }
    bVar1 = param_1 & 0x80;
    if (bVar1 == 0) {
      if ((param_1 & 0x60) == 0) {
        return 0;
      }
      if ((param_1 & 0x60) == 0x60) {
        DAT_ram_a00d = in_C;
        return in_C;
      }
      *in_HL = param_1;
      bVar1 = DAT_ram_a00b;
      if (DAT_ram_a00b == param_1) {
        *in_HL = 0x20;
      }
    }
    _DAT_ram_a00e = in_HL + 1;
    return bVar1;
  }
  DAT_ram_a014 = DAT_ram_a014 & 0xfa | 0x80;
  DAT_ram_a015 = DAT_ram_a014;
  bVar2 = in_io_000000a3 & 0x1f;
  if (bVar2 == 0x1f) {
    return 0x1f;
  }
  if (bVar2 == 0) {
    return 0;
  }
  if (bVar2 == 1) {
    return 1;
  }
  if ((bVar2 != 2) && (bVar2 != 3)) {
    if (bVar2 == 4) {
      return 4;
    }
    if (bVar2 == 5) {
      return 5;
    }
    if (bVar2 == 6) {
      return 6;
    }
    if (bVar2 == 7) {
      return 7;
    }
    if (bVar2 == 8) {
      return 8;
    }
    if (bVar2 == 9) {
      return 9;
    }
    if (bVar2 == 10) {
      return 10;
    }
    if (bVar2 == 0xb) {
      return 0xb;
    }
    if (bVar2 == 0xc) {
      return 0xc;
    }
    if (bVar2 != 0xd) {
      if (bVar2 == 0xe) {
        return 0xe;
      }
      if (bVar2 == 0xf) {
        return 0xf;
      }
      if (bVar2 == 0x10) {
        return 0x10;
      }
      if (bVar2 == 0x11) {
        return 0x11;
      }
      if ((((bVar2 != 0x12) && (bVar2 != 0x13)) && (bVar2 != 0x14)) && (bVar2 != 0x15)) {
        if (bVar2 == 0x16) {
          return 0x16;
        }
        if (bVar2 != 0x17) {
          if (bVar2 == 0x18) {
LAB_ram_0ab1:
            bVar2 = in_io_000000e2 & 0x7f;
            if ((bVar1 & 2) != 0) {
              bVar2 = bVar2 | 0x80;
            }
            return bVar2;
          }
          if (bVar2 == 0x19) {
            return in_io_000000e2 & 0xfc | bVar1 & 2;
          }
          if (bVar2 != 0x1a) {
            if (bVar2 == 0x1b) goto LAB_ram_0ab1;
            if (((bVar2 != 0x1c) && (bVar2 != 0x1d)) && (bVar2 != 0x1e)) {
              return bVar2;
            }
          }
        }
      }
    }
  }
  return in_io_000000e3 & 0xfc | bVar1 & 2;
}



byte FUN_ram_09f3(void)

{
  byte in_io_000000a3;
  byte in_io_000000e2;
  byte in_io_000000e3;
  byte bVar1;
  
  if (DAT_ram_a014 == DAT_ram_a015) {
    return DAT_ram_a014;
  }
  DAT_ram_a015 = DAT_ram_a014;
  bVar1 = in_io_000000a3 & 0x1f;
  if (bVar1 == 0x1f) {
    return 0x1f;
  }
  if (bVar1 == 0) {
    return 0;
  }
  if (bVar1 == 1) {
    return 1;
  }
  if ((bVar1 != 2) && (bVar1 != 3)) {
    if (bVar1 == 4) {
      return 4;
    }
    if (bVar1 == 5) {
      return 5;
    }
    if (bVar1 == 6) {
      return 6;
    }
    if (bVar1 == 7) {
      return 7;
    }
    if (bVar1 == 8) {
      return 8;
    }
    if (bVar1 == 9) {
      return 9;
    }
    if (bVar1 == 10) {
      return 10;
    }
    if (bVar1 == 0xb) {
      return 0xb;
    }
    if (bVar1 == 0xc) {
      return 0xc;
    }
    if (bVar1 != 0xd) {
      if (bVar1 == 0xe) {
        return 0xe;
      }
      if (bVar1 == 0xf) {
        return 0xf;
      }
      if (bVar1 == 0x10) {
        return 0x10;
      }
      if (bVar1 == 0x11) {
        return 0x11;
      }
      if ((((bVar1 != 0x12) && (bVar1 != 0x13)) && (bVar1 != 0x14)) && (bVar1 != 0x15)) {
        if (bVar1 == 0x16) {
          return 0x16;
        }
        if (bVar1 != 0x17) {
          if (bVar1 == 0x18) {
LAB_ram_0ab1:
            bVar1 = in_io_000000e2 & 0x7f;
            if ((DAT_ram_a014 & 2) != 0) {
              bVar1 = bVar1 | 0x80;
            }
            return bVar1;
          }
          if (bVar1 == 0x19) {
            return in_io_000000e2 & 0xfc | DAT_ram_a014 & 3;
          }
          if (bVar1 != 0x1a) {
            if (bVar1 == 0x1b) goto LAB_ram_0ab1;
            if (((bVar1 != 0x1c) && (bVar1 != 0x1d)) && (bVar1 != 0x1e)) {
              return bVar1;
            }
          }
        }
      }
    }
  }
  return in_io_000000e3 & 0xfc | DAT_ram_a014 & 3;
}



// WARNING: Removing unreachable block (ram,0x0b69)
// WARNING: Removing unreachable block (ram,0x0b71)
// WARNING: Removing unreachable block (ram,0x0d0e)
// WARNING: Removing unreachable block (ram,0x0eee)
// WARNING: Removing unreachable block (ram,0x0c86)
// WARNING: Removing unreachable block (ram,0x1c00)
// WARNING: Removing unreachable block (ram,0x1101)
// WARNING: Removing unreachable block (ram,0x0fc4)
// WARNING: Removing unreachable block (ram,0x0e94)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_0acb(void)

{
  byte in_io_00000080;
  byte in_io_00000081;
  byte in_io_000000a1;
  byte in_io_000000a3;
  byte in_io_000000e7;
  byte bVar1;
  byte bVar2;
  undefined1 in_C;
  undefined1 uVar3;
  ushort uVar4;
  char cVar5;
  char cVar9;
  char *pcVar6;
  char *pcVar7;
  undefined2 uVar8;
  short sVar10;
  undefined1 *puVar11;
  char *pcVar12;
  
  DAT_ram_a07b = 0;
  if (DAT_ram_a040 != '\0') {
    if (DAT_ram_a03f != '\0') goto LAB_ram_105e;
    if (DAT_ram_a053 == '\0') {
      if (DAT_ram_a055 != '\0') {
        DAT_ram_a014 = in_io_00000080 & 7;
        DAT_ram_a055 = '\0';
        DAT_ram_a07b = in_io_00000080;
        goto LAB_ram_0c41;
      }
      if (DAT_ram_a039 != '\0') goto LAB_ram_0cb7;
      if (DAT_ram_a03e != '\0') {
        DAT_ram_a041 = in_io_00000080;
        if (in_io_00000080 == 0x12) {
          DAT_ram_a041 = 0x23;
        }
        DAT_ram_a03e = '\0';
        DAT_ram_a03d = 1;
        DAT_ram_a07b = in_io_00000080;
        goto LAB_ram_0c41;
      }
      if (DAT_ram_a04c != '\0') {
        DAT_ram_a07b = in_io_00000080;
        *_DAT_ram_a04d = in_io_00000080 & 0x7f;
        DAT_ram_a04c = DAT_ram_a04c + '\x01';
        _DAT_ram_a04d = _DAT_ram_a04d + 1;
        pcVar6 = "";
        do {
          pcVar12 = &DAT_ram_a046;
          cVar9 = '\x01';
          pcVar7 = pcVar6;
LAB_ram_0ec1:
          if (*pcVar7 == -1) {
            if (DAT_ram_a04c != '\x02') {
              return;
            }
            DAT_ram_a03a = '0';
            DAT_ram_a04f = 2;
            cVar9 = '\0';
            goto LAB_ram_0ed5;
          }
          if (*pcVar7 == *pcVar12) goto code_r0x0ecd;
          pcVar6 = pcVar6 + 6;
        } while( true );
      }
      if (DAT_ram_a05d != '\0') goto LAB_ram_1115;
      DAT_ram_a04f = in_io_00000081 & 0x78;
      if ((DAT_ram_a04f == 8) || (bVar2 = in_io_00000080, DAT_ram_a04f != 0)) {
        DAT_ram_a03a = '0';
        in_io_00000080 = 0x23;
        bVar2 = DAT_ram_a07b;
      }
      DAT_ram_a07b = bVar2;
      uVar4 = CONCAT11(in_io_00000080,in_C);
      goto LAB_ram_0b76;
    }
    DAT_ram_a053 = '\0';
    DAT_ram_a07b = 0;
    if (DAT_ram_a054 != in_io_00000080) {
      DAT_ram_a03a = '0';
    }
LAB_ram_0f31:
    DAT_ram_a042 = 0;
    if (DAT_ram_a00d != '\x7f') {
      FUN_ram_1a7f();
      uVar3 = 6;
      if (DAT_ram_a03a != '\0') {
        uVar3 = 0x15;
      }
      FUN_ram_1a0c(uVar3);
      if ((in_io_000000a1 & 0x10) == 0) {
        bVar2 = in_io_000000a1 & 0x1f | 0x30;
      }
      else {
        bVar2 = in_io_000000a1 & 0xf | 0x40;
      }
      FUN_ram_1a0c(bVar2);
      if (((DAT_ram_a03a != '\0') && (cVar9 = FUN_ram_1a0c(), cVar9 != '0')) && (cVar9 != '2')) {
        FUN_ram_1a0c(DAT_ram_a050);
        FUN_ram_1a0c(DAT_ram_a051);
      }
      FUN_ram_1af5();
      DAT_ram_a03a = '\0';
      DAT_ram_a04f = 0;
      DAT_ram_a050 = 0;
      DAT_ram_a051 = 0;
      DAT_ram_a052 = 0;
      DAT_ram_a054 = 0;
    }
    goto LAB_ram_0c41;
  }
  if (DAT_ram_a03f != '\0') {
LAB_ram_105e:
    DAT_ram_a00d = in_io_00000080 & 0x7f;
    if (DAT_ram_a00d != 0x7f) {
      if ((in_io_000000a1 & 0x10) == 0) {
        bVar2 = in_io_000000a1 & 0xf | 0x30;
      }
      else {
        bVar2 = in_io_000000a1 & 0xf | 0x40;
      }
      if (bVar2 != DAT_ram_a00d) {
        DAT_ram_a03f = 0;
        DAT_ram_a07b = 0;
        return;
      }
    }
    DAT_ram_a03f = 0;
    DAT_ram_a040 = 1;
    DAT_ram_a06f = 1;
    DAT_ram_a07b = 0;
    return;
  }
  if (DAT_ram_a05d != '\0') {
LAB_ram_1115:
    if (DAT_ram_a05d == '\x01') {
      DAT_ram_a056 = in_io_00000080 & 0x7f;
      if (DAT_ram_a056 == 0x33) {
        DAT_ram_a05d = 7;
        DAT_ram_a07b = in_io_00000080;
        return;
      }
      if ((in_io_000000e7 & 0x10) != 0) {
        DAT_ram_a06f = '\0';
      }
      if (DAT_ram_a056 == 0x32) {
        DAT_ram_a05d = 2;
        DAT_ram_a07b = in_io_00000080;
        return;
      }
      DAT_ram_a07b = in_io_00000080;
      if (DAT_ram_a056 == 0x31) {
        if (DAT_ram_a06f != '\0') {
          DAT_ram_a045 = 2;
          _DAT_ram_a05b = 0x40;
          DAT_ram_a05d = 6;
          _DAT_ram_a06c = 0xe084;
          return;
        }
        _DAT_ram_a05b = 0x40;
        DAT_ram_a05d = 6;
        _DAT_ram_a06c = 0xe084;
        return;
      }
LAB_ram_1201:
      DAT_ram_a05d = 0;
      DAT_ram_a056 = 0x30;
      if (DAT_ram_a040 != '\0') {
        DAT_ram_a06f = 1;
        FUN_ram_0f17();
        return;
      }
      DAT_ram_a056 = 0x30;
      DAT_ram_a05d = 0;
      return;
    }
    if (DAT_ram_a05d == '\x02') {
      _DAT_ram_a06c = CONCAT11(in_io_00000080,(char)_DAT_ram_a06c);
      if (DAT_ram_a06f != '\0') {
        DAT_ram_a045 = 2;
        DAT_ram_a05d = 3;
        DAT_ram_a07b = in_io_00000080;
        return;
      }
      DAT_ram_a05d = 3;
      DAT_ram_a07b = in_io_00000080;
      return;
    }
    if (DAT_ram_a05d == '\x03') {
      DAT_ram_a05d = 4;
      _DAT_ram_a06c = CONCAT11((char)((ushort)_DAT_ram_a06c >> 8),in_io_00000080) + -0x1f7c;
      DAT_ram_a07b = in_io_00000080;
      return;
    }
    if (DAT_ram_a05d == '\x04') {
      _DAT_ram_a05b = CONCAT11(in_io_00000080,(char)_DAT_ram_a05b);
      DAT_ram_a05d = 5;
      DAT_ram_a07b = in_io_00000080;
      return;
    }
    if (DAT_ram_a05d == '\x05') {
      _DAT_ram_a05b = CONCAT11((char)((ushort)_DAT_ram_a05b >> 8),in_io_00000080);
      DAT_ram_a05d = 6;
      DAT_ram_a07b = in_io_00000080;
      return;
    }
    if (DAT_ram_a05d == '\x06') {
      DAT_ram_a07b = in_io_00000080;
      if (DAT_ram_a06f != '\0') {
        FUN_ram_1b95(in_io_00000080);
      }
      _DAT_ram_a06c = _DAT_ram_a06c + 1;
      _DAT_ram_a05b = _DAT_ram_a05b + -1;
      if (_DAT_ram_a05b != 0) {
        DAT_ram_a012 = 0x4a;
        return;
      }
      goto LAB_ram_1201;
    }
    DAT_ram_a05d = '\0';
    DAT_ram_a056 = 0x30;
    DAT_ram_a07b = in_io_00000080;
    if (DAT_ram_a06f == '\0') {
      DAT_ram_a056 = 0x30;
      DAT_ram_a05d = 0;
      return;
    }
LAB_ram_0c10:
    FUN_ram_1b95();
    DAT_ram_a012 = 'J';
LAB_ram_0c1b:
    sVar10 = _DAT_ram_a03b + 1;
    DAT_ram_a044 = DAT_ram_a044 + 1;
    if (DAT_ram_a044 == 0x40) {
      DAT_ram_a044 = 0;
      DAT_ram_a043 = DAT_ram_a043 + '\x01';
    }
    goto LAB_ram_0c36;
  }
  if (DAT_ram_a039 != '\0') {
LAB_ram_0cb7:
    if (DAT_ram_a039 == '\x01') {
      DAT_ram_a039 = '\x02';
      if (DAT_ram_a06f == '\0') {
        DAT_ram_a039 = 2;
        DAT_ram_a07b = in_io_00000080;
        return;
      }
      sVar10 = -0x1fff;
      DAT_ram_a043 = (in_io_00000080 & 0x7f) - 0x30;
      for (cVar9 = DAT_ram_a043; DAT_ram_a07b = in_io_00000080, cVar9 != '\0'; cVar9 = cVar9 + -1) {
        sVar10 = sVar10 + 0x40;
      }
    }
    else {
      DAT_ram_a039 = '\0';
      if (DAT_ram_a06f == '\0') {
        DAT_ram_a039 = 0;
        DAT_ram_a07b = in_io_00000080;
        return;
      }
      DAT_ram_a044 = (in_io_00000080 & 0x7f) - 0x30;
      sVar10 = _DAT_ram_a03b + (ushort)DAT_ram_a044;
      DAT_ram_a07b = in_io_00000080;
    }
    goto LAB_ram_0c36;
  }
  if (in_io_00000080 == 4) {
LAB_ram_110d:
    DAT_ram_a05d = '\x01';
    goto LAB_ram_0c41;
  }
  if (in_io_00000080 != 0xb) {
    if (in_io_00000080 == 0x1d) {
      DAT_ram_a03f = 1;
      DAT_ram_a07b = 0;
      return;
    }
    DAT_ram_a07b = 0;
    return;
  }
LAB_ram_0ca7:
  DAT_ram_a039 = '\x01';
  if (DAT_ram_a06f == '\0') {
    DAT_ram_a039 = 1;
    return;
  }
  goto LAB_ram_0c41;
code_r0x0ecd:
  pcVar12 = pcVar12 + 1;
  pcVar7 = pcVar7 + 1;
  cVar9 = cVar9 + -1;
  if (cVar9 != '\0') goto LAB_ram_0ec1;
  cVar9 = *pcVar7;
LAB_ram_0ed5:
  puVar11 = &DAT_ram_a046;
  cVar5 = '\a';
  do {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
    cVar5 = cVar5 + -1;
  } while (cVar5 != '\0');
  uVar4 = CONCAT11(cVar9,cVar9);
LAB_ram_0b76:
  bVar2 = (byte)(uVar4 >> 8);
  if (bVar2 == 0) {
    return;
  }
  if (bVar2 == 0x10) {
    disableMaskableInterrupts();
    DAT_ram_a05a = 0;
    _DAT_ram_a0f1 = (undefined *)0x356;
    FUN_ram_1994();
    _DAT_ram_a0f1 = (undefined *)0x359;
    FUN_ram_03f2();
    if ((in_io_000000a3 & 0x20) == 0) {
      DAT_ram_a00c = '\0';
      DAT_ram_a07e = 0x20;
      DAT_ram_a080 = 0x20;
      DAT_ram_a07d = 0xa0;
    }
    if (DAT_ram_a00c != '\0') {
      _DAT_ram_a0f1 = (undefined *)0x379;
      FUN_ram_07da();
    }
    setInterruptMask(0x1a);
    enableMaskableInterrupts();
    if ((in_io_000000a3 & 0x20) == 0) {
      do {
        do {
          do {
            _DAT_ram_a0f1 = (undefined *)0x3b5;
            FUN_ram_199f();
          } while ((DAT_ram_a07e & 0xc0) != 0x40);
        } while ((DAT_ram_a07c & 1) != 0);
        _DAT_ram_a0f1 = &DAT_ram_a039;
        uVar4 = CONCAT11(DAT_ram_a07f,(char)uVar4);
        do {
          _DAT_ram_a0ed = 0x3db;
          _DAT_ram_a0ef = uVar4;
          FUN_ram_08da(*_DAT_ram_a0f1,0x61);
          _DAT_ram_a0f1 = _DAT_ram_a0f1 + 1;
          cVar9 = (char)(_DAT_ram_a0ef >> 8) + -1;
          uVar4 = CONCAT11(cVar9,(char)_DAT_ram_a0ef);
        } while (cVar9 != '\0');
        DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x20;
        _DAT_ram_a0f1 = (undefined *)0x3ef;
        FUN_ram_124e();
      } while( true );
    }
    DAT_ram_a027 = 6;
    if (DAT_ram_a023 != '\0') {
      do {
        do {
          _DAT_ram_a0f1 = (undefined *)0x39d;
          FUN_ram_199f();
          _DAT_ram_a0f1 = (undefined *)0x3a0;
          FUN_ram_2a62();
          cVar9 = DAT_ram_a012;
          disableMaskableInterrupts();
          DAT_ram_a012 = 'N';
          enableMaskableInterrupts();
        } while (cVar9 != 'J');
        _DAT_ram_a0f1 = (undefined *)0x39a;
        FUN_ram_123e();
      } while( true );
    }
    do {
      _DAT_ram_a0f1 = (undefined *)0x394;
      FUN_ram_123e();
    } while( true );
  }
  if (bVar2 == 0xc) {
    uVar8 = 0x2300;
    if (DAT_ram_a057 != '\0') {
      DAT_ram_a06e = 2;
      uVar8 = 0xff00;
    }
LAB_ram_1003:
    sVar10 = 0x40;
    DAT_ram_a045 = 2;
    do {
      FUN_ram_1b95((char)((ushort)uVar8 >> 8));
      sVar10 = sVar10 + -1;
    } while (sVar10 != 0);
    DAT_ram_a012 = 'J';
LAB_ram_0f1c:
    sVar10 = -0x1fff;
    DAT_ram_a043 = '\0';
    DAT_ram_a044 = 0;
  }
  else {
    if (bVar2 == 0x18) {
LAB_ram_101e:
      uVar8 = 0x2000;
      if (DAT_ram_a057 != '\0') {
        DAT_ram_a06e = 1;
        uVar8 = 0;
      }
      goto LAB_ram_1003;
    }
    if (bVar2 == 0xb) goto LAB_ram_0ca7;
    if (bVar2 == 0x12) goto LAB_ram_0c1b;
    sVar10 = _DAT_ram_a03b;
    if (bVar2 != 0xd) {
      bVar1 = DAT_ram_a044;
      if (bVar2 == 10) {
        for (; bVar1 != 0x40; bVar1 = bVar1 + 1) {
          sVar10 = sVar10 + 1;
        }
        if ((char)(DAT_ram_a043 + '\x01') == '\x01') {
          DAT_ram_a03a = 0x30;
          DAT_ram_a04f = 1;
          return;
        }
        DAT_ram_a044 = 0;
        DAT_ram_a043 = DAT_ram_a043 + '\x01';
        goto LAB_ram_0c36;
      }
      if (bVar2 == 0x11) {
        cVar9 = '@';
        do {
          sVar10 = sVar10 + -1;
          cVar9 = cVar9 + -1;
        } while (cVar9 != '\0');
        if (DAT_ram_a043 == '\0') {
          DAT_ram_a03a = 0x30;
          DAT_ram_a04f = 1;
          return;
        }
        DAT_ram_a043 = DAT_ram_a043 + -1;
        goto LAB_ram_0c36;
      }
      if (bVar2 == 0x13) {
        sVar10 = _DAT_ram_a03b + -1;
        if (DAT_ram_a044 == 0) {
          if (DAT_ram_a043 == '\0') {
            DAT_ram_a03a = 0x30;
            DAT_ram_a04f = 1;
            return;
          }
          DAT_ram_a044 = 0x3f;
          DAT_ram_a043 = DAT_ram_a043 + -1;
        }
        else {
          DAT_ram_a044 = DAT_ram_a044 - 1;
        }
        goto LAB_ram_0c36;
      }
      if (bVar2 == 0x14) {
        sVar10 = _DAT_ram_a03b + 0x40;
        if (DAT_ram_a043 == '\0') {
          DAT_ram_a03a = 0x30;
          DAT_ram_a04f = 1;
          return;
        }
        DAT_ram_a043 = DAT_ram_a043 + '\x01';
        goto LAB_ram_0c36;
      }
      if (bVar2 == 0x17) {
LAB_ram_0daa:
        bVar2 = DAT_ram_a044;
        if (DAT_ram_a043 == '\x01') {
          DAT_ram_a03a = 0x30;
          DAT_ram_a04f = 1;
          return;
        }
        for (; bVar2 != 0x40; bVar2 = bVar2 + 1) {
          FUN_ram_1b95(0x20);
        }
        DAT_ram_a012 = 0x4a;
        return;
      }
      if (bVar2 == 0x19) {
        bVar2 = DAT_ram_a044;
        cVar9 = DAT_ram_a043;
        if (DAT_ram_a043 != '\0') {
          while( true ) {
            for (; bVar2 != 0x40; bVar2 = bVar2 + 1) {
              FUN_ram_1b95(0x20);
            }
            if ((char)(cVar9 + '\x01') == '\x01') break;
            bVar2 = 0;
            cVar9 = cVar9 + '\x01';
          }
          DAT_ram_a012 = 0x4a;
          return;
        }
        goto LAB_ram_0daa;
      }
      if (bVar2 == 9) {
        FUN_ram_1a7f();
        FUN_ram_1a69();
        FUN_ram_1a0c(DAT_ram_a043 + '0');
        FUN_ram_1a0c(DAT_ram_a044 + 0x30);
        FUN_ram_1a69();
        FUN_ram_1af5();
        return;
      }
      if (bVar2 == 0xe) {
        FUN_ram_1a7f();
        FUN_ram_1a69();
        FUN_ram_1b69();
        FUN_ram_1a0c();
        FUN_ram_1a69();
        FUN_ram_1af5();
        return;
      }
      if (bVar2 == 0xf) {
        FUN_ram_1a7f();
        FUN_ram_1a69();
        FUN_ram_1b95(0xff);
        FUN_ram_1bf3();
        FUN_ram_1a69();
        FUN_ram_1af5();
        return;
      }
      if (bVar2 == 0x1c) {
        return;
      }
      if (bVar2 == 0x1f) {
        return;
      }
      if (bVar2 == 0x1a) {
        FUN_ram_1a7f();
        FUN_ram_1a69();
        puVar11 = &DAT_ram_0064;
        cVar9 = '-';
        do {
          FUN_ram_1a0c(*puVar11);
          puVar11 = puVar11 + 1;
          cVar9 = cVar9 + -1;
        } while (cVar9 != '\0');
        FUN_ram_1a69();
        FUN_ram_1af5();
        return;
      }
      if (bVar2 == 0x1b) {
        return;
      }
      if (bVar2 != 2) {
        if (bVar2 == 3) goto LAB_ram_0f31;
        if (bVar2 == 5) {
          DAT_ram_a03e = '\x01';
          goto LAB_ram_0c41;
        }
        if (bVar2 != 6) {
          if (bVar2 == 4) goto LAB_ram_110d;
          if (bVar2 == 7) {
            DAT_ram_a055 = '\x01';
            goto LAB_ram_0c41;
          }
          if (bVar2 == 8) {
            FUN_ram_1a7f();
            FUN_ram_1a0c(8);
            FUN_ram_1a0c(DAT_ram_a024);
            FUN_ram_1af5();
            DAT_ram_a024 = 0;
            return;
          }
          if (bVar2 == 0x1e) {
            if ((in_io_000000a1 & 0x1f) != 0) {
              DAT_ram_a00d = 0;
              DAT_ram_a040 = 0;
              DAT_ram_a06f = 0;
              DAT_ram_a07b = 0;
              return;
            }
            DAT_ram_a07b = 0;
            return;
          }
          if ((uVar4 & 0xf000) == 0) {
            return;
          }
          if ((bVar2 & 0xf0) == 0x10) {
            return;
          }
          goto LAB_ram_0c10;
        }
        DAT_ram_a03d = 0;
        goto LAB_ram_101e;
      }
      DAT_ram_a042 = 1;
      goto LAB_ram_0f1c;
    }
    for (; DAT_ram_a044 != 0; DAT_ram_a044 = DAT_ram_a044 - 1) {
      sVar10 = sVar10 + -1;
    }
    DAT_ram_a044 = 0;
  }
LAB_ram_0c36:
  _DAT_ram_a03b = sVar10;
  if (DAT_ram_a039 != '\0') {
    return;
  }
LAB_ram_0c41:
  bVar2 = (byte)((ushort)_DAT_ram_a03b >> 8);
  if (bVar2 < 0xe1) {
    if (bVar2 != 0xe0) {
      return;
    }
    if ((byte)_DAT_ram_a03b < 0x42) {
      return;
    }
  }
  DAT_ram_a04f = 1;
  DAT_ram_a044 = 0;
  DAT_ram_a043 = 1;
  _DAT_ram_a03b = 0xe041;
  DAT_ram_a03a = 0x30;
  return;
}



// WARNING: Removing unreachable block (ram,0x0c59)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_0f17(void)

{
  DAT_ram_a042 = 1;
  DAT_ram_a043 = 0;
  _DAT_ram_a03b = 0xe001;
  DAT_ram_a044 = 0;
  return;
}



// WARNING: Removing unreachable block (ram,0x0c59)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_0feb(void)

{
  short sVar1;
  undefined2 in_DE;
  
  if (DAT_ram_a057 != '\0') {
    DAT_ram_a06e = 2;
    in_DE = 0xff00;
  }
  sVar1 = 0x40;
  DAT_ram_a045 = 2;
  do {
    FUN_ram_1b95((char)((ushort)in_DE >> 8));
    sVar1 = sVar1 + -1;
  } while (sVar1 != 0);
  DAT_ram_a012 = 0x4a;
  DAT_ram_a043 = 0;
  _DAT_ram_a03b = 0xe001;
  DAT_ram_a044 = 0;
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_123e(void)

{
  byte in_io_000000a1;
  byte in_io_000000a3;
  char in_io_000000e7;
  undefined1 uVar1;
  short sVar2;
  char cVar3;
  char cVar4;
  
  if (DAT_ram_a00c != '\0') goto code_r0x12d0;
  if (DAT_ram_a008 == '\x01') {
    do {
      FUN_ram_13c6();
      FUN_ram_2a62();
      DAT_ram_a021 = 0;
      DAT_ram_a00a = '\0';
      FUN_ram_1445();
      cVar3 = DAT_ram_a009;
      if (DAT_ram_a009 == '\x03') {
        cVar3 = FUN_ram_147e();
      }
      if (cVar3 == '\b') {
        cVar3 = FUN_ram_147e();
      }
      if (cVar3 == '\n') {
        cVar3 = FUN_ram_147e();
      }
      if (cVar3 == '\f') {
        cVar3 = FUN_ram_147e();
      }
      if (cVar3 == '\r') {
        cVar3 = FUN_ram_147e();
      }
      if (cVar3 == '\x0e') {
        FUN_ram_147e();
      }
      DAT_ram_a021 = 1;
      FUN_ram_1445();
    } while (DAT_ram_a023 != -1);
    return;
  }
  while (DAT_ram_a009 != '\0') {
    FUN_ram_13c6();
    cVar3 = '\x01';
    DAT_ram_a00a = '\0';
    DAT_ram_a025 = '\0';
    do {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (cVar3 == '\0') {
        FUN_ram_136a();
      }
      FUN_ram_199f();
      DAT_ram_a025 = DAT_ram_a025 + '\x01';
      DAT_ram_a00a = DAT_ram_a00a + '\x01';
      cVar4 = DAT_ram_a00c == '\0';
      if ((!(bool)cVar4) && (cVar4 = false, DAT_ram_a025 == '@')) {
        cVar3 = '\x01';
        while (cVar3 = cVar3 + -1, cVar3 != '\0') {
          DAT_ram_a00a = DAT_ram_a00a + '\x01';
        }
        cVar4 = true;
        DAT_ram_a025 = '\0';
      }
      FUN_ram_1bc4(DAT_ram_a007);
      cVar3 = '\0';
    } while (cVar4 == '\0');
    cVar3 = DAT_ram_a009;
    if (DAT_ram_a009 == '\n') {
      cVar3 = FUN_ram_2146();
    }
    if (cVar3 == '\f') {
      cVar3 = FUN_ram_25a8();
    }
    if (cVar3 == '\r') {
      cVar3 = FUN_ram_25b1();
    }
    if (cVar3 == '\x0e') {
      cVar3 = FUN_ram_25c3();
    }
    if (cVar3 == '\b') {
      FUN_ram_131b(0x36);
      return;
    }
    if (DAT_ram_a023 == -1) {
      return;
    }
    if ((in_io_000000a3 & 0x20) == 0) {
      return;
    }
    if (DAT_ram_a00c != '\0') {
      if (DAT_ram_a03d == '\x01') {
        uVar1 = 2;
LAB_ram_1338:
        FUN_ram_1348(uVar1);
        FUN_ram_12d0();
        return;
      }
      if (DAT_ram_a03d == '\x02') {
        uVar1 = 1;
        goto LAB_ram_1338;
      }
code_r0x12d0:
      DAT_ram_a045 = '\0';
      DAT_ram_a007 = 3;
      FUN_ram_1b95();
      FUN_ram_1b95();
      if ((DAT_ram_a03d == '\0') && ((in_io_000000a1 & 0x1f) != 0)) {
        do {
        } while (DAT_ram_a040 != '\0');
        DAT_ram_a045 = 1;
        FUN_ram_124e();
        return;
      }
    }
  }
  if (in_io_000000e7 == -1) {
    return;
  }
  if (DAT_ram_a057 == '\x01') {
    FUN_ram_1703();
  }
  _DAT_ram_a010 = 0;
  DAT_ram_a058 = 1;
  uVar1 = DAT_ram_a057 == '\x01';
  do {
    cVar3 = '\x01';
    while( true ) {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (!(bool)uVar1) break;
      cVar3 = cVar3 + -1;
      if (cVar3 == '\0') goto LAB_ram_14d8;
      uVar1 = false;
    }
    DAT_ram_a022 = '\x01';
LAB_ram_14d8:
    if (DAT_ram_a045 != '\0') {
      if (DAT_ram_a045 == '\x02') {
LAB_ram_1517:
        FUN_ram_131b(3);
        return;
      }
      do {
      } while (DAT_ram_a040 != '\0');
    }
    uVar1 = DAT_ram_a022 == '\0';
    if (!(bool)uVar1) {
      FUN_ram_1520();
    }
    if ((bool)uVar1) {
      FUN_ram_15d2();
    }
    DAT_ram_a022 = '\0';
    sVar2 = _DAT_ram_a010 + 1;
    if (((char)((ushort)sVar2 >> 8) == '\0') && ((char)sVar2 == '@')) goto LAB_ram_1517;
    uVar1 = false;
    _DAT_ram_a010 = sVar2;
  } while( true );
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_124e(void)

{
  byte in_io_000000a1;
  byte in_io_000000a3;
  char in_io_000000e7;
  undefined1 uVar1;
  short sVar2;
  char cVar3;
  char cVar4;
  
  while (DAT_ram_a009 != '\0') {
    FUN_ram_13c6();
    cVar3 = '\x01';
    DAT_ram_a00a = '\0';
    DAT_ram_a025 = '\0';
    do {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (cVar3 == '\0') {
        FUN_ram_136a();
      }
      FUN_ram_199f();
      DAT_ram_a025 = DAT_ram_a025 + '\x01';
      DAT_ram_a00a = DAT_ram_a00a + '\x01';
      cVar4 = DAT_ram_a00c == '\0';
      if ((!(bool)cVar4) && (cVar4 = false, DAT_ram_a025 == '@')) {
        cVar3 = '\x01';
        while (cVar3 = cVar3 + -1, cVar3 != '\0') {
          DAT_ram_a00a = DAT_ram_a00a + '\x01';
        }
        cVar4 = true;
        DAT_ram_a025 = '\0';
      }
      FUN_ram_1bc4(DAT_ram_a007);
      cVar3 = '\0';
    } while (cVar4 == '\0');
    cVar3 = DAT_ram_a009;
    if (DAT_ram_a009 == '\n') {
      cVar3 = FUN_ram_2146();
    }
    if (cVar3 == '\f') {
      cVar3 = FUN_ram_25a8();
    }
    if (cVar3 == '\r') {
      cVar3 = FUN_ram_25b1();
    }
    if (cVar3 == '\x0e') {
      cVar3 = FUN_ram_25c3();
    }
    if (cVar3 == '\b') {
      FUN_ram_131b(0x36);
      return;
    }
    if (DAT_ram_a023 == -1) {
      return;
    }
    if ((in_io_000000a3 & 0x20) == 0) {
      return;
    }
    if (DAT_ram_a00c != '\0') {
      if (DAT_ram_a03d == '\x01') {
        uVar1 = 2;
LAB_ram_1338:
        FUN_ram_1348(uVar1);
        FUN_ram_12d0();
        return;
      }
      if (DAT_ram_a03d == '\x02') {
        uVar1 = 1;
        goto LAB_ram_1338;
      }
      DAT_ram_a045 = '\0';
      DAT_ram_a007 = 3;
      FUN_ram_1b95();
      FUN_ram_1b95();
      if ((DAT_ram_a03d == '\0') && ((in_io_000000a1 & 0x1f) != 0)) {
        do {
        } while (DAT_ram_a040 != '\0');
        DAT_ram_a045 = 1;
        FUN_ram_124e();
        return;
      }
    }
  }
  if (in_io_000000e7 == -1) {
    return;
  }
  if (DAT_ram_a057 == '\x01') {
    FUN_ram_1703();
  }
  _DAT_ram_a010 = 0;
  DAT_ram_a058 = 1;
  uVar1 = DAT_ram_a057 == '\x01';
  do {
    cVar3 = '\x01';
    while( true ) {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (!(bool)uVar1) break;
      cVar3 = cVar3 + -1;
      if (cVar3 == '\0') goto LAB_ram_14d8;
      uVar1 = false;
    }
    DAT_ram_a022 = '\x01';
LAB_ram_14d8:
    if (DAT_ram_a045 != '\0') {
      if (DAT_ram_a045 == '\x02') {
LAB_ram_1517:
        FUN_ram_131b(3);
        return;
      }
      do {
      } while (DAT_ram_a040 != '\0');
    }
    uVar1 = DAT_ram_a022 == '\0';
    if (!(bool)uVar1) {
      FUN_ram_1520();
    }
    if ((bool)uVar1) {
      FUN_ram_15d2();
    }
    DAT_ram_a022 = '\0';
    sVar2 = _DAT_ram_a010 + 1;
    if (((char)((ushort)sVar2 >> 8) == '\0') && ((char)sVar2 == '@')) goto LAB_ram_1517;
    uVar1 = false;
    _DAT_ram_a010 = sVar2;
  } while( true );
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_12d0(void)

{
  byte in_io_000000a1;
  byte in_io_000000a3;
  char in_io_000000e7;
  undefined1 uVar1;
  short sVar2;
  char cVar3;
  char cVar4;
  
  while( true ) {
    DAT_ram_a045 = '\0';
    DAT_ram_a007 = 3;
    FUN_ram_1b95();
    FUN_ram_1b95();
    if ((DAT_ram_a03d == '\0') && ((in_io_000000a1 & 0x1f) != 0)) {
      do {
      } while (DAT_ram_a040 != '\0');
      DAT_ram_a045 = 1;
      FUN_ram_124e();
      return;
    }
    do {
      if (DAT_ram_a009 == '\0') {
        if (in_io_000000e7 == -1) {
          return;
        }
        if (DAT_ram_a057 == '\x01') {
          FUN_ram_1703();
        }
        _DAT_ram_a010 = 0;
        DAT_ram_a058 = 1;
        uVar1 = DAT_ram_a057 == '\x01';
        goto LAB_ram_14b1;
      }
      FUN_ram_13c6();
      cVar3 = '\x01';
      DAT_ram_a00a = '\0';
      DAT_ram_a025 = '\0';
      do {
        FUN_ram_1b69();
        FUN_ram_1bc4();
        if (cVar3 == '\0') {
          FUN_ram_136a();
        }
        FUN_ram_199f();
        DAT_ram_a025 = DAT_ram_a025 + '\x01';
        DAT_ram_a00a = DAT_ram_a00a + '\x01';
        cVar4 = DAT_ram_a00c == '\0';
        if ((!(bool)cVar4) && (cVar4 = false, DAT_ram_a025 == '@')) {
          cVar3 = '\x01';
          while (cVar3 = cVar3 + -1, cVar3 != '\0') {
            DAT_ram_a00a = DAT_ram_a00a + '\x01';
          }
          cVar4 = true;
          DAT_ram_a025 = '\0';
        }
        FUN_ram_1bc4(DAT_ram_a007);
        cVar3 = '\0';
      } while (cVar4 == '\0');
      cVar3 = DAT_ram_a009;
      if (DAT_ram_a009 == '\n') {
        cVar3 = FUN_ram_2146();
      }
      if (cVar3 == '\f') {
        cVar3 = FUN_ram_25a8();
      }
      if (cVar3 == '\r') {
        cVar3 = FUN_ram_25b1();
      }
      if (cVar3 == '\x0e') {
        cVar3 = FUN_ram_25c3();
      }
      if (cVar3 == '\b') {
        FUN_ram_131b(0x36);
        return;
      }
      if (DAT_ram_a023 == -1) {
        return;
      }
      if ((in_io_000000a3 & 0x20) == 0) {
        return;
      }
    } while (DAT_ram_a00c == '\0');
    if (DAT_ram_a03d == '\x01') break;
    if (DAT_ram_a03d == '\x02') {
      uVar1 = 1;
LAB_ram_1338:
      FUN_ram_1348(uVar1);
      FUN_ram_12d0();
      return;
    }
  }
  uVar1 = 2;
  goto LAB_ram_1338;
LAB_ram_14b1:
  cVar3 = '\x01';
  while( true ) {
    FUN_ram_1b69();
    FUN_ram_1bc4();
    if (!(bool)uVar1) break;
    cVar3 = cVar3 + -1;
    if (cVar3 == '\0') goto LAB_ram_14d8;
    uVar1 = false;
  }
  DAT_ram_a022 = '\x01';
LAB_ram_14d8:
  if (DAT_ram_a045 != '\0') {
    if (DAT_ram_a045 == '\x02') {
LAB_ram_1517:
      FUN_ram_131b(3);
      return;
    }
    do {
    } while (DAT_ram_a040 != '\0');
  }
  uVar1 = DAT_ram_a022 == '\0';
  if (!(bool)uVar1) {
    FUN_ram_1520();
  }
  if ((bool)uVar1) {
    FUN_ram_15d2();
  }
  DAT_ram_a022 = '\0';
  sVar2 = _DAT_ram_a010 + 1;
  if (((char)((ushort)sVar2 >> 8) == '\0') && ((char)sVar2 == '@')) goto LAB_ram_1517;
  uVar1 = false;
  _DAT_ram_a010 = sVar2;
  goto LAB_ram_14b1;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_131b(void)

{
  byte in_io_000000a1;
  byte in_io_000000a3;
  char in_io_000000e7;
  undefined1 uVar1;
  short sVar2;
  char cVar3;
  char cVar4;
  
  do {
    if (DAT_ram_a023 == -1) {
      return;
    }
    if ((in_io_000000a3 & 0x20) == 0) {
      return;
    }
    if (DAT_ram_a00c != '\0') {
      if (DAT_ram_a03d == '\x01') {
        uVar1 = 2;
LAB_ram_1338:
        FUN_ram_1348(uVar1);
        FUN_ram_12d0();
        return;
      }
      if (DAT_ram_a03d == '\x02') {
        uVar1 = 1;
        goto LAB_ram_1338;
      }
      DAT_ram_a045 = '\0';
      DAT_ram_a007 = 3;
      FUN_ram_1b95();
      FUN_ram_1b95();
      if ((DAT_ram_a03d == '\0') && ((in_io_000000a1 & 0x1f) != 0)) {
        do {
        } while (DAT_ram_a040 != '\0');
        DAT_ram_a045 = 1;
        FUN_ram_124e();
        return;
      }
    }
    if (DAT_ram_a009 == '\0') {
      if (in_io_000000e7 == -1) {
        return;
      }
      if (DAT_ram_a057 == '\x01') {
        FUN_ram_1703();
      }
      _DAT_ram_a010 = 0;
      DAT_ram_a058 = 1;
      uVar1 = DAT_ram_a057 == '\x01';
      break;
    }
    FUN_ram_13c6();
    cVar3 = '\x01';
    DAT_ram_a00a = '\0';
    DAT_ram_a025 = '\0';
    do {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (cVar3 == '\0') {
        FUN_ram_136a();
      }
      FUN_ram_199f();
      DAT_ram_a025 = DAT_ram_a025 + '\x01';
      DAT_ram_a00a = DAT_ram_a00a + '\x01';
      cVar4 = DAT_ram_a00c == '\0';
      if ((!(bool)cVar4) && (cVar4 = false, DAT_ram_a025 == '@')) {
        cVar3 = '\x01';
        while (cVar3 = cVar3 + -1, cVar3 != '\0') {
          DAT_ram_a00a = DAT_ram_a00a + '\x01';
        }
        cVar4 = true;
        DAT_ram_a025 = '\0';
      }
      FUN_ram_1bc4(DAT_ram_a007);
      cVar3 = '\0';
    } while (cVar4 == '\0');
    cVar3 = DAT_ram_a009;
    if (DAT_ram_a009 == '\n') {
      cVar3 = FUN_ram_2146();
    }
    if (cVar3 == '\f') {
      cVar3 = FUN_ram_25a8();
    }
    if (cVar3 == '\r') {
      cVar3 = FUN_ram_25b1();
    }
    if (cVar3 == '\x0e') {
      cVar3 = FUN_ram_25c3();
    }
    if (cVar3 == '\b') {
      FUN_ram_131b(0x36);
      return;
    }
  } while( true );
LAB_ram_14b1:
  cVar3 = '\x01';
  while( true ) {
    FUN_ram_1b69();
    FUN_ram_1bc4();
    if (!(bool)uVar1) break;
    cVar3 = cVar3 + -1;
    if (cVar3 == '\0') goto LAB_ram_14d8;
    uVar1 = false;
  }
  DAT_ram_a022 = '\x01';
LAB_ram_14d8:
  if (DAT_ram_a045 != '\0') {
    if (DAT_ram_a045 == '\x02') {
LAB_ram_1517:
      FUN_ram_131b(3);
      return;
    }
    do {
    } while (DAT_ram_a040 != '\0');
  }
  uVar1 = DAT_ram_a022 == '\0';
  if (!(bool)uVar1) {
    FUN_ram_1520();
  }
  if ((bool)uVar1) {
    FUN_ram_15d2();
  }
  DAT_ram_a022 = '\0';
  sVar2 = _DAT_ram_a010 + 1;
  if (((char)((ushort)sVar2 >> 8) == '\0') && ((char)sVar2 == '@')) goto LAB_ram_1517;
  uVar1 = false;
  _DAT_ram_a010 = sVar2;
  goto LAB_ram_14b1;
}



// WARNING: Removing unreachable block (ram,0x0c59)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1348(void)

{
  byte in_io_000000a2;
  undefined2 in_AF;
  char cVar1;
  short sVar2;
  undefined1 uVar3;
  
  cVar1 = (char)((ushort)in_AF >> 8);
  DAT_ram_a03d = cVar1;
  FUN_ram_1b58();
  if ((in_io_000000a2 & 1) != 0) {
    DAT_ram_a041 = 0x23;
  }
  if (cVar1 != '\x01') {
    uVar3 = 0x20;
    if (DAT_ram_a057 != '\0') {
      DAT_ram_a06e = 1;
      uVar3 = 0;
    }
    sVar2 = 0x40;
    DAT_ram_a045 = 2;
    do {
      FUN_ram_1b95(uVar3);
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    DAT_ram_a012 = 0x4a;
    DAT_ram_a043 = 0;
    _DAT_ram_a03b = 0xe001;
    DAT_ram_a044 = 0;
    return;
  }
  FUN_ram_0feb(DAT_ram_a041);
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

byte FUN_ram_136a(void)

{
  byte in_io_000000e2;
  byte in_io_000000e3;
  undefined1 uVar1;
  char cVar2;
  char cVar3;
  byte bVar4;
  byte bVar5;
  char cVar6;
  undefined2 in_HL;
  undefined1 *puVar7;
  
  FUN_ram_1b95();
  if (DAT_ram_a009 != 1) {
    if (DAT_ram_a009 == 2) {
      cVar2 = '\b';
      bVar4 = DAT_ram_a00a;
      if (DAT_ram_a00a == 8) {
        bVar4 = 8;
        do {
          FUN_ram_1b69(bVar4);
          bVar4 = FUN_ram_190d(0);
          bVar4 = FUN_ram_190d(bVar4 | 0xc0);
          bVar4 = bVar4 & 0x80;
          cVar2 = cVar2 + -1;
        } while (cVar2 != '\0');
        bVar4 = FUN_ram_190d();
        FUN_ram_1916(bVar4 | 0x20);
        bVar4 = 0;
      }
      return bVar4;
    }
    if (DAT_ram_a009 != 3) {
      if ((DAT_ram_a009 == 4) || (DAT_ram_a009 == 5)) {
        cVar2 = FUN_ram_1b69();
        do {
          puVar7 = &DAT_ram_1c5d;
          cVar3 = '\x10';
          do {
            if (cVar3 == cVar2) {
              if (DAT_ram_a009 != 5) {
                cVar2 = '\x05';
                do {
                  in_io_000000e2 = FUN_ram_1939(in_io_000000e2 | ~DAT_ram_a00a & 0x3f | 0x40);
                  in_io_000000e2 = in_io_000000e2 & 0x80;
                  cVar2 = cVar2 + -1;
                } while (cVar2 != '\0');
                return in_io_000000e2;
              }
              _DAT_ram_a010 = puVar7;
              FUN_ram_2545();
              FUN_ram_254b();
              FUN_ram_2553();
              FUN_ram_2560();
              FUN_ram_2567();
              return 0;
            }
            puVar7 = puVar7 + 5;
            cVar3 = cVar3 + '\x01';
          } while (cVar3 != -0x80);
          cVar2 = '#';
        } while( true );
      }
      if ((DAT_ram_a009 == 6) || (DAT_ram_a009 == 7)) {
        bVar4 = FUN_ram_1b69();
        do {
          bVar5 = 0x10;
          do {
            if (bVar5 == bVar4) {
              cVar2 = '\x05';
              bVar4 = 1;
              do {
                FUN_ram_22bb(bVar5);
                FUN_ram_22bb();
                bVar5 = bVar4 << 1 | bVar4 >> 7;
                cVar2 = cVar2 + -1;
                bVar4 = bVar5;
              } while (cVar2 != '\0');
              return bVar5;
            }
            bVar5 = bVar5 + 1;
          } while (bVar5 != 0x80);
          bVar4 = 0x23;
        } while( true );
      }
      if (DAT_ram_a009 != 8) {
        if (DAT_ram_a009 == 9) {
          FUN_ram_2378();
          cVar2 = '\x03';
          do {
            uVar1 = FUN_ram_1b69();
            FUN_ram_1b95(uVar1);
            FUN_ram_2330();
            cVar2 = cVar2 + -1;
          } while (cVar2 != '\0');
          FUN_ram_23a4();
          bVar4 = FUN_ram_239b();
          return bVar4;
        }
        if (DAT_ram_a009 == 10) {
          FUN_ram_1b69();
          bVar4 = 0x10;
          if (DAT_ram_a021 == '\x01') {
            bVar4 = 0x20;
          }
          FUN_ram_190d(~DAT_ram_a00a & 0xf | bVar4);
          FUN_ram_190d();
          FUN_ram_190d();
          DAT_ram_a022 = 1;
          return 1;
        }
        if (DAT_ram_a009 == 0xb) {
          return 0xb;
        }
        if (DAT_ram_a009 != 0xc) {
          if (DAT_ram_a009 == 0xd) {
            DAT_ram_a022 = 1;
            return 1;
          }
          if (DAT_ram_a009 == 0xe) {
            _DAT_ram_a081 = in_HL;
            DAT_ram_a083 = DAT_ram_a00a;
            DAT_ram_a022 = 1;
            return 1;
          }
          if (DAT_ram_a009 != 0x10) {
            if (DAT_ram_a009 == 0x11) {
              cVar2 = FUN_ram_1b69();
              do {
                cVar3 = '\0';
                do {
                  if (cVar3 == cVar2) {
                    cVar2 = '%';
                    do {
                      cVar2 = cVar2 + -1;
                    } while (cVar2 != '\0');
                    cVar2 = '\x05';
                    do {
                      cVar3 = '\a';
                      do {
                        cVar3 = cVar3 + -1;
                      } while (cVar3 != '\0');
                      cVar2 = cVar2 + -1;
                    } while (cVar2 != '\0');
                    return 0x80;
                  }
                  cVar3 = cVar3 + '\x01';
                } while (cVar3 != -1);
                cVar2 = '\0';
              } while( true );
            }
            if (DAT_ram_a009 != 0x12) {
              return DAT_ram_a009;
            }
          }
          cVar2 = FUN_ram_1b69();
          do {
            puVar7 = &DAT_ram_1c0d;
            cVar3 = '\0';
            do {
              if (cVar3 == cVar2) {
                bVar4 = ~DAT_ram_a00a;
                bVar5 = in_io_000000e3 & 3 | 4;
                FUN_ram_190d(bVar5);
                FUN_ram_190d();
                cVar2 = '\x05';
                do {
                  bVar5 = FUN_ram_190d(bVar5 & 0xf7 | 0x10);
                  bVar5 = FUN_ram_190d(bVar5 & 0x2f);
                  bVar5 = bVar5 | 0x20;
                  if (DAT_ram_a009 == 0x12) {
                    FUN_ram_190d(*puVar7);
                    bVar4 = 0x81;
                    FUN_ram_190d(0x81);
                    FUN_ram_190d(0);
                  }
                  bVar4 = FUN_ram_1981(bVar4 & 0x7f);
                  bVar4 = bVar4 | 0x80;
                  FUN_ram_190d(bVar4);
                  if (DAT_ram_a009 == 0x12) {
                    bVar4 = ~DAT_ram_a00a;
                    bVar5 = bVar5 | 8;
                    FUN_ram_190d(bVar5);
                  }
                  bVar5 = FUN_ram_190d(bVar5 | 0x10);
                  bVar5 = bVar5 & 0x2f;
                  FUN_ram_190d(bVar5);
                  FUN_ram_190d();
                  FUN_ram_196e(*puVar7);
                  FUN_ram_190d(0);
                  puVar7 = puVar7 + 1;
                  cVar2 = cVar2 + -1;
                } while (cVar2 != '\0');
                return 0x81;
              }
              puVar7 = puVar7 + 5;
              cVar3 = cVar3 + '\x01';
            } while (cVar3 != -1);
            cVar2 = '\0';
          } while( true );
        }
        DAT_ram_a022 = 1;
        return 1;
      }
      cVar2 = FUN_ram_1b69();
      do {
        cVar3 = '\x10';
        do {
          if (cVar3 == cVar2) {
            cVar2 = '\x01';
            if (DAT_ram_a00a == 0) {
              DAT_ram_a002 = 0x35;
            }
            else {
              if (DAT_ram_a00a != 0) {
                DAT_ram_a002 = DAT_ram_a002 & 0x2e | 0x12;
                goto LAB_ram_2240;
              }
              DAT_ram_a002 = 0x2e;
            }
            cVar2 = '\0';
LAB_ram_2240:
            cVar3 = '\x05';
            do {
              if (cVar2 != '\0') {
                DAT_ram_a002 = 0x3f;
              }
              cVar6 = '\x14';
              do {
                cVar6 = cVar6 + -1;
              } while (cVar6 != '\0');
              cVar6 = '\x14';
              do {
                cVar6 = cVar6 + -1;
              } while (cVar6 != '\0');
              bVar4 = FUN_ram_1939(DAT_ram_a002 & 0x2b | 0x12);
              cVar2 = cVar2 + '\x01';
              cVar3 = cVar3 + -1;
            } while (cVar3 != '\0');
            return bVar4;
          }
          cVar3 = cVar3 + '\x01';
        } while (cVar3 != -0x80);
        cVar2 = '#';
      } while( true );
    }
  }
  FUN_ram_1b69();
  bVar4 = 0x80;
  if (DAT_ram_a021 == '\x01') {
    bVar4 = 0x40;
  }
  FUN_ram_190d(~DAT_ram_a00a & 0x3f | bVar4);
  FUN_ram_190d();
  bVar4 = FUN_ram_190d();
  return bVar4 & 0x3f;
}



void FUN_ram_13c6(void)

{
  byte in_io_000000a3;
  short sVar1;
  
  if (DAT_ram_a009 != '\b') {
    if (DAT_ram_a009 == '\x12') {
      return;
    }
    if (((in_io_000000a3 & 0x20) != 0) && (DAT_ram_a023 != -1)) {
      return;
    }
  }
  sVar1 = 0x40;
  do {
    FUN_ram_1b95(0);
    sVar1 = sVar1 + -1;
  } while (sVar1 != 0);
  return;
}



char FUN_ram_1445(void)

{
  char cVar1;
  char *in_DE;
  char *in_HL;
  
  do {
    if (*in_HL != *in_DE) {
      FUN_ram_136a();
    }
    FUN_ram_199f();
    DAT_ram_a00a = DAT_ram_a00a + '\x01';
    in_HL = in_HL + 1;
    in_DE = in_DE + 1;
  } while (DAT_ram_a007 != *in_HL);
  cVar1 = DAT_ram_a009;
  if (DAT_ram_a009 == '\n') {
    cVar1 = FUN_ram_2146();
  }
  if (cVar1 == '\f') {
    cVar1 = FUN_ram_25a8();
  }
  if (cVar1 == '\r') {
    cVar1 = FUN_ram_25b1();
  }
  if (cVar1 == '\x0e') {
    cVar1 = FUN_ram_25c3();
  }
  if (cVar1 != '\b') {
    return cVar1;
  }
  return '6';
}



undefined1 FUN_ram_147e(void)

{
  undefined2 in_AF;
  
  DAT_ram_a00a = 0;
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined1 FUN_ram_1520(void)

{
  undefined2 in_AF;
  undefined1 uVar1;
  char cVar2;
  
  DAT_ram_a00a = '\0';
  FUN_ram_16dc();
  do {
    DAT_ram_a025 = '\0';
    FUN_ram_15ff();
    do {
      uVar1 = FUN_ram_1b69();
      if (DAT_ram_a00a == '\0') {
        FUN_ram_1b95(uVar1);
      }
      if (DAT_ram_a057 == '\0') {
        cVar2 = '\x04';
        do {
          cVar2 = cVar2 + -1;
        } while (cVar2 != '\0');
        FUN_ram_1630();
      }
      else {
        FUN_ram_1631(uVar1);
      }
      DAT_ram_a025 = DAT_ram_a025 + '\x01';
    } while (DAT_ram_a025 != '\x01');
    FUN_ram_1675();
    if (DAT_ram_a057 != '\0') goto LAB_ram_15c6;
    DAT_ram_a00a = DAT_ram_a00a + '\x01';
  } while (DAT_ram_a00a != '\x05');
  FUN_ram_16dc();
LAB_ram_15c6:
  return (char)((ushort)in_AF >> 8);
}



undefined1 FUN_ram_15d2(void)

{
  byte in_io_000000e7;
  undefined2 in_AF;
  undefined1 uVar1;
  char cVar2;
  
  uVar1 = (undefined1)((ushort)in_AF >> 8);
  if (((in_io_000000e7 & 0xf) != 5) && ((in_io_000000e7 & 0xf) != 6)) {
    return uVar1;
  }
  cVar2 = '\n';
  DAT_ram_a00a = 0;
  if (DAT_ram_a057 != '\0') {
    cVar2 = '\x02';
  }
  do {
    FUN_ram_16b5();
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  return uVar1;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_15ff(void)

{
  byte bVar1;
  
  bVar1 = 0x2b;
  if (DAT_ram_a058 == '\x01') {
    DAT_ram_a058 = '\0';
    bVar1 = 0x2a;
  }
  DAT_ram_a01a = bVar1 & 0x2d | 7;
  _DAT_ram_a01c = 5;
  DAT_ram_a01b = DAT_ram_a01b | 0x81;
  return;
}



undefined1 FUN_ram_1630(void)

{
  byte in_io_000000e7;
  byte bVar1;
  undefined1 uVar2;
  
  if (((in_io_000000e7 & 0xf) != 6) && ((in_io_000000e7 & 0xf) != 2)) {
    bVar1 = FUN_ram_1b4d(~DAT_ram_a025 & 0x7f | 0x80);
    FUN_ram_1b4d(bVar1 & 0x7f);
    return 0;
  }
  uVar2 = FUN_ram_1645(DAT_ram_a025 * '\x02');
  return uVar2;
}



undefined1 FUN_ram_1631(void)

{
  byte in_io_000000e7;
  byte bVar1;
  undefined1 uVar2;
  
  if (((in_io_000000e7 & 0xf) != 6) && ((in_io_000000e7 & 0xf) != 2)) {
    bVar1 = FUN_ram_1b4d(~DAT_ram_a025 & 0x7f | 0x80);
    FUN_ram_1b4d(bVar1 & 0x7f);
    return 0;
  }
  uVar2 = FUN_ram_1645(DAT_ram_a025 * '\x02');
  return uVar2;
}



undefined1 FUN_ram_1645(byte param_1)

{
  byte bVar1;
  
  bVar1 = FUN_ram_1b4d(~param_1 & 0x7f | 0x80);
  FUN_ram_1b4d(bVar1 & 0x7f);
  return 0;
}



// WARNING: Removing unreachable block (ram,0x16a9)
// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1675(void)

{
  do {
  } while ((DAT_ram_a01b & 1) != 0);
  FUN_ram_1b4f(0x2b);
  FUN_ram_1b4f(0x2b);
  do {
  } while( true );
}



undefined1 FUN_ram_16b5(void)

{
  if (DAT_ram_a058 == '\x01') {
    DAT_ram_a058 = '\0';
  }
  DAT_ram_a00a = DAT_ram_a00a + '\x01';
  return 0xb;
}



void FUN_ram_16dc(void)

{
  byte in_io_000000e7;
  char cVar1;
  char cVar2;
  undefined1 in_E;
  char cVar3;
  
  if ((in_io_000000e7 & 0xf) != 2) {
    return;
  }
  cVar2 = '\x01';
  cVar1 = '\0';
  cVar3 = '\x01';
  DAT_ram_a058 = in_E;
  do {
    FUN_ram_1645(cVar3);
    cVar3 = cVar3 + '\x02';
    cVar1 = cVar1 + '\x01';
  } while (cVar1 != cVar2);
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1703(void)

{
  undefined2 uVar1;
  char cVar3;
  short sVar2;
  short sVar4;
  char cVar5;
  
  if (DAT_ram_a06e != '\0') {
    sVar4 = 0x20;
    if (DAT_ram_a06e != '\x01') {
      sVar4 = 0x23;
    }
    sVar4 = sVar4 << 8;
    DAT_ram_a06e = 0;
    sVar2 = 0x40;
    do {
      FUN_ram_1b95((char)((ushort)sVar4 >> 8));
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    sVar2 = 0x40;
    do {
      FUN_ram_1b95((char)((ushort)sVar4 >> 8));
      sVar2 = sVar2 + -1;
    } while (sVar2 != 0);
    return;
  }
  _DAT_ram_a068 = -0x1f7c;
  cVar5 = '@';
  while( true ) {
    uVar1 = CONCAT11(1,cVar5);
    cVar5 = true;
    DAT_ram_a063 = '\0';
    _DAT_ram_a010 = _DAT_ram_a068;
    while( true ) {
      FUN_ram_1b69();
      FUN_ram_1bc4();
      if (cVar5 == '\0') {
        FUN_ram_177f();
      }
      cVar5 = (char)uVar1;
      cVar3 = (char)((ushort)uVar1 >> 8) + -1;
      uVar1 = CONCAT11(cVar3,cVar5);
      if (cVar3 == '\0') break;
      sVar4 = _DAT_ram_a068 + 0x40;
      if (DAT_ram_a063 == '\x03') {
        sVar4 = _DAT_ram_a068 + 0x80;
      }
      DAT_ram_a063 = DAT_ram_a063 + '\x01';
      cVar5 = DAT_ram_a063 == '\x04';
      _DAT_ram_a068 = sVar4;
      if ((bool)cVar5) {
        DAT_ram_a063 = '\0';
      }
    }
    cVar5 = cVar5 + -1;
    if (cVar5 == '\0') break;
    cVar3 = '\a';
    do {
      _DAT_ram_a010 = _DAT_ram_a010 + 1;
      cVar3 = cVar3 + -1;
      _DAT_ram_a068 = _DAT_ram_a010;
    } while (cVar3 != '\0');
  }
  return;
}



void FUN_ram_177f(void)

{
  byte bVar1;
  char cVar2;
  char *pcVar3;
  byte *pbVar4;
  char *pcVar5;
  
  bVar1 = FUN_ram_1b95();
  pcVar5 = &DAT_ram_1c0d + (ushort)bVar1 + (ushort)bVar1 * 4;
  if (DAT_ram_a063 != '\0') {
    if (DAT_ram_a063 == '\x01') {
      pcVar3 = &DAT_ram_a05e;
      FUN_ram_1824();
      cVar2 = '\x05';
      do {
        *pcVar3 = *pcVar5 << 2;
        pcVar5 = pcVar5 + 1;
        pcVar3 = pcVar3 + 1;
        cVar2 = cVar2 + -1;
      } while (cVar2 != '\0');
      pcVar5 = &DAT_ram_a05e;
    }
    else if (DAT_ram_a063 == '\x02') {
      pbVar4 = &DAT_ram_a05e;
      FUN_ram_1855();
      cVar2 = '\x05';
      do {
        *pbVar4 = *pbVar4 | *pcVar5 << 4;
        pcVar5 = pcVar5 + 1;
        pbVar4 = pbVar4 + 1;
        cVar2 = cVar2 + -1;
      } while (cVar2 != '\0');
      pcVar5 = &DAT_ram_a05e;
    }
    else {
      pbVar4 = &DAT_ram_a05e;
      FUN_ram_1893();
      cVar2 = '\x05';
      do {
        *pbVar4 = *pbVar4 | *pcVar5 << 6;
        pcVar5 = pcVar5 + 1;
        pbVar4 = pbVar4 + 1;
        cVar2 = cVar2 + -1;
      } while (cVar2 != '\0');
      pcVar5 = &DAT_ram_a05e;
    }
  }
  cVar2 = '\x05';
  do {
    FUN_ram_1b95(*pcVar5);
    pcVar5 = pcVar5 + 1;
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  FUN_ram_1b95(0);
  FUN_ram_1b95();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1824(void)

{
  byte bVar1;
  char cVar2;
  byte *in_HL;
  byte bVar3;
  
  _DAT_ram_a06a = _DAT_ram_a068;
  cVar2 = '\x05';
  do {
    bVar3 = (*in_HL & 0x7f) >> 6;
    _DAT_ram_a06a = _DAT_ram_a06a + 1;
    bVar1 = FUN_ram_1b69(bVar3);
    FUN_ram_1b95(bVar1 & 0xf0 | bVar3);
    in_HL = in_HL + 1;
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1855(void)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  byte *in_DE;
  byte *in_HL;
  
  _DAT_ram_a06a = _DAT_ram_a068;
  cVar3 = '\x05';
  do {
    bVar1 = *in_HL;
    bVar1 = (((bVar1 >> 7) << 1 | (bVar1 & 0x7f) >> 6) << 1 | (bVar1 << 1 & 0x7f) >> 6) << 1 |
            (bVar1 << 1 & 0x3f) >> 5;
    _DAT_ram_a06a = _DAT_ram_a06a + 1;
    bVar2 = FUN_ram_1b69();
    *in_DE = bVar2 & 1;
    bVar2 = FUN_ram_1b69();
    FUN_ram_1b95(bVar2 & 0xc0 | bVar1);
    in_HL = in_HL + 1;
    in_DE = in_DE + 1;
    cVar3 = cVar3 + -1;
  } while (cVar3 != '\0');
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_1893(void)

{
  byte bVar1;
  char cVar2;
  byte *in_DE;
  byte *in_HL;
  
  _DAT_ram_a06a = _DAT_ram_a068;
  cVar2 = '\x05';
  do {
    bVar1 = FUN_ram_1b69();
    *in_DE = bVar1 & 7;
    in_DE = in_DE + 1;
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  cVar2 = '\x05';
  do {
    _DAT_ram_a06a = _DAT_ram_a06a + 1;
    FUN_ram_1b95(*in_HL >> 2 & 0x1f);
    in_HL = in_HL + 1;
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  FUN_ram_1b95(0);
  FUN_ram_1b95();
  return;
}



undefined1 FUN_ram_190d(void)

{
  undefined2 in_AF;
  char cVar1;
  
  cVar1 = '\x0e';
  do {
    cVar1 = cVar1 + -1;
  } while (cVar1 != '\0');
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Removing unreachable block (ram,0x192e)

void FUN_ram_1916(void)

{
                    // WARNING: Do nothing block with infinite loop
  do {
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x192e)

void FUN_ram_1931(void)

{
                    // WARNING: Do nothing block with infinite loop
  do {
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x196b)

void FUN_ram_1939(void)

{
                    // WARNING: Do nothing block with infinite loop
  do {
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x196b)

void FUN_ram_196e(void)

{
                    // WARNING: Do nothing block with infinite loop
  do {
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x196b)

void FUN_ram_1981(void)

{
                    // WARNING: Do nothing block with infinite loop
  do {
  } while( true );
}



undefined1 FUN_ram_1994(void)

{
  undefined2 in_AF;
  
  setInterruptMask(0xc0);
  setInterruptMask(0x40);
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined1 FUN_ram_199f(void)

{
  undefined2 in_AF;
  byte bVar1;
  
  if (DAT_ram_a00c == '\0') {
    bVar1 = DAT_ram_a014;
    if (DAT_ram_a014 != 0) {
      if (DAT_ram_a014 == 0x80) goto LAB_ram_19f6;
      if (((DAT_ram_a014 & 5) == 1) &&
         ((char)((ushort)_DAT_ram_a018 >> 8) == '\0' && (char)_DAT_ram_a018 == '\0')) {
        bVar1 = DAT_ram_a014 & 0xfe;
      }
      if (((bVar1 & 2) != 0) &&
         ((char)((ushort)_DAT_ram_a016 >> 8) == '\0' && (char)_DAT_ram_a016 == '\0')) {
        bVar1 = bVar1 & 0xfd;
      }
      DAT_ram_a014 = bVar1;
      FUN_ram_09f3();
    }
    DAT_ram_a013 = (bVar1 & 3) != 0;
  }
  else {
LAB_ram_19f6:
    FUN_ram_09f3();
  }
  return (char)((ushort)in_AF >> 8);
}



undefined1 FUN_ram_19fc(void)

{
  byte in_io_000000de;
  undefined2 in_AF;
  
  DAT_ram_a024 = DAT_ram_a024 | ~in_io_000000de;
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Removing unreachable block (ram,0x1a24)
// WARNING: Removing unreachable block (ram,0x1a33)
// WARNING: Removing unreachable block (ram,0x1a14)

undefined1 FUN_ram_1a0c(void)

{
  byte in_io_00000081;
  undefined2 in_AF;
  
  do {
  } while ((in_io_00000081 & 1) == 0);
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Removing unreachable block (ram,0x1a24)
// WARNING: Removing unreachable block (ram,0x1a33)

undefined1 FUN_ram_1a1a(void)

{
  byte in_io_00000081;
  undefined2 in_AF;
  
  do {
  } while ((in_io_00000081 & 1) == 0);
  return (char)((ushort)in_AF >> 8);
}



byte FUN_ram_1a46(void)

{
  byte bVar1;
  byte bVar2;
  undefined2 in_AF;
  byte bVar3;
  
  bVar3 = (byte)((ushort)in_AF >> 8);
  bVar1 = (byte)(bVar3 >> 1 | bVar3 << 7) >> 1;
  bVar2 = (byte)(bVar1 | (bVar3 >> 1) << 7) >> 1;
  FUN_ram_1a60((byte)(bVar2 | bVar1 << 7) >> 1 | bVar2 << 7);
  FUN_ram_1a1a();
  FUN_ram_1a60();
  FUN_ram_1a1a();
  FUN_ram_1a1a(0x20);
  return bVar3;
}



undefined1 FUN_ram_1a60(byte param_1)

{
  byte bVar1;
  undefined1 uVar2;
  
  param_1 = param_1 & 0xf;
  bVar1 = BCDadjust(param_1 + 0x90);
  hasEvenParity(bVar1);
  uVar2 = BCDadjust(bVar1 + 0x40 +
                    (CARRY1(bVar1,0x6f < param_1) || 0xbf < (byte)(bVar1 + (0x6f < param_1))));
  hasEvenParity(uVar2);
  return uVar2;
}



void FUN_ram_1a69(void)

{
  char *in_HL;
  
  for (; *in_HL != -1; in_HL = in_HL + 1) {
    FUN_ram_1a0c();
  }
  return;
}



void FUN_ram_1a74(void)

{
  char *in_HL;
  
  for (; *in_HL != -1; in_HL = in_HL + 1) {
    FUN_ram_1a1a();
  }
  return;
}



// WARNING: Removing unreachable block (ram,0x1a92)
// WARNING: Removing unreachable block (ram,0x1abc)
// WARNING: Removing unreachable block (ram,0x1a99)
// WARNING: Removing unreachable block (ram,0x1aaa)
// WARNING: Removing unreachable block (ram,0x1a8b)

void FUN_ram_1a7f(void)

{
  byte in_io_000000a3;
  undefined1 uVar1;
  char cVar2;
  
  if ((in_io_000000a3 & 0x20) == 0) {
    disableMaskableInterrupts();
    setInterruptMask(0xb);
    enableMaskableInterrupts();
    uVar1 = 0x17;
    cVar2 = '\x02';
  }
  else {
    uVar1 = 4;
    cVar2 = '(';
  }
  do {
    uVar1 = FUN_ram_1b4f(uVar1);
    cVar2 = cVar2 + -1;
  } while (cVar2 != '\0');
  return;
}



// WARNING: Removing unreachable block (ram,0x1b0f)
// WARNING: Removing unreachable block (ram,0x1b1d)
// WARNING: Removing unreachable block (ram,0x1b08)
// WARNING: Removing unreachable block (ram,0x1b01)

undefined1 FUN_ram_1af5(void)

{
  byte in_io_00000081;
  byte in_io_000000a3;
  char cVar1;
  byte bVar2;
  
  if ((in_io_000000a3 & 0x20) == 0) {
    do {
    } while ((in_io_00000081 & 4) == 0);
    DAT_ram_a01e = 0;
    disableMaskableInterrupts();
    setInterruptMask(10);
    enableMaskableInterrupts();
    return 10;
  }
  bVar2 = 0;
  do {
  } while ((in_io_00000081 & 4) == 0);
  cVar1 = '(';
  do {
    FUN_ram_1b4f();
    cVar1 = cVar1 + -1;
  } while (cVar1 != '\0');
  if ((bVar2 & 8) != 0) {
    return 0x37;
  }
  return 0x17;
}



void FUN_ram_1b4d(void)

{
  return;
}



undefined1 FUN_ram_1b4f(void)

{
  undefined2 in_AF;
  char cVar1;
  
  cVar1 = '\x11';
  do {
    cVar1 = cVar1 + -1;
  } while (cVar1 != '\0');
  return (char)((ushort)in_AF >> 8);
}



void FUN_ram_1b58(undefined1 param_1,char param_2)

{
  char cVar1;
  char cVar2;
  
  do {
    cVar2 = -0x38;
    do {
      cVar1 = -0x38;
      do {
        cVar1 = cVar1 + -1;
      } while (cVar1 != '\0');
      cVar2 = cVar2 + -1;
    } while (cVar2 != '\0');
    param_2 = param_2 + -1;
  } while (param_2 != '\0');
  return;
}



undefined1 FUN_ram_1b69(void)

{
  ushort uVar1;
  char in_io_000000e7;
  undefined1 *in_HL;
  
  if (in_io_000000e7 != -1) {
    uVar1 = (ushort)in_HL >> 8;
    if (DAT_ram_a05a == '\x01') {
      return *(undefined1 *)((ushort)in_HL | 0xe000);
    }
    in_HL = (undefined1 *)((ushort)in_HL | 0xe000);
    DAT_ram_a059 = (char)uVar1;
  }
  return *in_HL;
}



undefined1 FUN_ram_1b95(void)

{
  ushort uVar1;
  char in_io_000000e7;
  undefined2 in_AF;
  undefined1 uVar2;
  undefined1 *in_HL;
  
  uVar2 = (undefined1)((ushort)in_AF >> 8);
  if (in_io_000000e7 != -1) {
    uVar1 = (ushort)in_HL >> 8;
    if (DAT_ram_a05a == '\x01') {
      *(undefined1 *)((ushort)in_HL | 0xe000) = uVar2;
      return uVar2;
    }
    in_HL = (undefined1 *)((ushort)in_HL | 0xe000);
    DAT_ram_a059 = (char)uVar1;
  }
  *in_HL = uVar2;
  return uVar2;
}



undefined1 FUN_ram_1bc4(void)

{
  undefined1 uVar1;
  char in_io_000000e7;
  undefined2 in_AF;
  undefined1 uVar2;
  undefined2 in_HL;
  
  uVar2 = (undefined1)((ushort)in_AF >> 8);
  uVar1 = DAT_ram_a059;
  if ((in_io_000000e7 != -1) && (uVar1 = (char)((ushort)in_HL >> 8), DAT_ram_a05a == '\x01')) {
    return uVar2;
  }
  DAT_ram_a059 = uVar1;
  return uVar2;
}



void FUN_ram_1bf3(void)

{
  char cVar1;
  
  while( true ) {
    cVar1 = FUN_ram_1b69();
    if (cVar1 == -1) break;
    FUN_ram_1a0c();
  }
  return;
}



undefined1 FUN_ram_2146(void)

{
  undefined2 in_AF;
  undefined1 uVar1;
  
  if (DAT_ram_a022 != '\0') {
    DAT_ram_a022 = '\0';
    uVar1 = 0x40;
    if (DAT_ram_a021 != '\0') {
      uVar1 = 0x80;
    }
    FUN_ram_1931(uVar1);
  }
  return (char)((ushort)in_AF >> 8);
}



void FUN_ram_22bb(void)

{
  byte in_io_000000e2;
  byte in_io_000000e3;
  char cVar1;
  undefined1 in_E;
  byte in_D;
  
  FUN_ram_22f7(in_io_000000e2 & 0x80 | ~DAT_ram_a00a & 0x7f);
  FUN_ram_22f7(in_E);
  FUN_ram_1939(in_io_000000e3 & 0x20 | in_D);
  FUN_ram_22f7(0x80);
  cVar1 = '\n';
  do {
    cVar1 = FUN_ram_190d(cVar1);
    cVar1 = cVar1 + -1;
  } while (cVar1 != '\0');
  return;
}



undefined1 FUN_ram_22f7(void)

{
  undefined2 in_AF;
  char cVar1;
  
  do {
    cVar1 = FUN_ram_190d();
  } while (cVar1 != '\x01');
  return (char)((ushort)in_AF >> 8);
}



void FUN_ram_2330(void)

{
  char cVar1;
  undefined2 in_BC;
  byte *pbVar2;
  
  if ((char)in_BC == '\x03') {
    FUN_ram_23b2();
    return;
  }
  do {
    pbVar2 = &DAT_ram_23cf;
    cVar1 = ' ';
    do {
      if (cVar1 == (char)((ushort)in_BC >> 8)) {
        cVar1 = '\x04';
        do {
          FUN_ram_23b2(~(*pbVar2 & 0x7f));
          pbVar2 = pbVar2 + 1;
          cVar1 = cVar1 + -1;
        } while (cVar1 != '\0');
        return;
      }
      pbVar2 = pbVar2 + 4;
      cVar1 = cVar1 + '\x01';
    } while (cVar1 != '[');
    in_BC = 0x2300;
  } while( true );
}



byte FUN_ram_2378(void)

{
  byte bVar1;
  byte bVar2;
  
  if (DAT_ram_a00a < 8) {
    bVar1 = 0xfe;
    for (bVar2 = DAT_ram_a00a; bVar2 != 0; bVar2 = bVar2 - 1) {
      bVar1 = bVar1 << 1 | bVar1 >> 7;
    }
    return bVar1;
  }
  return 0xff;
}



undefined1 FUN_ram_239b(void)

{
  return 0xff;
}



byte FUN_ram_23a4(void)

{
  byte in_io_000000e1;
  byte bVar1;
  
  bVar1 = FUN_ram_1916(in_io_000000e1 & 0xfb);
  return bVar1 | 4;
}



byte FUN_ram_23b2(void)

{
  byte in_E;
  char in_D;
  
  do {
    in_E = in_E >> 1 | in_E << 7;
    in_D = in_D + -1;
  } while (in_D != '\0');
  return in_E;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_2545(void)

{
  byte bVar1;
  undefined1 in_D;
  
  bVar1 = (*_DAT_ram_a010 ^ 0x55) & 0x7f;
  FUN_ram_2596(~DAT_ram_a00a & 0x7f);
  FUN_ram_2596(bVar1);
  FUN_ram_1939(in_D);
  FUN_ram_2596(0x80);
  FUN_ram_2596();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_254b(void)

{
  byte bVar1;
  undefined1 in_D;
  
  bVar1 = (*(byte *)(_DAT_ram_a010 + 2) ^ 0x55) & 0x7f;
  FUN_ram_2596(~DAT_ram_a00a & 0x7f);
  FUN_ram_2596(bVar1);
  FUN_ram_1939(in_D);
  FUN_ram_2596(0x80);
  FUN_ram_2596();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_2553(void)

{
  byte bVar1;
  undefined1 in_D;
  
  bVar1 = (*(byte *)(_DAT_ram_a010 + 4) ^ 0x55) & 0x7f;
  FUN_ram_2596(~DAT_ram_a00a & 0x7f);
  FUN_ram_2596(bVar1);
  FUN_ram_1939(in_D);
  FUN_ram_2596(0x80);
  FUN_ram_2596();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_2560(void)

{
  byte bVar1;
  undefined1 in_D;
  
  bVar1 = *(byte *)(_DAT_ram_a010 + 1) & 0x7f;
  FUN_ram_2596(~DAT_ram_a00a & 0x7f);
  FUN_ram_2596(bVar1);
  FUN_ram_1939(in_D);
  FUN_ram_2596(0x80);
  FUN_ram_2596();
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_2567(void)

{
  byte bVar1;
  undefined1 in_D;
  
  bVar1 = *(byte *)(_DAT_ram_a010 + 3) & 0x7f;
  FUN_ram_2596(~DAT_ram_a00a & 0x7f);
  FUN_ram_2596(bVar1);
  FUN_ram_1939(in_D);
  FUN_ram_2596(0x80);
  FUN_ram_2596();
  return;
}



undefined1 FUN_ram_2596(void)

{
  undefined2 in_AF;
  char cVar1;
  
  do {
    cVar1 = FUN_ram_190d();
  } while (cVar1 != '\x01');
  return (char)((ushort)in_AF >> 8);
}



undefined1 FUN_ram_25a8(void)

{
  undefined2 in_AF;
  
  return (char)((ushort)in_AF >> 8);
}



undefined1 FUN_ram_25b1(void)

{
  undefined2 in_AF;
  
  return (char)((ushort)in_AF >> 8);
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined1 FUN_ram_25c3(void)

{
  byte bVar1;
  undefined2 in_AF;
  undefined1 uVar2;
  
  uVar2 = (undefined1)((ushort)in_AF >> 8);
  if (DAT_ram_a022 == '\0') {
    return uVar2;
  }
  DAT_ram_a022 = 0;
  bVar1 = DAT_ram_a083 + 1;
  if ((DAT_ram_a083 + 1 & 1) != 0) {
    bVar1 = DAT_ram_a083 + 2;
  }
  do {
    DAT_ram_a083 = bVar1;
    FUN_ram_264f();
    FUN_ram_2661();
    FUN_ram_2680();
    FUN_ram_264f();
    FUN_ram_2661();
    FUN_ram_2680();
    FUN_ram_264f();
    FUN_ram_2661();
    FUN_ram_2680();
    FUN_ram_264f();
    FUN_ram_2661();
    FUN_ram_2680();
    DAT_ram_a083 = DAT_ram_a083 - 2;
    if (DAT_ram_a083 == 0) {
      return uVar2;
    }
    bVar1 = DAT_ram_a083;
  } while (DAT_ram_a083 != 0xff);
  return uVar2;
}



void FUN_ram_264f(void)

{
  FUN_ram_1b69();
  return;
}



byte FUN_ram_2661(void)

{
  char cVar1;
  char cVar2;
  byte bVar3;
  byte *in_HL;
  
  cVar2 = '\x1e';
  do {
    cVar1 = '\b';
    bVar3 = *in_HL;
    do {
      cVar2 = cVar2 + -1;
      if (cVar2 == '\0') {
        return bVar3 & 0x80;
      }
      bVar3 = bVar3 << 1 | bVar3 >> 7;
      cVar1 = cVar1 + -1;
    } while (cVar1 != '\0');
    in_HL = in_HL + 1;
  } while( true );
}



undefined1 FUN_ram_2680(void)

{
  char cVar1;
  
  cVar1 = '\n';
  do {
    cVar1 = cVar1 + -1;
  } while (cVar1 != '\0');
  return 0;
}



undefined1 FUN_ram_2a62(void)

{
  char in_io_000000cd;
  undefined2 in_AF;
  undefined1 uVar1;
  byte bVar2;
  char cVar3;
  char cVar4;
  char cVar5;
  undefined1 *puVar6;
  
  cVar3 = DAT_ram_a027;
  uVar1 = (undefined1)((ushort)in_AF >> 8);
  if (in_io_000000cd == -1) {
    return uVar1;
  }
  disableMaskableInterrupts();
  DAT_ram_a027 = 0;
  enableMaskableInterrupts();
  if (cVar3 == '\0') {
    if (DAT_ram_a026 != '\x11') {
      if (DAT_ram_a026 != '\x12') {
        DAT_ram_a027 = 0;
        return uVar1;
      }
      if (DAT_ram_a00d != '\x03') {
        DAT_ram_a027 = 0;
        return uVar1;
      }
      FUN_ram_2ace();
      goto LAB_ram_2b0f;
    }
  }
  else {
    cVar4 = DAT_ram_a02a;
    cVar5 = DAT_ram_a02b;
    if (cVar3 == '\x01') {
      bVar2 = DAT_ram_a028 & 0xf0;
      if (bVar2 == 0x70) {
        cVar5 = '\x01';
      }
      else {
        if (bVar2 != 0xb0) {
          if (bVar2 == 0xd0) goto LAB_ram_2c19;
          if (bVar2 == 0xe0) goto LAB_ram_2c3e;
          if (bVar2 == 0xc0) goto code_r0x2bdd;
          if ((DAT_ram_a028 & 4) == 0) {
            cVar4 = '\x02';
          }
          else {
            cVar4 = '\x01';
          }
          if (DAT_ram_a02a == cVar4) {
            DAT_ram_a027 = 0;
            return uVar1;
          }
          goto LAB_ram_2b3d;
        }
        cVar5 = '\x02';
      }
      if (DAT_ram_a02b == cVar5) {
        DAT_ram_a027 = 0;
        return uVar1;
      }
    }
    else if (cVar3 != '\x02') {
      if (cVar3 == '\x03') {
code_r0x2bdd:
        cVar3 = '\x04';
        puVar6 = &DAT_ram_a02c;
        do {
          *puVar6 = 0;
          puVar6 = puVar6 + 1;
          cVar3 = cVar3 + -1;
        } while (cVar3 != '\0');
        FUN_ram_2ace(10);
        cVar4 = DAT_ram_a02a;
        cVar5 = DAT_ram_a02b;
      }
      else {
        if (cVar3 == '\x04') {
LAB_ram_2c19:
          FUN_ram_2a8d();
          DAT_ram_a02f = DAT_ram_a02f + '\x01';
          if (DAT_ram_a02f == ':') {
            DAT_ram_a02f = '0';
            DAT_ram_a02e = DAT_ram_a02e + '\x01';
            if (DAT_ram_a02e == '6') {
              DAT_ram_a02e = '0';
            }
          }
        }
        else {
          if (cVar3 != '\x05') {
            if (cVar3 != '\x06') {
              DAT_ram_a027 = 0;
              return uVar1;
            }
            goto LAB_ram_2b3d;
          }
LAB_ram_2c3e:
          FUN_ram_2a8d();
          DAT_ram_a02d = DAT_ram_a02d + '\x01';
          if (DAT_ram_a02d == '4') {
            if (DAT_ram_a02c == '2') {
              DAT_ram_a02c = '0';
              DAT_ram_a02d = '0';
            }
          }
          else if (DAT_ram_a02d == ':') {
            DAT_ram_a02d = '0';
            DAT_ram_a02c = DAT_ram_a02c + '\x01';
            if ((DAT_ram_a02c == '!') || (DAT_ram_a02c == '@')) {
              DAT_ram_a02c = '1';
            }
            else if (DAT_ram_a02c == '3') {
              DAT_ram_a02c = '0';
            }
          }
        }
        FUN_ram_2ad9();
        cVar4 = DAT_ram_a02a;
        cVar5 = DAT_ram_a02b;
      }
    }
LAB_ram_2b3d:
    DAT_ram_a02b = cVar5;
    DAT_ram_a02a = cVar4;
    if ((DAT_ram_a008 != '\x01') ||
       (((DAT_ram_a026 != '\x11' && (DAT_ram_a026 != '\x12')) && (DAT_ram_a026 != '\0')))) {
      return uVar1;
    }
  }
  FUN_ram_2a8d();
LAB_ram_2b0f:
  DAT_ram_a026 = 0;
  puVar6 = &DAT_ram_e043;
  cVar3 = '\x04';
  do {
    *puVar6 = 0x23;
    puVar6 = puVar6 + 1;
    cVar3 = cVar3 + -1;
  } while (cVar3 != '\0');
  DAT_ram_a012 = 0x4a;
  return uVar1;
}



void FUN_ram_2a8d(void)

{
  byte in_io_000000c2;
  byte in_io_000000c3;
  byte in_io_000000c4;
  byte in_io_000000c5;
  byte bVar1;
  byte *in_HL;
  
  bVar1 = in_io_000000c5 & 3 | 0x30;
  *in_HL = bVar1;
  if (((bVar1 == 0x30) && (*in_HL = 0x3f, DAT_ram_a009 != '\x03')) && (DAT_ram_a009 != '\n')) {
    *in_HL = 0x20;
  }
  in_HL[1] = in_io_000000c4 & 0xf | 0x30;
  in_HL[2] = in_io_000000c3 & 7 | 0x30;
  in_HL[3] = in_io_000000c2 & 0xf | 0x30;
  return;
}



byte FUN_ram_2ace(void)

{
  byte bVar1;
  short in_HL;
  
  bVar1 = *(byte *)(in_HL + 3) & 0xf;
  if (9 < bVar1) {
    bVar1 = 0;
  }
  return bVar1;
}



byte FUN_ram_2ad9(void)

{
  byte bVar1;
  short in_HL;
  
  bVar1 = *(byte *)(in_HL + 3) & 0xf;
  if (9 < bVar1) {
    bVar1 = 0;
  }
  return bVar1;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_ram_2c80(void)

{
  byte in_io_00000080;
  byte in_io_00000081;
  byte in_io_000000a3;
  undefined1 in_C;
  char cVar3;
  short sVar1;
  ushort uVar2;
  byte bVar4;
  byte *pbVar5;
  
  bVar4 = in_io_00000080;
  FUN_ram_3022();
  DAT_ram_a07b = bVar4;
  if (((in_io_00000081 & 0x78) == 8) || ((in_io_00000081 & 0x78) != 0)) {
    DAT_ram_a07c = DAT_ram_a07c | 0x80;
    DAT_ram_a07b = 0;
  }
  else {
    uVar2 = CONCAT11(DAT_ram_a006,in_C) & 0x1fff;
    in_C = (undefined1)uVar2;
    DAT_ram_a07d = DAT_ram_a07d & 0x20 |
                   DAT_ram_a080 & 0x40 | (DAT_ram_a07e & 0x20) << 2 | (byte)(uVar2 >> 8);
  }
  bVar4 = DAT_ram_a07c;
  if (DAT_ram_a089 != '\0') {
LAB_ram_2d3d:
    do {
      if (DAT_ram_a089 == '\x01') {
        DAT_ram_a087 = '\x03';
LAB_ram_2d4a:
        if (DAT_ram_a087 != '\0') {
          DAT_ram_a087 = DAT_ram_a087 + -1;
          DAT_ram_a089 = 2;
          if (DAT_ram_a088 == '\x02') {
            FUN_ram_1a7f();
            FUN_ram_3027();
            FUN_ram_3027();
            FUN_ram_1af5();
            FUN_ram_3042();
            return;
          }
          if (DAT_ram_a088 != '\x01') {
            DAT_ram_a089 = 0;
            DAT_ram_a080 = DAT_ram_a080 | 0x80;
            return;
          }
          FUN_ram_1a7f();
          FUN_ram_3027();
          DAT_ram_a085 = DAT_ram_a07d & 0x1f;
          FUN_ram_3027();
          sVar1 = (ushort)DAT_ram_a08c << 8;
          DAT_ram_a085 = DAT_ram_a085 ^ DAT_ram_a08c;
          FUN_ram_3027();
          pbVar5 = &DAT_ram_a08d;
          do {
            cVar3 = (char)((ushort)sVar1 >> 8);
            DAT_ram_a085 = DAT_ram_a085 ^ *pbVar5;
            FUN_ram_3027();
            pbVar5 = pbVar5 + 1;
            sVar1 = (ushort)(byte)(cVar3 - 1U) << 8;
          } while ((byte)(cVar3 - 1U) != 0);
          FUN_ram_3027();
          FUN_ram_3027();
          FUN_ram_1af5();
          DAT_ram_a088 = 0;
          FUN_ram_3042();
          return;
        }
LAB_ram_2d52:
        FUN_ram_1a7f();
        FUN_ram_3027();
        FUN_ram_3027();
        FUN_ram_3027();
        FUN_ram_3027();
        FUN_ram_1af5();
        DAT_ram_a089 = 0;
        DAT_ram_a07c = 0;
        DAT_ram_a088 = 0;
        return;
      }
      if (DAT_ram_a089 == '\x02') {
        if (DAT_ram_a07b == 6) {
          DAT_ram_a089 = 3;
          FUN_ram_3042();
          return;
        }
LAB_ram_2e15:
        FUN_ram_1a7f();
        FUN_ram_3027();
        if (DAT_ram_a088 == '\x01') goto LAB_ram_2d4a;
        goto LAB_ram_2d52;
      }
      if ((DAT_ram_a07b & 0x1f) != (DAT_ram_a086 & 0x1f)) goto LAB_ram_2e15;
      DAT_ram_a086 = DAT_ram_a07b;
      if (DAT_ram_a088 == '\0') {
        DAT_ram_a089 = 0;
        DAT_ram_a080 = DAT_ram_a080 & 0x9f | 0x20;
        return;
      }
      if ((DAT_ram_a07b & 0x80) == 0) goto LAB_ram_2e15;
      DAT_ram_a088 = '\x01';
      DAT_ram_a089 = '\x01';
    } while( true );
  }
  uVar2 = CONCAT11(DAT_ram_a07c,in_C);
  if ((DAT_ram_a07c & 1) == 0) {
    if (DAT_ram_a07b == 0x1d) {
      if ((DAT_ram_a07c & 2) == 0) {
        DAT_ram_a07c = DAT_ram_a07c | 2;
        FUN_ram_3042();
        return;
      }
    }
    else if ((DAT_ram_a07c & 2) == 0) {
      return;
    }
    uVar2 = CONCAT11(DAT_ram_a07c,DAT_ram_a07b) & 0xff1f;
    bVar4 = (byte)(uVar2 >> 8);
    if ((DAT_ram_a07d & 0x1f) == (byte)uVar2) {
      DAT_ram_a07c = bVar4 & 0xfd | 1;
      FUN_ram_1a7f();
      FUN_ram_3027();
      FUN_ram_3027();
      FUN_ram_1af5();
      return;
    }
    DAT_ram_a07c = bVar4 & 0x7d;
    return;
  }
  if ((DAT_ram_a07c & 4) != 0) {
    *_DAT_ram_a08a = DAT_ram_a07b;
    DAT_ram_a085 = DAT_ram_a085 ^ *_DAT_ram_a08a;
    _DAT_ram_a08a = _DAT_ram_a08a + 1;
    DAT_ram_a084 = DAT_ram_a084 + -1;
    if (DAT_ram_a084 == '\0') {
      DAT_ram_a07c = bVar4 & 0xfb;
      FUN_ram_3042();
      return;
    }
    FUN_ram_3042();
    return;
  }
  if ((DAT_ram_a07c & 8) != 0) {
    if (DAT_ram_a088 == '\x01') {
      _DAT_ram_a08a = &DAT_ram_a039;
      DAT_ram_a084 = DAT_ram_a07b;
      DAT_ram_a07f = DAT_ram_a07b;
      DAT_ram_a085 = DAT_ram_a085 ^ DAT_ram_a07b;
      DAT_ram_a088 = 0;
      DAT_ram_a07c = DAT_ram_a07c & 0xf7 | 4;
      FUN_ram_3042();
      return;
    }
    DAT_ram_a088 = 1;
    DAT_ram_a085 = DAT_ram_a07b;
    DAT_ram_a07e = DAT_ram_a07e & 0xe0 | DAT_ram_a07b & 0x1f;
    FUN_ram_3042();
    return;
  }
  if ((DAT_ram_a07c & 0x10) == 0) {
    if ((DAT_ram_a07c & 0x20) == 0) {
      if ((DAT_ram_a07c & 0x40) != 0) {
        DAT_ram_a07c = DAT_ram_a07c & 0xbf;
        DAT_ram_a07f = 1;
        DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x40;
        return;
      }
      if (DAT_ram_a07b == 0x18) {
        DAT_ram_a07c = DAT_ram_a07c & 0x81;
        DAT_ram_a07e = DAT_ram_a07e & 0xbf | 0xa0;
        return;
      }
      if (DAT_ram_a07b == 3) {
        DAT_ram_a07c = DAT_ram_a07c | 0x20;
        FUN_ram_3042();
        return;
      }
      if (DAT_ram_a07b == 0) {
        return;
      }
      if (DAT_ram_a07b == 2) {
        if ((DAT_ram_a07d & 0x80) != 0) {
          DAT_ram_a088 = 0;
          DAT_ram_a085 = 0;
          DAT_ram_a07c = DAT_ram_a07c | 8;
          FUN_ram_3042();
          return;
        }
        return;
      }
      if (DAT_ram_a07b == 1) {
        FUN_ram_1a7f();
        FUN_ram_3027();
        FUN_ram_3027();
        FUN_ram_1af5();
        return;
      }
      if (DAT_ram_a07b == 0xc) {
        disableMaskableInterrupts();
        DAT_ram_a05a = 0;
        _DAT_ram_a0f1 = (undefined *)0x356;
        FUN_ram_1994();
        _DAT_ram_a0f1 = (undefined *)0x359;
        FUN_ram_03f2();
        if ((in_io_000000a3 & 0x20) == 0) {
          DAT_ram_a00c = '\0';
          DAT_ram_a07e = 0x20;
          DAT_ram_a080 = 0x20;
          DAT_ram_a07d = 0xa0;
        }
        if (DAT_ram_a00c != '\0') {
          _DAT_ram_a0f1 = (undefined *)0x379;
          FUN_ram_07da();
        }
        setInterruptMask(0x1a);
        enableMaskableInterrupts();
        if ((in_io_000000a3 & 0x20) == 0) {
          do {
            do {
              do {
                _DAT_ram_a0f1 = (undefined *)0x3b5;
                FUN_ram_199f();
              } while ((DAT_ram_a07e & 0xc0) != 0x40);
            } while ((DAT_ram_a07c & 1) != 0);
            _DAT_ram_a0f1 = &DAT_ram_a039;
            uVar2 = CONCAT11(DAT_ram_a07f,(char)uVar2);
            do {
              _DAT_ram_a0ed = 0x3db;
              _DAT_ram_a0ef = uVar2;
              FUN_ram_08da(*_DAT_ram_a0f1,0x61);
              _DAT_ram_a0f1 = _DAT_ram_a0f1 + 1;
              cVar3 = (char)(_DAT_ram_a0ef >> 8) + -1;
              uVar2 = CONCAT11(cVar3,(char)_DAT_ram_a0ef);
            } while (cVar3 != '\0');
            DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x20;
            _DAT_ram_a0f1 = (undefined *)0x3ef;
            FUN_ram_124e();
          } while( true );
        }
        DAT_ram_a027 = 6;
        if (DAT_ram_a023 != '\0') {
          do {
            do {
              _DAT_ram_a0f1 = (undefined *)0x39d;
              FUN_ram_199f();
              _DAT_ram_a0f1 = (undefined *)0x3a0;
              FUN_ram_2a62();
              cVar3 = DAT_ram_a012;
              disableMaskableInterrupts();
              DAT_ram_a012 = 'N';
              enableMaskableInterrupts();
            } while (cVar3 != 'J');
            _DAT_ram_a0f1 = (undefined *)0x39a;
            FUN_ram_123e();
          } while( true );
        }
        do {
          _DAT_ram_a0f1 = (undefined *)0x394;
          FUN_ram_123e();
        } while( true );
      }
      if (DAT_ram_a07b == 0xf) {
        DAT_ram_a07c = DAT_ram_a07c | 0x40;
        FUN_ram_3042();
        return;
      }
      if (DAT_ram_a07b == 0x1e) {
        if ((DAT_ram_a07d & 0x1f) != 0) {
          DAT_ram_a07d = DAT_ram_a07d & 0xdf;
          DAT_ram_a089 = 0;
          DAT_ram_a07c = 0;
          DAT_ram_a088 = 0;
          FUN_ram_3022();
          return;
        }
        return;
      }
      return;
    }
    uVar2 = CONCAT11(DAT_ram_a07c,DAT_ram_a07b) & 0xdfff;
    if ((DAT_ram_a085 == DAT_ram_a07b) && ((DAT_ram_a07c & 0x80) == 0)) {
      DAT_ram_a07c = DAT_ram_a07c & 0xdf;
      FUN_ram_1a7f();
      FUN_ram_3027();
      FUN_ram_3027();
      FUN_ram_1af5();
      DAT_ram_a07e = DAT_ram_a07e & 0x1f | 0x40;
      return;
    }
  }
  else if ((DAT_ram_a07c & 0x80) == 0) {
    DAT_ram_a089 = '\x01';
    DAT_ram_a088 = '\x02';
    DAT_ram_a086 = DAT_ram_a07b;
    DAT_ram_a07c = DAT_ram_a07c & 0xef;
    goto LAB_ram_2d3d;
  }
  DAT_ram_a07c = (byte)(uVar2 >> 8) & 0x7f;
  FUN_ram_1a7f();
  FUN_ram_3027();
  FUN_ram_3027();
  FUN_ram_1af5();
  return;
}



void FUN_ram_3022(void)

{
  DAT_ram_a01e = 0;
  return;
}



void FUN_ram_3027(void)

{
  byte in_io_00000081;
  
  do {
  } while ((in_io_00000081 & 1) == 0);
  DAT_ram_a01e = 0;
  return;
}



void FUN_ram_3042(void)

{
  DAT_ram_a01e = 200;
  return;
}



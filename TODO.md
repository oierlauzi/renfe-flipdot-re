# What is left

The project met its goal and is in maintenance. These are the questions that
stayed open. None of them stops the sign working.

## With the hardware on a bench

- **DB37 pin numbers.** Four conductors, identified by colour but not by pin.
  Buzz each one through to the daughter 1 terminal block. This is the last gap
  in an otherwise complete wiring diagram.
- **Switch sense and bit order.** The measured configuration fixes what the
  ports must read, but neither polarity reproduces it from the switch positions
  as found. Continuity from one end of each bank to the chip settles bit order,
  and an ohmmeter to ground and Vcc settles the sense.
  See [docs/4-inside-the-firmware.md](docs/4-inside-the-firmware.md#this-units-settings).
- **What the two effect lines do.** Send `BEL 0` through `BEL 3` to a powered
  unit and watch. They are not brightness and not the tube, so the guess is the
  LED row that a complete sign carries.
- **What the other 48 bus positions reach.** Instrument the ribbon and send a
  64-character message. Nothing responding past position 15 would rule out the
  LED row sitting on the character bus.
- **Where the flip-dot high side comes from.** The Darlington arrays can only
  sink. Part numbers on the 9 transistors near the ribbon, and which side of the
  load they sit on, would settle it.
- **U8's marking.** The last unidentified part on the motherboard, and the only
  candidate for driving the unconnected `RP RN TP TN` terminal block.
- **Is the clock board fitted?** The firmware disables the feature silently when
  the bus reads open, so its absence is invisible from outside.
- **How two modules on one carrier get distinct addresses** from a single set of
  six switches. A base plus a fixed per-socket offset would fit.

## With only the ROM

- **The third receive protocol.** 930 bytes at `0x2C80`, the largest undecoded
  region of the ROM, selected when port C bit 5 is clear. The command bytes, the
  reply frames and the length-prefixed message staging are known. The frame
  layout, any checksum, and which command opens a transaction are not. Single
  bytes fed to it in the simulator produce silence, so it wants a sequence.

## Paper trail

- **PAGE Ibérica still operates in Tres Cantos.** A polite email asking for
  PI-T100 documentation costs nothing.
- **Search under Telenorma and Bosch Telecom**, not only PAGE Ibérica. The
  motherboard part numbers in [hardware/README.md](hardware/README.md#telenorma-part-numbers)
  are the most specific identifiers the unit carries.
- **Any original controller, or any recording of the line**, would settle the
  third protocol at a stroke.

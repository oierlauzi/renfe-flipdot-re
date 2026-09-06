# Hardware

Physical description of the unit. Behaviour and register-level detail live in
[../docs/](../docs/).

| File | Contents |
| --- | --- |
| [chips.md](chips.md) | Per-IC inventory: part numbers, PCB, datasheets |
| [pcbs.md](pcbs.md) | The three boards and what each does |
| [connectors.md](connectors.md) | DB37 external link, panel ribbon, power |
| [psu.md](psu.md) | Power supply |
| [photos/](photos/) | As-found photographs |

## Photos

| File | Subject |
| --- | --- |
| [cover_open.jpg](photos/cover_open.jpg) | Whole unit with the cover removed |
| [mother_pcb_top.jpg](photos/mother_pcb_top.jpg) | Motherboard — CPU, RAM/IO, USART, EPROM |
| [daughter_pcb1_top.jpg](photos/daughter_pcb1_top.jpg) | Daughter 1 — serial TX/RX, DC converter |
| [daughter_pcb2_top.jpg](photos/daughter_pcb2_top.jpg) | Daughter 2 — flip-dot drive |
| [ROM_chip.jpg](photos/ROM_chip.jpg) | U4, the M27256 EPROM |
| [sticker.jpg](photos/sticker.jpg) | Chassis label — model, serial, "MAESTRA" |
| [switches1.jpg](photos/switches1.jpg) [switches2.jpg](photos/switches2.jpg) [switches3.jpg](photos/switches3.jpg) | The three DIP switch banks |
| [Flip-dot carrier.jpg](photos/Flip-dot%20carrier.jpg) | The panel-side carrier board — address switches, ribbon connector, shared by a pair of character modules |
| [Flip-dot daughter (front).jpg](photos/Flip-dot%20daughter%20%28front%29.jpg) | A character module, dot side |
| [Flip-dot daughter (back).jpg](photos/Flip-dot%20daughter%20%28back%29.jpg) | A character module, diode-matrix side |

When adding photos: no spaces in filenames, and prefer a focused shot of the
thing being documented over another whole-board picture. If a photo is evidence
for a claim, link it from the document making the claim.

## What is still unphotographed

- [ ] Both PCBs from the **solder side** — needed to trace the chip selects and
      the DB37 wiring.
- [ ] The **DB37 pin numbering** alongside the 4 populated wires.
- [ ] The **DIP switch positions as found**, per bank, close enough to read.
      This is the one piece of state that is destroyed by experimenting and
      cannot be recovered from the ROM. See
      [../docs/dip-switches.md](../docs/dip-switches.md).
- [ ] The **panel ribbon connector**, with a conductor count.

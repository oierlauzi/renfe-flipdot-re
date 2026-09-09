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
| [Electronics.jpg](photos/Electronics.jpg) [Electronics + PSU.jpg](photos/Electronics%20%2B%20PSU.jpg) | The three boards together, and with the PSU |
| [Mother (top).jpg](photos/Mother%20%28top%29.jpg) [Mother (bottom).jpg](photos/Mother%20%28bottom%29.jpg) | Motherboard — CPU, RAM/IO, USART, EPROM |
| [Daughter 1 (top).jpg](photos/Daughter%201%20%28top%29.jpg) [Daughter 1 (bottom).jpg](photos/Daughter%201%20%28bottom%29.jpg) | Daughter 1 — RS-422 interface, DC converter |
| [Daughter 1 Terminal block connections.jpg](photos/Daughter%201%20Terminal%20block%20connections.jpg) | Where the four DB37 conductors land |
| [Daughter 2 (top).jpg](photos/Daughter%202%20%28top%29.jpg) [Daughter 2 (bottom).jpg](photos/Daughter%202%20%28bottom%29.jpg) | Daughter 2 — flip-dot drive |
| [DB37_back.jpg](photos/DB37_back.jpg) | The DB37 connector, wire side |
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

- [ ] The **DB37 pin numbering** alongside the four populated wires.
- [ ] The motherboard's `RP RN TP TN` terminal block and what it connects to.

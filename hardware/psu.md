# Power supply

| Property | Value | Conf. |
| --- | --- | --- |
| Input to the unit | **24 VDC** | **[C]** |
| Current draw, idle | *TODO* | |
| Current draw, flipping | *TODO* | |
| Logic rail | 5 V, from the DC converter module on daughter 1 | **[L]** |
| Connector | *TODO* | |
| Polarity | *TODO* | |

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

A relatively large DC-DC converter module is soldered to daughter 1 (see
[pcbs.md](pcbs.md)); the logic almost certainly runs from that, with the 24 V
rail going straight to the flip-dot coils via the ULN2003A sinks on daughter 2.

## Before first power-up

- [ ] Record the connector type and **polarity** — photograph it.
- [ ] Measure the idle current on a bench supply with a current limit set. A
      unit that has been in storage may have leaky electrolytics; bringing it up
      slowly on a variable supply is worth the extra minutes.
- [ ] Measure the peak current while dots are flipping — this sizes the supply
      needed for permanent installation.
- [ ] Confirm the 5 V rail before assuming the logic is healthy.

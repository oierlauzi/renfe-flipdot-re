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

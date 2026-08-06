# Power and lighting

Confidence key: **[C]** confirmed, **[L]** likely, **[?]** guess.

## Two independent AC inputs **[C]**

The enclosure takes **two separate mains feeds**:

| Feed | Supplies | Expected switching |
| --- | --- | --- |
| AC input 1 | The 24 VDC PSU, i.e. the electronics | Permanently live **[L]** |
| AC input 2 | The fluorescent tube along the lower edge | Switched with the street-lighting circuit, so on at night **[L]** |

Separating them is what you would do if the sign must keep running around the
clock while its backlight follows the station lighting. The flip-dot panel is
bistable and readable in daylight without illumination, so the tube is purely a
night-time reading light — losing it does not blank the sign.

**The tube's ballast/reactance is inside the display body.** That matters well
beyond the lighting:

* It is a substantial **inductive noise source** sitting inside the same
  enclosure as the logic and the serial interface. Any signal measurement taken
  with the tube energised is suspect — see
  [connectors.md](connectors.md#why-the-scope-showed-nothing-useful).
* It means **mains voltage is present inside the case**, in a second place
  beyond the PSU. Treat the interior as live whenever either feed is connected.

## 24 VDC rail

| Property | Value | Conf. |
| --- | --- | --- |
| Input to the electronics | **24 VDC** | **[C]** |
| Current draw, idle | *TODO* | |
| Current draw, flipping | *TODO* | |
| Logic rail | 5 V, from the DC converter module on daughter 1 | **[L]** |
| Connector | *TODO* | |
| Polarity | *TODO* | |

A relatively large DC-DC converter module is soldered to daughter 1 (see
[pcbs.md](pcbs.md)); the logic almost certainly runs from that, with the 24 V
rail going straight to the flip-dot coils via the ULN2003A sinks on daughter 2.

The 24 V rail is also the likeliest source for the serial loop current, if the
interface turns out to be a current loop — worth checking whether it reaches the
daughter-1 terminal block.

## Safety

Two mains feeds and an internal ballast mean **live mains in more than one place
inside the enclosure**. Before probing anything:

- [ ] Identify and label both AC inputs, and confirm which one feeds the PSU.
- [ ] Establish that the tube feed is genuinely dead before working inside.
- [ ] Prefer measurements with the tube feed disconnected — it removes the
      ballast as a noise source *and* as a hazard.

## Before first power-up

- [ ] Record the connector type and **polarity** for the DC side.
- [ ] Bring the unit up on a current-limited bench supply. A unit that has been
      in storage may have leaky electrolytics.
- [ ] Measure idle and peak (dots flipping) current.
- [ ] Confirm the 5 V rail before assuming the logic is healthy.

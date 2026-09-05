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

**The tube is not coupled to the electronics in any way** (owner, 2026-08): no
control line, no sense line, mains feed and ballast entirely its own. **[C]**

That is worth stating explicitly because it *excludes* an otherwise attractive
hypothesis. The firmware drives two "display attribute" lines out to the panel
([../docs/flipdot-interface.md](../docs/flipdot-interface.md#port-c-bits-10-are-the-display-attribute-lines-c)),
and illumination control would have been the natural guess for what two such lines
do on a sign with a lamp in it. The firmware cannot switch this tube, so they must
do something else.

**The tube's ballast/reactance is inside the display body.** That matters well
beyond the lighting:

* It is a substantial **inductive noise source** sitting inside the same
  enclosure as the logic and the serial interface. Any signal measurement taken
  with the tube energised is suspect — see
  [connectors.md](connectors.md#attempt-1-oscilloscope-2026-08--transitions-present-signal-unclear).
* It means **mains voltage is present inside the case**, in a second place
  beyond the PSU. Treat the interior as live whenever either feed is connected.

## 24 VDC rail

| Property | Value | Conf. |
| --- | --- | --- |
| Input to the electronics | **24 VDC** | **[C]** |
| Current draw, idle | *TODO* | |
| Current draw, flipping | *TODO* | |
| Logic rail | **5 V**, from the `24 IMP 3-05-7` module on daughter 1 | **[L]** |
| Connector | *TODO* | |
| Polarity | *TODO* | |

A relatively large DC-DC converter module is soldered to daughter 1 (see
[pcbs.md](pcbs.md)), a Melcher **`24 IMP 3-05-7`**: **10-36 V DC in, 5 V / 500 mA
out**, isolated. The 24 V rail itself goes to the flip-dot coils via the ULN2003A
sinks on daughter 2.

### The logic 5 V comes from the converter **[L]**

Settled by the module's own type code, `24 IMP 3-05-7`: **5 V out at 500 mA**,
from a 10-36 V input ([pcbs.md](pcbs.md#the-dc-dc-converter)). That is a logic
supply, and 500 mA is a comfortable budget for this board's CMOS parts — an
MSM80C85AH, two M81C55-5s, an M82C51A-2 and a 32 KiB EPROM.

It stays **[L]** rather than **[C]** only because nobody has traced `VO+` to the
5 V net.

> **A contradiction, and I picked the wrong side of it.** For one revision this
> section carried the 5 V source as **[?]**, on the grounds that
> [pcbs.md](pcbs.md#the-dc-dc-converter) said the converter's output floats
> against the display's 0 V and therefore powers an isolated line side — which
> would leave the logic supply unaccounted for. I judged that the likelier of the
> two readings and sent someone hunting for a regulator on the motherboard.
>
> **It was the other one.** Four converter pins prove input-to-output isolation
> and nothing about which side the display's 0 V is on; that had been assumed.
> This file's original claim — the logic runs from the daughter-1 converter — was
> right all along, and the 24 V input is what floats relative to the logic.

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

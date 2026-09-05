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
| Logic rail | **5 V**, from two TO-220 regulators **on the motherboard** | **[L]** |
| Connector | *TODO* | |
| Polarity | *TODO* | |

A relatively large DC-DC converter module is soldered to daughter 1 (see
[pcbs.md](pcbs.md)), a Melcher **`24 IMP 3-05-7`**: **10-36 V DC in, 5 V / 500 mA
out**, isolated. The 24 V rail itself goes to the flip-dot coils via the ULN2003A
sinks on daughter 2.

### The logic 5 V comes from the motherboard, not the converter **[L]**

Two TO-220 regulators on gold heatsinks sit on the motherboard
([pcbs.md](pcbs.md#motherboard)) and supply the logic rails from the 24 V input.
That is consistent with the board architecture: the motherboard is the logic, and
it powers itself.

Daughter 1's Melcher `24 IMP 3-05-7` is a **separate, isolated 5 V** for the
**line side** of the serial interface — the job a media-conversion board actually
has. See [pcbs.md](pcbs.md#the-dc-dc-converter).

> **I flip-flopped on this, and the answer was already written down.** This file
> originally said the logic ran from the daughter-1 converter. I first downgraded
> that to **[?]** on a bad argument in `pcbs.md`, then — when the converter turned
> out to be 5 V — reversed and declared the converter *was* the logic supply, and
> closed a TODO to look for a motherboard regulator as a "non-item".
>
> **Both moves were wrong.** `pcbs.md` had recorded the two TO-220 regulators on
> the motherboard from the start, in a section above the one I was editing. The
> converter is the isolated line-side supply, which is what `pcbs.md` concluded
> originally — its *reasoning* was faulty, its conclusion was not.

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

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
| Logic rail | 5 V, source **unresolved** — see below | **[?]** |
| Connector | *TODO* | |
| Polarity | *TODO* | |

A relatively large DC-DC converter module is soldered to daughter 1 (see
[pcbs.md](pcbs.md)), marked **`Mercher 24 IMP 3-50-7`** with a **9-36 VDC** input
range — consistent with running from the 24 V rail. The 24 V rail itself goes to
the flip-dot coils via the ULN2003A sinks on daughter 2.

### Where the logic 5 V comes from is unresolved **[?]**

This file used to say the logic "almost certainly runs from" the daughter-1
converter. **That cannot be reconciled with what the same converter is doing for
the serial interface**, and the conflict was sitting unnoticed across two
documents:

* [pcbs.md](pcbs.md#the-dc-dc-converter) establishes the converter is **genuinely
  isolated** — four nets, no shared return — so its output floats with respect to
  the display's internal 0 V, and concludes it exists to power an isolated line
  side.
* An isolated floating rail **cannot be the logic supply**, because the logic
  shares 0 V with everything else in the unit.

Both cannot be true. Either the converter feeds the line side and the logic 5 V
comes from somewhere not yet identified, or the isolation reading is wrong. The
first is far more likely given daughter 1's role, but nothing has been measured.
**Find the 5 V regulator** — it is not on the parts list for either daughter
board, so look on the motherboard.

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

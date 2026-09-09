# Power and lighting

## Two independent AC inputs

The enclosure takes two separate mains feeds:

| Feed | Supplies |
| --- | --- |
| AC input 1 | The 24 VDC PSU, i.e. the electronics. Presumably permanently live |
| AC input 2 | The fluorescent tube along the lower edge. Presumably switched with the station lighting |

Separating them is what a sign needs if it must run around the clock while its
backlight follows the station lighting. The flip-dot panel is bistable and
readable in daylight without illumination, so the tube is purely a night-time
reading light and losing it does not blank the sign.

**The tube is not coupled to the electronics in any way**: no control line, no
sense line, and its own mains feed and ballast. The firmware cannot switch it,
which is why the two display attribute lines it drives out to the panel
([../docs/flipdot-interface.md](../docs/flipdot-interface.md#display-attributes-on-port-c-bits-10))
must do something other than illumination control.

**The tube's ballast is inside the display body**, which matters beyond the
lighting:

* It is a substantial inductive noise source inside the same enclosure as the
  logic and the serial interface, so any signal measurement taken with the tube
  energised is suspect.
* Mains voltage is present inside the case in a second place beyond the PSU.
  Treat the interior as live whenever either feed is connected.

## The 24 VDC rail

| Property | Value |
| --- | --- |
| Input to the electronics | 24 VDC |
| Logic rail | 5 V, from two TO-220 regulators on the motherboard |
| Line-side rail | 5 V isolated, from the Melcher converter on daughter 1 |
| Current draw, idle and flipping | not measured |
| Connector and polarity | not recorded |

The 24 V rail goes to the flip-dot coils through the ULN2003A sinks on daughter 2.

Two TO-220 regulators on gold heatsinks sit on the motherboard
([pcbs.md](pcbs.md#motherboard)) and supply the logic rails from the 24 V input,
so the logic board powers itself.

Daughter 1's Melcher `24 IMP 3-05-7` — 10-36 V DC in, 5 V at 500 mA out, isolated
— is a separate supply for the **line side** of the serial interface, which is
what a media-conversion board needs. See
[pcbs.md](pcbs.md#the-dc-dc-converter).

## Safety

Two mains feeds and an internal ballast mean live mains in more than one place
inside the enclosure. Before probing anything:

- [ ] Identify and label both AC inputs, and confirm which one feeds the PSU.
- [ ] Establish that the tube feed is genuinely dead before working inside.
- [ ] Prefer measurements with the tube feed disconnected — it removes the
      ballast as a noise source and as a hazard.

## Before first power-up

- [ ] Record the connector type and polarity for the DC side.
- [ ] Bring the unit up on a current-limited bench supply. A unit that has been
      in storage may have leaky electrolytics.
- [ ] Measure idle and peak current, with dots flipping.
- [ ] Confirm the 5 V rail before assuming the logic is healthy.

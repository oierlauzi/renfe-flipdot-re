# External references

## Datasheets

Component-level links live in [hardware/chips.md](../hardware/chips.md) next to
each part. Only the four that matter for the protocol are repeated here:

| Part | Role | Datasheet |
| --- | --- | --- |
| OKI MSM80C85AH | CPU | <https://www.silicon-ark.co.uk/datasheets/m80c85AH-datasheet-oki.pdf> |
| OKI M81C55-5 | RAM + I/O ports + timer | <https://docs.rs-online.com/1f1a/0900766b800264a3.pdf> |
| OKI M82C51A-2 | USART | <https://www.cryptomuseum.com/crypto/rs/pu104/files/M82C51_datasheet.pdf> |
| HP 6N139 | Optocoupler (serial RX isolation) | <https://www.vishay.com/docs/83605/6n139.pdf> |

The Intel originals are the better reference for register-level behaviour:
search for the **Intel 8085AH**, **8155/8156**, and **8251A** datasheets. The
8251A one in particular has the mode/command word bit tables used throughout
[io-map.md](io-map.md).

## Still to find

- [ ] Any PAGE Ibérica **PI-T100** documentation: service manual, installation
      guide, protocol specification. PAGE Ibérica still exists as a display
      manufacturer in Tres Cantos, Madrid — worth an email.
- [ ] **Search the Telenorma PCB part numbers directly.** These are the most
      specific identifiers the unit carries, and unlike a product name they are
      unambiguous: `300-48.0216.5586` / `324-48.0216.5586` (motherboard),
      `300-40.0219.9078` / `324-40.0219.9078` (daughter 1), `41.0222.0026`,
      `11.0238.4030`, `37623.6`. Verify the digits against the board first — they
      were read from photographs. →
      [../hardware/pcbs.md](../hardware/pcbs.md#telenorma-part-numbers-c--but-verify-the-digits-against-the-board)
- [ ] **Telenorma / Telefonbau und Normalzeit / Bosch Telecom** display and clock
      system documentation. Both the EPROM label *and* the motherboard logo are
      Telenorma, so the hardware and firmware both originate there — see
      [device.md](device.md#dating-and-the-telenorma-connection-c-for-the-label).
      Search terms worth trying:
      - Telenorma *Anzeigetafel* / *Fahrgastinformation* (passenger information)
      - Telefonbau und Normalzeit *Nebenuhr* / *Hauptuhr* (slave / master clock)
      - Bosch Telecom station display protocol
      - German railway (DB) passenger-information display buses of the 1990s
- [ ] German clock and time-distribution **bus protocols** of the era. If the
      serial link descends from one, the framing decoded in
      [serial-protocol.md](serial-protocol.md) may be documented somewhere under
      a Telenorma or Bosch part number.
- [ ] RENFE Cercanías platform-signage installation or maintenance notes from
      the era.
- [ ] A slave unit, or photos of one — comparing master and slave firmware would
      settle the addressing question immediately.

## Live departure data, for the web replica

Surveyed 2026-08-10 while adding the replica's real-time tab. Recorded so the
same ground is not covered twice.

### What Renfe publishes openly

[data.renfe.com](https://data.renfe.com/) is a CKAN portal whose API answers
without authentication and, unlike the feeds themselves, is reachable from a
script. The Cercanías real-time feeds it points at:

| Feed | URL | Refresh | Carries |
| --- | --- | --- | --- |
| Vehicle positions | `https://gtfsrt.renfe.com/vehicle_positions.json` | 20 s | `tripId`, current `stopId`, `currentStatus`, GPS, **and the platform** |
| Trip updates | `https://gtfsrt.renfe.com/trip_updates.json` | 20 s | predicted arrival `time` and `delay` |
| Alerts | `https://gtfsrt.renfe.com/alerts.json` | 20 s | incidents |
| GTFS static | `https://ssl.renfe.com/ftransit/Fichero_CER_FOMENTO/fomento_transit.zip` | daily | 15.9 MB timetable |

**The platform is smuggled into `vehicle.vehicle.label`** rather than any GTFS
field — the dataset description gives the format as `C1-23537-PLATF.(3)`, and
live data agrees: `"label": "C1-23545-PLATF.(4)"`. Standard GTFS-RT has nowhere
to put a platform, so this is Renfe's own extension. It appears only once the
train is at or approaching the station, which is when the vía is actually
assigned.

Two limits found by reading the live feeds rather than the documentation:

- **`trip_updates` holds only the *next* stop.** Typically a single
  `stopTimeUpdate`, with no `stopSequence` and no `departure` object. The
  exception is skipped stops, which are listed. So the terminus cannot be read
  off the feed, and neither can a route.
- **Nothing in the RT feeds names a destination.** A departure board therefore
  needs the static GTFS underneath, with the live feed overlaid for delay and
  platform. `estaciones.csv` and the per-núcleo station lists carry no platform
  data at all.

### Why the replica cannot fetch any of it directly

None of `gtfsrt.renfe.com`, `ssl.renfe.com` or `info.adif.es` returns an
`Access-Control-Allow-Origin` header, and none sends `Vary: Origin` either,
which suggests CORS is simply unconfigured rather than origin-dependent. A page
served from GitHub Pages cannot `fetch()` them. `info.adif.es` additionally
answers **403** to non-browser clients and blocks cross-domain framing.

The exception, and the reason the replica's live tab speaks SignalR over a raw
WebSocket: **browsers do not apply CORS to the WebSocket handshake.** SignalR's
HTTP `negotiate` step can be skipped entirely when the WebSockets transport is
forced, and that path is open to a static cross-origin page where `fetch()` is
not.

### The ADIF hub [C]

Captured 2026-08-10 by driving a real browser at the page and reading the socket.
The WAF blocks headless Chrome and every scripted HTTP client (Akamai, per the
`errors.edgesuite.net` reference on its 403 page), so a headful browser was
needed to see any of this.

```
wss://info.adif.es/InfoStation
  ->  {"protocol":"json","version":1}
  <-  {}
  ->  {"type":1,"invocationId":"0","target":"JoinInfo","arguments":["PRO-ECM-17000"]}
  ->  {"type":1,"invocationId":"1","target":"GetLastMessage","arguments":["PRO-ECM-17000"]}
  <-  {"type":1,"target":"ReceiveMessage","arguments":["<JSON string, ~600 kB>"]}
```

**The connection token must be omitted, not invented.** ADIF's own app calls
`POST /InfoStation/negotiate?negotiateVersion=1` and connects with the `?id=` it
returns. Connecting with **no** `?id=` succeeds and receives live data; supplying
one that did not come from `negotiate` is refused with close code 1006. Verified
from `http://127.0.0.1:*`, an origin that is emphatically not ADIF's, so the hub
does not check `Origin`. This is what makes the live tab possible at all on
GitHub Pages.

`PRO-ECM-<stationCode>` is the group name and the prefix is not station-specific
— 17000 (Chamartín), 71801 (Barcelona Sants) and 60000 (Puerta de Atocha) all
answer. A bare code with no prefix is silently ignored, as is an unknown code, so
the client needs a receive timeout to tell "wrong station" from "still
connecting". Some codes return a server-side fault instead: 18000 currently
answers with a `HybridCache` `InvalidOperationException`.

One `ReceiveMessage` carries the station's whole board:

| Field | Meaning |
| --- | --- |
| `trains[]` | every train, ~385 at Chamartín — the whole day, not just the next few |
| `.departure_time` | ISO 8601, **already including the delay**; `countdown` agrees with it to the minute |
| `.platform` | the vía, empty until actually assigned (16 of 385 had one) |
| `.platform_preview` | a hint like `"4 · 5"`, *not* an assignment |
| `.delay_out` / `.delay_in` | minutes, and **can be negative** when running early |
| `.class_stop` | `origin` / `intermediate` / `destination` — the last means it terminates here, so it is an arrival |
| `.journey_stops_destination[]` | the calling points **after** this station, in order, each `{code, arrival_time}` — this is the "PARA EN: …" list. `journey_stops_origin` is its mirror, the stops already made. All 1 750 codes at Chamartín resolve against `stations`; 106 of 385 trains list none, being terminating services |
| `.commercial_id[].product` | `C03CERMAD` = line C-3 Madrid, `C4BCERMAD` = C-4B; long distance is its own name (AVE, ALVIA, IRYO…) |
| `stations{}` | code → `name`, a **ladder of progressively shorter forms**, longest first |

That name ladder is the useful surprise: ADIF publishes its own abbreviations
(`Bilbao Abando-Indalecio Prieto` … `Bilbao Abando-I. Pr.`), so a narrow display
can take the last entry rather than inventing abbreviations. It bottoms out
around 20-22 characters though, because their screens are wider than this
panel's 16 cells — 126 of the 185 stations Chamartín references reach 16, and
the other 59 have to be cut.

### Related projects

- [mariomnts/pantallas-estaciones](https://github.com/mariomnts/pantallas-estaciones)
  — configures ADIF's SIV screens by building a `gravita.html` URL and embedding
  it in an iframe. It never touches the API; its README notes ADIF blocks
  cross-domain framing, and its own workaround is to disable browser security.
- [mitren.app](https://mitren.app/) ("¿Dónde está mi tren?", by JotaEle) — shows
  Cercanías platforms and calling points. Server-rendered HTML on nginx with
  `X-Robots-Tag: noindex`, no JSON API and no CORS header, so it is not usable as
  a data source from a browser. Its station codes match ADIF's (`17000`
  Chamartín, `18000` Atocha), which is a useful confirmation that the code space
  is shared.
- [guardovich/renfe-realtime-map](https://github.com/guardovich/renfe-realtime-map)
  — consumes `vehicle_positions.json` behind a Flask proxy, which is the usual
  way around the CORS problem when a backend is available.

## Prior art worth reading

Nothing specific to this unit has been found. General flip-dot reverse
engineering write-ups (Luminator, Mobitec, BRose bus destination signs) are
useful for the *mechanical* drive side — coil pulse widths, H-bridge patterns,
why you must not leave a coil energised — but their protocols are unrelated.

Add anything found here, with a date and a one-line note on why it was useful.

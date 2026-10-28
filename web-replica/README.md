# Web replica

A browser replica of the display, for anyone without a panel of their own. It is
published at <https://oierlauzi.github.io/renfe-flipdot-re/> from this
directory, by [../.github/workflows/pages.yml](../.github/workflows/pages.yml).

| File | |
| --- | --- |
| `index.html` | The page people use: compose a message, pick an appearance, export |
| `display.html` | The panel itself. Its text comes from the URL, so it can be embedded |
| `display-realtime.html` | `display.html` with a live arrivals feed behind it |
| `cp437.js` | The code page, including the `ñ` and `Ñ` substitutions |
| `realtime.js` | The live departure sources |
| `support.js` | Generated runtime. Rebuilt from a separate `dc-runtime` project, not from here |
| `reference.svg` | Reference artwork |

The page is in Spanish with an English switch, like the controller's
configuration page and for the same reason.

## Live departures

Surveyed in August 2026. Recorded here so the same ground is not covered twice.

### What is published openly

[data.renfe.com](https://data.renfe.com/) is a CKAN portal whose API answers
without authentication. The Cercanías feeds it points at:

| Feed | Refresh | Carries |
| --- | --- | --- |
| `gtfsrt.renfe.com/vehicle_positions.json` | 20 s | Trip, current stop, GPS, and the platform |
| `gtfsrt.renfe.com/trip_updates.json` | 20 s | Predicted arrival time and delay |
| `gtfsrt.renfe.com/alerts.json` | 20 s | Incidents |
| `ssl.renfe.com/.../fomento_transit.zip` | daily | The static timetable, 16 MB |

The platform is smuggled into the vehicle label rather than any GTFS field,
in the form `C1-23545-PLATF.(4)`. Standard GTFS has nowhere to put a platform,
so this is Renfe's own extension. It appears only once the train is at or
approaching the station, which is when the platform is actually assigned.

Two limits, found by reading the live feeds rather than the documentation:

* **Trip updates hold only the next stop**, with no sequence and no departure
  time. The terminus cannot be read off the feed, and neither can a route.
* **Nothing in the real-time feeds names a destination.** A departure board
  therefore needs the static timetable underneath, with the live feed laid over
  it for delay and platform.

### Why the replica cannot fetch any of it

None of those hosts sends an `Access-Control-Allow-Origin` header, so a page
served from GitHub Pages cannot fetch them. ADIF's own host additionally answers
403 to anything that does not look like a real browser.

The exception, and the reason the live tab speaks SignalR over a raw WebSocket:
**browsers do not apply CORS to the WebSocket handshake.** The HTTP negotiate
step can be skipped entirely when the WebSocket transport is forced, and that
path is open to a static cross-origin page where `fetch()` is not.

### The ADIF hub

```
wss://info.adif.es/InfoStation
  ->  {"protocol":"json","version":1}
  <-  {}
  ->  {"type":1,"invocationId":"0","target":"JoinInfo","arguments":["PRO-ECM-17000"]}
  ->  {"type":1,"invocationId":"1","target":"GetLastMessage","arguments":["PRO-ECM-17000"]}
  <-  {"type":1,"target":"ReceiveMessage","arguments":["<JSON string, about 600 kB>"]}
```

**The connection token must be omitted, not invented.** ADIF's own app negotiates
and connects with the id it gets back. Connecting with no id at all succeeds and
receives live data. Supplying one that did not come from negotiate is refused.
Verified from a local origin, so the hub does not check `Origin` either.

`PRO-ECM-<stationCode>` is the group name, and the prefix is not
station-specific: Chamartín (17000), Barcelona Sants (71801) and Puerta de
Atocha (60000) all answer. An unknown code is silently ignored rather than
refused, so a client needs a receive timeout to tell "wrong station" from "still
connecting".

One message carries the station's whole board, which at Chamartín is about 385
trains, the entire day. The useful fields:

| Field | Meaning |
| --- | --- |
| `departure_time` | ISO 8601, already including the delay |
| `platform` | Empty until actually assigned |
| `platform_preview` | A hint like `"4 · 5"`, not an assignment |
| `delay_out` | Minutes, and can be negative when running early |
| `class_stop` | `destination` means it terminates here, so it is an arrival |
| `journey_stops_destination[]` | The calling points after this station, in order |
| `stations{}` | Code to name, as a ladder of progressively shorter forms |

That name ladder is the useful surprise. ADIF publishes its own abbreviations,
so a narrow display can take the last entry instead of inventing one. It bottoms
out around 20 characters, though, because their screens are wider than this
panel's 16 cells.

### Related projects

* [mariomnts/pantallas-estaciones](https://github.com/mariomnts/pantallas-estaciones)
  builds an ADIF screen URL and embeds it in an iframe. It never touches the
  API, and ADIF blocks cross-domain framing.
* [mitren.app](https://mitren.app/) shows Cercanías platforms and calling
  points. Server-rendered, no JSON API and no CORS header, so it is not usable
  as a data source from a browser. Its station codes match ADIF's.
* [guardovich/renfe-realtime-map](https://github.com/guardovich/renfe-realtime-map)
  consumes the vehicle positions feed behind a proxy, which is the usual way
  around the CORS problem when you have a backend.

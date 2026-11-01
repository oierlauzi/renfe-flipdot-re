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
| `realtime.js` | The live departure sources (mitren.app and a simulation), station search |
| `stations.js` | Generated station list. Rebuilt by [../tools/build_stations.py](../tools/build_stations.py) |
| `support.js` | Generated runtime. Rebuilt from a separate `dc-runtime` project, not from here |
| `reference.svg` | Reference artwork |

The page is in Spanish with an English switch, like the controller's
configuration page and for the same reason.

## Live departures

### Current source: mitren.app

Since November 2026 the live tab is fed by
[¿Dónde está mi tren?](https://github.com/jotaelediaz/dondeestamitren),
deployed at [mitren.app](https://mitren.app/). ADIF started restricting requests
to `info.adif.es`, and rather than keep working around that, the replica now
reads a project that already does the hard part: it lays Renfe's GTFS-RT feeds
over the static timetable server-side, which is exactly what a departure board
needs and what a static page cannot do (see below).

Its REST API under `/api/v1` sends no CORS header, so it is out of reach for
the same reason as everything else here. Its WebSocket is not, and the server
does not check `Origin`:

```
wss://mitren.app/api/ws/trains/madrid_cercanias?listing=0
  <-  {"type":"subscribed","nucleus":"madrid_cercanias","station_id":null,...}
  ->  {"type":"subscribe","station_id":"17000"}
  <-  {"type":"subscribed","nucleus":"madrid_cercanias","station_id":"17000"}
  <-  {"type":"arrivals_update","station_id":"17000","arrivals":[...]}
  ->  {"type":"ping"}
  <-  {"type":"pong","server_epoch":...}
```

The protocol is documented in `app/routers/trains_api.py` upstream, and the row
shape in `app/schemas/arrivals.py`. `listing=0` turns off the network-wide train
positions, the heaviest message and one the panel has no use for. The board is
pushed at once on subscribe and again on every server poll cycle.

What the panel takes from each row:

| Field | Use |
| --- | --- |
| `epoch` | Departure time, delay already included |
| `display_destination_name` | Destination, in full. Rows naming the station itself are trains ending their run there, and are dropped |
| `platform.value` | On the panel only when `platform.confirmed`, as a real sign would. The list shows the predicted one, or `pair`, greyed out |
| `line` | `short_name` (`C2`, `R4`…) and the line's colours, for the badges in the list |
| `is_realtime` | Realtime or timetable, marked in the list |
| `presence` | "Now" / "At platform" in the list; a countdown reaching zero is not taken as presence |
| `service_state` | `canceled` rows are dropped |

What changed against ADIF:

* **Cercanías and Rodalies only.** The 19 networks are listed in
  `Realtime.NETWORKS`. Long-distance trains are no longer shown, which suits a
  sign that only ever stood on Cercanías platforms.
* **A station needs its network.** Codes are still Renfe's five-digit ones, but
  the socket is per network. `display-realtime.html` takes `network=`, and
  without it looks the code up in `stations.js`, so existing embeds keep
  working.
* **No calling points.** The board does not list them, so the LED row stays
  dark. Train detail would have them, at the cost of extra subscriptions.
* **Up to 20 rows, at most 3 per line**, instead of the whole day.
* **No separate delay figure.**

### Station names

mitren.app's search API would turn "Sants" into `barcelona_rodalies/71801`, but
it has no CORS header either. So the names are baked into `stations.js`
instead, about 1,000 stations, generated from the data dondeestamitren itself
ships (Apache-2.0):

```sh
git clone --depth 1 https://github.com/jotaelediaz/dondeestamitren
python3 tools/build_stations.py dondeestamitren > web-replica/stations.js
```

That gives the page a search box (by name, town, mitren.app's aliases or code,
accents optional) instead of a bare code field, a link to each station on
mitren.app, and mitren.app's short names: the panel takes the longest form of
a destination that fits ("BILBAO-ABANDO" rather than "BILBAO-ABANDO-IND"),
which is what ADIF's name ladder used to provide. A code missing from the list
can still be typed in.

### Load on mitren.app

The server is a small VPS that caps WebSocket connections at 200 overall and
20 per IP, and drops a client silent for 150 s. The client keeps to one socket
per tab, pings once a minute, backs off with jitter, and waits at least a
minute when told the server is full.

### Survey of the official feeds

Done in August 2026, when the replica still spoke to ADIF directly. Recorded
here so the same ground is not covered twice.

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

### Why the replica could not fetch any of it

None of those hosts sends an `Access-Control-Allow-Origin` header, so a page
served from GitHub Pages cannot fetch them. ADIF's own host additionally answers
403 to anything that does not look like a real browser.

The exception, and the reason the live tab has always used a WebSocket:
**browsers do not apply CORS to the WebSocket handshake.** The HTTP negotiate
step can be skipped entirely when the WebSocket transport is forced, and that
path is open to a static cross-origin page where `fetch()` is not.

### The ADIF hub (no longer used)

This is what the live tab used until November 2026, when ADIF began
restricting requests to `info.adif.es`. The client is in the git history.

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
  points. It does have a JSON API, which the August survey missed, and its
  WebSocket is what the live tab now uses (see above). Its station codes
  match ADIF's.
* [guardovich/renfe-realtime-map](https://github.com/guardovich/renfe-realtime-map)
  consumes the vehicle positions feed behind a proxy, which is the usual way
  around the CORS problem when you have a backend.

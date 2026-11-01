/* realtime.js — live departure sources for the replica page.
 *
 * A "source" is anything with start(handlers) / stop(). It reports departures
 * through handlers.onDepartures and connection state through handlers.onStatus.
 *
 * A departure is a plain object:
 *   { destination, epoch, platform, line, train, delayMin, stops }
 * `epoch` is the departure time in seconds. Minutes-to-go are recomputed from
 * it locally on every tick rather than being read from the message, so a stale
 * feed still counts down correctly — only the delay figure goes stale, not the
 * clock.
 */
(function (global) {
  'use strict';

  // --- ¿Dónde está mi tren? (mitren.app) -----------------------------------
  //
  // https://github.com/jotaelediaz/dondeestamitren, deployed at mitren.app. It
  // blends Renfe's GTFS timetable with the GTFS-RT feeds server-side, so it
  // already does the work this page cannot: naming the destination, which the
  // real-time feeds never do (see README.md).
  //
  // Its REST API (/api/v1/…) sends no Access-Control-Allow-Origin header, so a
  // page on GitHub Pages cannot fetch() it. Its WebSocket can be used, for the
  // same reason the ADIF hub could: browsers do not apply CORS to the WebSocket
  // handshake, and the server does not check Origin. The board it pushes is
  // built by the same serializer as REST /arrivals, so nothing is lost.
  //
  // Protocol, from app/routers/trains_api.py upstream. Plain JSON frames:
  //   wss://mitren.app/api/ws/trains/<nucleus>?listing=0
  //   <-  {"type":"subscribed","nucleus":…,"station_id":null,…}   on connect
  //   ->  {"type":"subscriptions","station_ids":["17000"],"train_ids":[…]}
  //   <-  {"type":"subscriptions",…}                              the accepted set
  //   <-  {"type":"arrivals_update","station_id":"17000","arrivals":[…]}
  //       at once, then again on every poll cycle of the server
  //   <-  {"type":"train_update","train_id":…,"data":{"stops":[…],…}}
  //   <-  {"type":"train_removed","train_id":…}     not running (yet, or any more)
  //   ->  {"type":"ping"}   <-  {"type":"pong","server_epoch":…}
  //   <-  {"type":"error","message":…}
  // listing=0 opts out of the nucleus-wide train positions, which this page has
  // no use for and which are by far the heaviest message.
  //
  // The server drops a client that sends nothing for 150 s, and is a small VPS
  // with a global cap of 200 sockets, so: one socket per tab, one station, a
  // ping well inside the idle window, and a patient reconnect.
  const MITREN_WS = 'wss://mitren.app/api/ws/trains/';
  const PING_MS = 60000;
  // Not a health check: the first board is pushed on subscribe, but an unknown
  // station code is ignored silently, so this is what tells the two apart.
  const DATA_TIMEOUT_MS = 30000;

  // --- Networks and stations ----------------------------------------------
  //
  // From stations.js, which tools/build_stations.py generates out of the data
  // mitren.app itself ships: the search API that would answer this at runtime
  // has no CORS header. Station codes are Renfe's five-digit ones, the same
  // ADIF uses; the socket is per network, so a code travels with its network.
  const DATA = global.RealtimeData || {
    networks: [{ id: 'madrid_cercanias', name: 'Cercanías Madrid', city: 'madrid' }],
    stations: []
  };
  const NETWORKS = DATA.networks;
  const DEFAULT_NETWORK = 'madrid_cercanias';
  const networkById = new Map(NETWORKS.map(n => [n.id, n]));

  /* Case- and accent-insensitive form for matching. "Ñ" folds to "N" here,
     which is what someone typing on a keyboard without one expects. */
  function fold(text) {
    return String(text || '').normalize('NFD').replace(/[̀-ͯ]/g, '')
      .toLowerCase().replace(/[^a-z0-9]+/g, ' ').trim();
  }

  const STATIONS = DATA.stations.map(row => {
    const st = {
      network: row[0], code: row[1], name: row[2],
      short: row[3] || '', mini: row[4] || '', town: row[5] || '',
      aliases: row[6] ? row[6].split('|') : []
    };
    st.networkName = (networkById.get(st.network) || {}).name || st.network;
    st.haystack = fold([st.name, st.short, st.mini, st.town].concat(st.aliases).join(' '));
    st.foldedName = fold(st.name);
    return st;
  });
  const stationByKey = new Map(STATIONS.map(st => [st.network + '/' + st.code, st]));

  /* Every spelling of a station's name, longest first, keyed by each of them.
     A departure names its destination in full; the panel picks the longest
     form that fits, the way ADIF's own name ladder used to be read. */
  const ladderByName = new Map();
  for (const st of STATIONS) {
    const ladder = [st.name, st.short, st.mini].filter(Boolean);
    for (const form of ladder) {
      const key = fold(form);
      if (!ladderByName.has(key)) ladderByName.set(key, ladder);
    }
  }
  function nameLadder(name) {
    return ladderByName.get(fold(name)) || [String(name || '')];
  }

  function findStation(network, code) {
    return stationByKey.get(String(network) + '/' + String(code).trim()) || null;
  }

  /* A code on its own. Most codes belong to one network; where one is shared
     (a guest line), the default network wins, then the first listed. */
  function networkForCode(code) {
    const matches = STATIONS.filter(st => st.code === String(code).trim());
    if (!matches.length) return null;
    const home = matches.find(st => st.network === DEFAULT_NETWORK);
    return (home || matches[0]).network;
  }

  /* Ranked matches for what someone typed: a station code, or words from the
     name, its short forms, its town or mitren.app's search aliases, in any
     order and without accents. `network` floats that network's matches up. */
  function searchStations(query, options) {
    const prefer = options && options.network;
    const limit = (options && options.limit) || 8;
    const q = fold(query);
    if (!q) return [];
    const words = q.split(' ');
    const scored = [];
    for (const st of STATIONS) {
      let score;
      if (/^\d+$/.test(q)) {
        if (!st.code.startsWith(q)) continue;
        score = st.code === q ? 0 : 1;
      } else {
        if (!words.every(w => st.haystack.includes(w))) continue;
        if (st.foldedName.startsWith(q)) score = 0;
        else if ((' ' + st.foldedName).includes(' ' + words[0])) score = 1;
        else if (st.foldedName.includes(words[0])) score = 2;
        else score = 3;   // matched on town or alias only
      }
      if (prefer && st.network !== prefer) score += 0.5;
      scored.push([score, st]);
    }
    scored.sort((a, b) => a[0] - b[0] || a[1].name.localeCompare(b[1].name, 'es'));
    return scored.slice(0, limit).map(x => x[1]);
  }

  /* The station's page on mitren.app. The city/mode form is the public one;
     mitren.app redirects it to its canonical slug where those differ. */
  function stationUrl(network, code) {
    const net = networkById.get(network);
    if (!net || !/^\d+$/.test(String(code))) return 'https://mitren.app/';
    return 'https://mitren.app/' + encodeURIComponent(net.city) + '/cercanias/stations/' +
      encodeURIComponent(code);
  }

  /* A short trace for debugging from the console. */
  const lastFrames = [];
  function remember(msg, text) {
    lastFrames.push({ type: msg.type || null, chars: text.length, preview: text.slice(0, 500) });
    if (lastFrames.length > 25) lastFrames.shift();
  }

  // How many trains the server lets one socket follow, unless it says otherwise
  // in its first `subscribed`; and the least time between two changes of the
  // followed set, well inside its limit of 30 changes a minute.
  const DEFAULT_MAX_TRAINS = 4;
  const SUBSCRIBE_GAP_MS = 3000;

  function MitrenSource(options) {
    const station = String((options && options.station) || '').trim();
    const network = String((options && options.network) || DEFAULT_NETWORK).trim();
    const url = (options && options.url) ||
      MITREN_WS + encodeURIComponent(network) + '?listing=0';
    const here = findStation(network, station);
    let socket = null, pinger = null, retry = null, dataTimer = null;
    let attempts = 0, stopped = false;

    // Calling points. The board names no stops, but each train's own channel
    // does, so the source follows the few trains the page is about to show
    // (see focus()) and lays their remaining stops onto the board's rows.
    let maxTrains = DEFAULT_MAX_TRAINS;
    let wanted = [];          // train ids the page asked to follow, soonest first
    let followed = '';        // the set last sent, as a key
    let lastSentAt = 0;
    let holdUntil = 0;        // after a refusal, leave the set alone until then
    let subscribeTimer = null;
    let sendSubscriptions = null;   // set while a socket is open
    const stopsByTrain = new Map();
    let board = null;         // the last mapped board, before stops are laid on
    let emit = null;

    function withStops(rows) {
      return rows.map(row => {
        const stops = row.trainId ? stopsByTrain.get(row.trainId) : null;
        return stops ? Object.assign({}, row, { stops: stops }) : row;
      });
    }

    function syncSubscriptions() {
      clearTimeout(subscribeTimer);
      if (!sendSubscriptions) return;
      const ids = wanted.slice(0, maxTrains);
      // The same trains in another order are the same set to the server.
      const key = ids.slice().sort().join(',');
      if (key === followed) return;
      // The gap only spaces out changes to a set of trains; the first trains
      // after the station-only opening go at once.
      const gapFrom = followed ? lastSentAt : 0;
      const wait = Math.max(gapFrom + SUBSCRIBE_GAP_MS, holdUntil) - Date.now();
      if (wait > 0 && (followed || holdUntil > Date.now())) {
        subscribeTimer = setTimeout(syncSubscriptions, wait);
        return;
      }
      followed = key;
      lastSentAt = Date.now();
      for (const id of stopsByTrain.keys()) if (!ids.includes(id)) stopsByTrain.delete(id);
      sendSubscriptions(ids);
    }

    function open(handlers) {
      if (stopped) return;
      handlers.onStatus('connecting', null);
      let reported = false;   // a specific failure was surfaced; don't overwrite it
      let gotData = false;
      let busy = false;       // the server turned us away for load, not for a fault
      followed = '';
      emit = handlers.onDepartures;

      try {
        socket = new WebSocket(url);
      } catch (err) {
        // Malformed URL, or a mixed-content block on an http:// page.
        handlers.onStatus('error', err.message);
        return schedule(handlers, false);
      }
      const sock = socket;
      const send = (obj) => {
        if (sock.readyState === WebSocket.OPEN) sock.send(JSON.stringify(obj));
      };

      sock.addEventListener('open', () => {
        // The server subscribes the socket to the nucleus by itself and says so;
        // the station is asked for once that arrives (below), so an unknown
        // network surfaces as its own error rather than as a silent station.
        pinger = setInterval(() => send({ type: 'ping' }), PING_MS);
        dataTimer = setTimeout(() => {
          if (!gotData && !reported) handlers.onStatus('nodata', null);
        }, DATA_TIMEOUT_MS);
      });

      sock.addEventListener('message', (event) => {
        if (typeof event.data !== 'string') return;
        let msg;
        try { msg = JSON.parse(event.data); } catch (e) { return; }
        if (!msg || typeof msg !== 'object') return;
        remember(msg, event.data);

        switch (msg.type) {
          case 'subscribed':
            if (msg.station_id == null && !sendSubscriptions) {
              attempts = 0;
              if (Number(msg.max_trains) > 0) maxTrains = Number(msg.max_trains);
              // `subscriptions` replaces the whole set, station and trains, and
              // pushes the board of a newly added station at once.
              sendSubscriptions = (ids) => send({
                type: 'subscriptions', station_ids: [station], train_ids: ids
              });
              followed = null;   // force the first send
              lastSentAt = 0;
              syncSubscriptions();
            }
            return;
          case 'arrivals_update': {
            if (String(msg.station_id) !== station) return;
            const departures = mapMitren(msg, here);
            if (!departures) return;
            board = departures;
            gotData = true;
            reported = false;
            clearTimeout(dataTimer);
            handlers.onStatus('live', null);
            handlers.onDepartures(withStops(board));
            return;
          }
          case 'train_update': {
            const id = String(msg.train_id || '');
            if (!id || !wanted.includes(id)) return;
            const stops = remainingStops(msg.data, here, station);
            if (stops) stopsByTrain.set(id, stops); else stopsByTrain.delete(id);
            if (board) handlers.onDepartures(withStops(board));
            return;
          }
          case 'train_removed':
            // Not running yet, or finished: its stops are not known live.
            if (stopsByTrain.delete(String(msg.train_id || '')) && board) {
              handlers.onDepartures(withStops(board));
            }
            return;
          case 'error': {
            const text = String(msg.message || 'server error');
            // Following trains is a nicety. A refused change of subscriptions
            // (rate limit, malformed set) must not take the board down with it:
            // forget what was sent so the next sync tries again.
            if (/subscription|rate limit/i.test(text)) {
              if (global.console) global.console.warn('realtime: ' + text);
              followed = null;
              holdUntil = Date.now() + 20000;
              syncSubscriptions();
              return;
            }
            reported = true;
            busy = /busy|too many connections/i.test(text);
            handlers.onStatus('error', text);
            return;
          }
          default:
            return;   // pong, `subscriptions` echo, anything newer than this page
        }
      });

      sock.addEventListener('close', (event) => {
        clearInterval(pinger);
        clearTimeout(dataTimer);
        clearTimeout(subscribeTimer);
        if (sock !== socket) return;
        sendSubscriptions = null;
        if (stopped) return;
        if (!reported) handlers.onStatus('error', 'connection lost (' + event.code + ')');
        // 1013 is "try again later": the server is full. Don't add to it.
        schedule(handlers, busy || event.code === 1013);
      });

      sock.addEventListener('error', () => { /* 'close' always follows; report there */ });
    }

    function schedule(handlers, longWait) {
      if (stopped) return;
      const base = Math.min(60000, 2000 * Math.pow(2, attempts++));
      const delay = longWait ? Math.max(base, 60000) : base;
      // Jitter, so that a server restart is not met by every tab at once.
      retry = setTimeout(() => open(handlers), delay * (0.75 + Math.random() * 0.5));
    }

    return {
      start(handlers) { stopped = false; attempts = 0; open(handlers); },
      stop() {
        stopped = true;
        sendSubscriptions = null;
        clearInterval(pinger);
        clearTimeout(retry);
        clearTimeout(dataTimer);
        clearTimeout(subscribeTimer);
        if (socket) { try { socket.close(1000); } catch (e) { /* already closing */ } socket = null; }
      },
      /* The departures the page is showing, in the order it will show them.
         The first few (the server allows 4) are followed for their calling
         points. Unchanged lists cost nothing, so this can run on every tick. */
      focus(departures) {
        wanted = [];
        for (const dep of departures || []) {
          if (dep && dep.trainId && !wanted.includes(dep.trainId)) wanted.push(dep.trainId);
        }
        syncSubscriptions();
      }
    };
  }

  /* The stops a train still has to make after `station`, as the names to
     announce. From a train_update's data.stops (TrainStop in
     app/schemas/train_detail.py upstream). Null when the station is not on
     the train's list, which means nothing trustworthy can be said. */
  function remainingStops(data, here, station) {
    const stops = data && Array.isArray(data.stops) ? data.stops : null;
    if (!stops) return null;
    const at = stops.findIndex(s => s && (String(s.station_id) === station || String(s.stop_id) === station));
    if (at < 0) return null;
    return stops.slice(at + 1)
      .filter(s => {
        const state = s.times && s.times.display && s.times.display.service_state;
        return !s.skipped_side && state !== 'skipped' && state !== 'canceled';
      })
      .map(s => s.short_name || s.name || (findStation(here ? here.network : '', s.station_id) || {}).name)
      .filter(Boolean);
  }

  /* One arrivals_update carries the station's next trains (up to 20, at most
     3 per line), realtime where the server has it and timetable otherwise.
     Field meanings are documented in app/schemas/arrivals.py upstream.
     `here` is the station being shown, when stations.js knows it. */
  function mapMitren(msg, here) {
    if (!Array.isArray(msg.arrivals)) return null;
    // It is an arrivals board, so trains that end their run here are on it
    // too. A departure sign does not announce those.
    const ownNames = new Set(here ? [here.name, here.short, here.mini].filter(Boolean).map(fold) : []);
    const rows = [];
    for (const row of msg.arrivals) {
      if (!row || row.service_state === 'canceled') continue;
      const epoch = Number(row.epoch);
      if (!Number.isFinite(epoch) || epoch <= 0) continue;
      const destination = row.display_destination_name || row.destination_name;
      if (!destination || ownNames.has(fold(destination))) continue;

      const platform = row.platform || null;
      const line = row.line || {};
      const identity = row.identity || {};
      const confirmed = !!(platform && platform.confirmed && platform.value);
      rows.push({
        destination: String(destination),
        epoch: epoch,
        // Only a platform the operator has actually given goes on the panel.
        // mitren.app also predicts one from habit, which a real sign would not
        // show; the list on the page shows it greyed out as `platformGuess`.
        platform: confirmed ? String(platform.value) : null,
        platformGuess: !confirmed && platform
          ? (platform.pair && platform.pair.length ? platform.pair.join('/') : (platform.value || null))
          : null,
        line: line.short_name || row.short_name || '',
        lineColor: line.color_bg || null,
        lineTextColor: line.color_fg || null,
        train: identity.public_number || null,
        // The live train's id, which its own channel is keyed by. Only a train
        // already running has one; a timetabled row has a locator instead.
        trainId: row.is_realtime && identity.technical_id ? String(identity.technical_id) : null,
        realtime: !!row.is_realtime,
        // at_station / arriving, resolved by mitren.app from positive evidence.
        presence: row.presence || null,
        isOrigin: !!row.is_origin,
        // The board folds the delay into `epoch` rather than reporting it.
        delayMin: null,
        // Filled in from the train's own channel once it is followed.
        stops: []
      });
    }
    return rows;
  }

  // --- Demo source ---------------------------------------------------------
  //
  // Synthetic, and labelled as such in the UI. It exists so the tab stays
  // exercisable — layout, truncation, countdown, platform filtering — when the
  // live feed is unreachable, and so the page has something to show offline.
  const DEMO_DESTINATIONS = [
    { destination: 'ALCALA HENARES', line: 'C2' },
    { destination: 'GUADALAJARA', line: 'C2' },
    { destination: 'CHAMARTIN', line: 'C1' },
    { destination: 'PRINCIPE PIO', line: 'C7' },
    { destination: 'COLMENAR VIEJO', line: 'C4' },
    { destination: 'TRES CANTOS', line: 'C4' },
    { destination: 'ARANJUEZ', line: 'C3' },
    { destination: 'PARLA', line: 'C4' },
    { destination: 'FUENLABRADA', line: 'C5' },
    { destination: 'HUMANES', line: 'C5' },
    { destination: 'VILLALBA', line: 'C8' },
    { destination: 'EL ESCORIAL', line: 'C8' },
    { destination: 'ATOCHA', line: 'C1' },
    { destination: 'AEROPUERTO T4', line: 'C1' }
  ];

  function DemoSource(options) {
    let timer = null;
    const platforms = options && options.platforms ? options.platforms : [1, 2, 3, 4];
    // Seeded from the station code so each station shows its own stable board
    // rather than the same trains under a different name, and from the hour so
    // it changes over a session without re-rolling on every tick.
    const stationSeed = String((options && options.station) || '0')
      .split('').reduce((h, c) => (h * 31 + c.charCodeAt(0)) >>> 0, 7);
    let seed = stationSeed + Math.floor(Date.now() / 3600000);

    function build() {
      const now = Math.floor(Date.now() / 1000);
      const rows = [];
      for (let i = 0; i < 6; i++) {
        // Stride must be coprime with the table length, or the board repeats
        // the same destination every few rows.
        const pickIdx = (seed + i * 5) % DEMO_DESTINATIONS.length;
        const base = DEMO_DESTINATIONS[pickIdx];
        const delayMin = ((seed + i * 13) % 7 === 0) ? ((seed + i) % 9) + 1 : 0;
        rows.push({
          destination: base.destination,
          line: base.line,
          train: String(10000 + ((seed + i * 137) % 89999)),
          platform: String(platforms[(seed + i) % platforms.length]),
          epoch: now + 120 + i * 240 + ((seed + i * 31) % 90),
          delayMin: delayMin,
          stops: [1, 2, 3].map(k =>
            DEMO_DESTINATIONS[(pickIdx + k * 3) % DEMO_DESTINATIONS.length].destination)
        });
      }
      return rows;
    }

    return {
      start(handlers) {
        handlers.onStatus('live', 'demo');
        const tick = () => {
          // Retire departures that have gone and top the board back up, so the
          // list drains and refills the way a real one does.
          const now = Math.floor(Date.now() / 1000);
          const rows = build().filter(r => r.epoch > now - 30);
          if (rows.length < 3) seed += 1;
          handlers.onDepartures(rows);
        };
        tick();
        timer = setInterval(tick, 5000);
      },
      stop() { clearInterval(timer); timer = null; }
    };
  }

  // --- Turning departures into panel parameters ----------------------------
  //
  // Shared by index.html (the configurator) and display-realtime.html (the
  // embeddable panel) so the two cannot drift apart.
  const PANEL_CELLS = 16;

  /* CP437 has no accented capitals, and display.html already substitutes them,
     but folding here keeps any on-screen list and the panel in agreement.
     U+0303 is spared so that Ñ survives — it is the one accented capital the
     ROM does carry, at 0xA5. */
  function foldToPanel(text) {
    return String(text)
      .toUpperCase()
      .normalize('NFD')
      .replace(/[\u0300-\u0302\u0304-\u036f]/g, '')
      .normalize('NFC');
  }

  /* A vía may arrive as "4", "04" or "PLATF.(4)"; the sign shows the number. */
  function platformDigits(platform) {
    if (platform == null) return null;
    const m = String(platform).match(/\d+/);
    return m ? String(Number(m[0])) : null;
  }

  /* The LED matrix announces where the train calls. Terminating services carry
     no onward stops, and there is nothing to announce for them. */
  function callingPoints(dep) {
    const stops = (dep.stops || []).map(foldToPanel).filter(Boolean);
    return stops.length ? 'PARA EN: ' + stops.join(', ') : '';
  }

  /* Departures worth showing, soonest first, with anything already gone
     dropped. Platform matching is loose on purpose: the feed may render a vía
     as "4", "04" or "PLATF.(4)". Cap the list anyway — a sign shows the next
     few. */
  function selectDepartures(departures, options) {
    const wanted = String((options && options.platform) || '').trim();
    const max = (options && options.max) || 12;
    const now = Date.now() / 1000;
    return (departures || [])
      .filter(d => d.epoch > now - 30)
      .filter(d => {
        if (!wanted) return true;
        const digits = platformDigits(d.platform);
        return digits ? Number(digits) === Number(wanted) : false;
      })
      .sort((a, b) => a.epoch - b.epoch)
      .slice(0, max);
  }

  /* The four display.html parameters for one departure. */
  function panelFields(dep, options) {
    const showMinutes = !options || options.showMinutes !== false;
    const override = String((options && options.platform) || '').trim();
    const mins = String(minutesTo(dep.epoch));
    // display.html writes the minutes into the last cells of the same 16-cell
    // row, so the destination must stop short of them or it gets overwritten
    // ("BILBAO ABANDO-I." + "12" renders as "BILBAO ABANDO-12"). One blank cell
    // between the two keeps them legible.
    const room = showMinutes ? Math.max(1, PANEL_CELLS - mins.length - 1) : PANEL_CELLS;
    // Names arrive in full. Take the longest of mitren.app's own forms that
    // fits ("BILBAO-ABANDO-INDALECIO PRIETO" -> "BILBAO-ABANDO"), and only cut
    // the shortest when none does; a trailing hyphen or space is an artefact
    // of that cut, so it goes too.
    const ladder = nameLadder(dep.destination).map(foldToPanel);
    const fitting = ladder.find(form => form.length <= room);
    return {
      text: (fitting || ladder[ladder.length - 1]).slice(0, room).replace(/[\s-]+$/, ''),
      min: mins,
      // A platform filter means the sign is bolted to that platform, so it
      // always reads that number. Otherwise show the assignment if there is
      // one, and leave it blank if there is not — which is what the real signs
      // do before a platform has been allocated.
      via: override || platformDigits(dep.platform) || '',
      led: callingPoints(dep)
    };
  }

  /* A sign with no train scheduled goes dark — but one bolted to a platform
     still says which platform it is, so a selected vía stays lit even with
     nothing to announce. */
  function blankPanel(options) {
    return {
      text: '',
      min: '',
      via: String((options && options.platform) || '').trim(),
      led: ''
    };
  }

  /* Minutes until `epoch`, floored at 0. The panel shows whole minutes, and a
     train that is due now reads as 0 rather than going negative. */
  function minutesTo(epoch) {
    return Math.max(0, Math.round((epoch - Date.now() / 1000) / 60));
  }

  global.Realtime = {
    MitrenSource: MitrenSource,
    DemoSource: DemoSource,
    mapMitren: mapMitren,
    remainingStops: remainingStops,
    lastFrames: lastFrames,
    MITREN_WS: MITREN_WS,
    NETWORKS: NETWORKS,
    DEFAULT_NETWORK: DEFAULT_NETWORK,
    STATIONS: STATIONS,
    findStation: findStation,
    networkForCode: networkForCode,
    searchStations: searchStations,
    stationUrl: stationUrl,
    nameLadder: nameLadder,
    PANEL_CELLS: PANEL_CELLS,
    blankPanel: blankPanel,
    foldToPanel: foldToPanel,
    platformDigits: platformDigits,
    callingPoints: callingPoints,
    selectDepartures: selectDepartures,
    panelFields: panelFields,
    minutesTo: minutesTo
  };
})(window);

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
  //   ->  {"type":"subscribe","station_id":"17000"}
  //   <-  {"type":"subscribed","station_id":"17000"}
  //   <-  {"type":"arrivals_update","station_id":"17000","arrivals":[…]}
  //       at once, then again on every poll cycle of the server
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

  /* Renfe's Cercanías and Rodalies networks as mitren.app names them. Station
     codes are Renfe's five-digit ones, the same that ADIF uses, so a code is
     only meaningful together with its network. */
  const NETWORKS = [
    { id: 'madrid_cercanias', name: 'Cercanías Madrid' },
    { id: 'barcelona_rodalies', name: 'Rodalies Barcelona' },
    { id: 'valencia_rodalia', name: 'Rodalia València' },
    { id: 'sevilla_cercanias', name: 'Cercanías Sevilla' },
    { id: 'malaga_cercanias', name: 'Cercanías Málaga' },
    { id: 'cadiz_cercanias', name: 'Cercanías Cádiz' },
    { id: 'bilbao_aldiriak', name: 'Bilbao Aldiriak' },
    { id: 'donostia_aldiriak', name: 'Donostia Aldiriak' },
    { id: 'cantabria_cercanias', name: 'Cercanías Cantabria' },
    { id: 'asturias_cercanias', name: 'Cercanías Asturias' },
    { id: 'zaragoza_cercanias', name: 'Cercanías Zaragoza' },
    { id: 'murcia_cercanias', name: 'Cercanías Murcia' },
    { id: 'alacant_cercanias', name: 'Cercanías Alacant' },
    { id: 'cartagena_cercanias', name: 'Cercanías Cartagena' },
    { id: 'tarragona_rodalies', name: 'Rodalies Tarragona' },
    { id: 'girona_rodalies', name: 'Rodalies Girona' },
    { id: 'lleida_rodalies', name: 'Rodalies Lleida' },
    { id: 'leon_cercanias', name: 'Cercanías León' },
    { id: 'ferrol_cercanias', name: 'Cercanías Ferrol' }
  ];
  const DEFAULT_NETWORK = 'madrid_cercanias';

  /* A short trace for debugging from the console. */
  const lastFrames = [];
  function remember(msg, text) {
    lastFrames.push({ type: msg.type || null, chars: text.length, preview: text.slice(0, 500) });
    if (lastFrames.length > 25) lastFrames.shift();
  }

  function MitrenSource(options) {
    const station = String((options && options.station) || '').trim();
    const network = String((options && options.network) || DEFAULT_NETWORK).trim();
    const url = (options && options.url) ||
      MITREN_WS + encodeURIComponent(network) + '?listing=0';
    let socket = null, pinger = null, retry = null, dataTimer = null;
    let attempts = 0, stopped = false;

    function open(handlers) {
      if (stopped) return;
      handlers.onStatus('connecting', null);
      let reported = false;   // a specific failure was surfaced; don't overwrite it
      let gotData = false;
      let busy = false;       // the server turned us away for load, not for a fault

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
        // the station is asked for once that arrives (below). Sending it now
        // would also work, but this way an unknown network surfaces as its own
        // error rather than as a silent station.
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
            if (msg.station_id == null) {
              attempts = 0;
              send({ type: 'subscribe', station_id: station });
            }
            return;
          case 'arrivals_update': {
            if (String(msg.station_id) !== station) return;
            const departures = mapMitren(msg);
            if (!departures) return;
            gotData = true;
            reported = false;
            clearTimeout(dataTimer);
            handlers.onStatus('live', null);
            handlers.onDepartures(departures);
            return;
          }
          case 'error':
            reported = true;
            busy = /busy|too many/i.test(String(msg.message || ''));
            handlers.onStatus('error', String(msg.message || 'server error'));
            return;
          default:
            return;   // pong, and anything newer than this page
        }
      });

      sock.addEventListener('close', (event) => {
        clearInterval(pinger);
        clearTimeout(dataTimer);
        if (stopped || sock !== socket) return;
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
        clearInterval(pinger);
        clearTimeout(retry);
        clearTimeout(dataTimer);
        if (socket) { try { socket.close(1000); } catch (e) { /* already closing */ } socket = null; }
      }
    };
  }

  /* One arrivals_update carries the station's next trains (up to 20, at most
     3 per line), realtime where the server has it and timetable otherwise.
     Field meanings are documented in app/schemas/arrivals.py upstream. */
  function mapMitren(msg) {
    if (!Array.isArray(msg.arrivals)) return null;
    const rows = [];
    for (const row of msg.arrivals) {
      if (!row || row.service_state === 'canceled') continue;
      const epoch = Number(row.epoch);
      if (!Number.isFinite(epoch) || epoch <= 0) continue;
      const destination = row.display_destination_name || row.destination_name;
      if (!destination) continue;

      const platform = row.platform || null;
      const line = row.line || {};
      const identity = row.identity || {};
      rows.push({
        destination: String(destination),
        epoch: epoch,
        // Only a platform the operator has actually given. mitren.app also
        // predicts one from habit ("confirmed": false), which is useful in its
        // app but is not what a real sign shows: it stays blank until assigned.
        platform: platform && platform.confirmed && platform.value ? String(platform.value) : null,
        line: line.short_name || row.short_name || '',
        train: identity.public_number || null,
        // The board carries the delay already folded into `epoch`, not as a
        // separate figure.
        delayMin: null,
        // Nor does it carry calling points, so the LED row stays dark.
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
    return {
      // Station names arrive in full, and many Cercanías destinations fit 16
      // cells as they are. The rest simply get cut; a trailing hyphen or space
      // is an artefact of that cut, so it goes too.
      text: foldToPanel(dep.destination).slice(0, room).replace(/[\s-]+$/, ''),
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
    lastFrames: lastFrames,
    MITREN_WS: MITREN_WS,
    NETWORKS: NETWORKS,
    DEFAULT_NETWORK: DEFAULT_NETWORK,
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

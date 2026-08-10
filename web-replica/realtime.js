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
 * clock. ADIF's own `countdown` field agrees with this to the minute.
 */
(function (global) {
  'use strict';

  // --- SignalR JSON hub protocol, spoken directly over a raw WebSocket ------
  //
  // Not @microsoft/signalr, for two reasons. The page ships no dependencies,
  // and more importantly the library's default path begins with an HTTP POST
  // to `<hub>/negotiate` — a fetch, and therefore subject to CORS, which a
  // cross-origin static page would lose. Browsers do NOT apply CORS to the
  // WebSocket handshake, so going straight to wss:// is the only transport
  // available to a page served from anywhere but the hub's own origin. This is
  // what the library calls `skipNegotiation: true`.
  //
  // Verified against ADIF on 2026-08-10 from a foreign origin: connecting to
  // the hub path with NO `?id=` succeeds and receives live data. Supplying an
  // `?id=` that did not come from `negotiate` is refused with close code 1006 —
  // so the token must be omitted, not invented.
  //
  // Wire format: UTF-8 JSON records terminated by 0x1e.
  //   ->  {"protocol":"json","version":1}          handshake request
  //   <-  {}                                       handshake ack ({"error"} on failure)
  //   <-> {"type":6}                               ping
  //   <-  {"type":1,"target":"M","arguments":[…]}  server calling the client
  //   ->  {"type":1,"invocationId":"0",
  //        "target":"M","arguments":[…]}           client calling the hub
  //   <-  {"type":3,"invocationId":"0","error":…}  completion, possibly failed
  const RS = '\x1e'; // written as an escape: a literal 0x1e in source is too easy to mangle
  const PING = { type: 6 };
  const PING_MS = 15000;
  const DATA_TIMEOUT_MS = 20000;

  function encode(obj) { return JSON.stringify(obj) + RS; }

  function decode(buffer) {
    // Returns [messages, remainder] — a socket frame may split or coalesce records.
    const parts = buffer.split(RS);
    const remainder = parts.pop();
    const messages = [];
    for (const part of parts) {
      if (!part) continue;
      try { messages.push(JSON.parse(part)); } catch (e) { /* skip malformed record */ }
    }
    return [messages, remainder];
  }

  /* A short trace for debugging from the console. Only a preview of each
     message is kept: ADIF's payload is well over half a megabyte, and holding
     25 of them verbatim would cost more memory than the rest of the page. */
  const lastFrames = [];
  function remember(msg) {
    const text = JSON.stringify(msg);
    lastFrames.push({
      type: msg.type,
      target: msg.target || null,
      chars: text.length,
      preview: text.slice(0, 500)
    });
    if (lastFrames.length > 25) lastFrames.shift();
  }

  function SignalRSource(options) {
    const url = options.url;
    const invocations = options.subscribe || [];   // [{target, arguments}, …] sent once connected
    const map = options.map;
    let socket = null, pinger = null, retry = null, dataTimer = null;
    let attempts = 0, stopped = false;

    function open(handlers) {
      if (stopped) return;
      handlers.onStatus('connecting', null);
      let buffer = '';
      let handshakeDone = false;
      let reported = false;   // a specific failure was surfaced; don't overwrite it
      let gotData = false;

      try {
        socket = new WebSocket(url);
      } catch (err) {
        // Malformed URL, or a mixed-content block on an http:// page.
        handlers.onStatus('error', err.message);
        return schedule(handlers);
      }

      socket.addEventListener('open', () => {
        socket.send(encode({ protocol: 'json', version: 1 }));
      });

      socket.addEventListener('message', (event) => {
        buffer += typeof event.data === 'string' ? event.data : '';
        let messages;
        [messages, buffer] = decode(buffer);

        for (const msg of messages) {
          if (!handshakeDone) {
            handshakeDone = true;
            if (msg.error) {
              // The hub rejected the handshake itself (protocol/version mismatch).
              reported = true;
              handlers.onStatus('error', msg.error);
              socket.close();
              return;
            }
            attempts = 0;
            handlers.onStatus('connecting', null);
            pinger = setInterval(() => {
              if (socket && socket.readyState === WebSocket.OPEN) socket.send(encode(PING));
            }, PING_MS);
            // A hub that accepts the connection but never pushes looks identical
            // to a healthy one, so give the first message a deadline. An unknown
            // station code lands here.
            dataTimer = setTimeout(() => {
              if (!gotData && !reported) handlers.onStatus('nodata', null);
            }, DATA_TIMEOUT_MS);
            invocations.forEach((inv, i) => {
              socket.send(encode({
                type: 1, invocationId: String(i),
                target: inv.target, arguments: inv.arguments || []
              }));
            });
            continue;
          }

          if (msg.type === 6) continue;                                  // ping
          if (msg.type === 3) {                                          // completion
            if (msg.error) { reported = true; handlers.onStatus('error', msg.error); }
            continue;
          }
          if (msg.type === 7) {
            reported = true;
            handlers.onStatus('error', msg.error || 'closed by server');
            continue;
          }
          if (msg.type !== 1 && msg.type !== 2) continue;

          remember(msg);
          const departures = map(msg);
          if (departures) {
            gotData = true;
            reported = false;
            clearTimeout(dataTimer);
            handlers.onStatus('live', null);
            handlers.onDepartures(departures);
          }
        }
      });

      socket.addEventListener('close', (event) => {
        clearInterval(pinger);
        clearTimeout(dataTimer);
        if (stopped) return;
        // 1006 before the handshake is the signature of a rejected Origin, a
        // bad connection token, or a path that is not a hub.
        if (!reported) {
          handlers.onStatus('error', handshakeDone
            ? 'connection lost (' + event.code + ')'
            : 'refused before handshake (' + event.code + ')');
        }
        schedule(handlers);
      });

      socket.addEventListener('error', () => { /* 'close' always follows; report there */ });
    }

    function schedule(handlers) {
      if (stopped) return;
      const delay = Math.min(30000, 1000 * Math.pow(2, attempts++));
      retry = setTimeout(() => open(handlers), delay);
    }

    return {
      start(handlers) { stopped = false; attempts = 0; open(handlers); },
      stop() {
        stopped = true;
        clearInterval(pinger);
        clearTimeout(retry);
        clearTimeout(dataTimer);
        if (socket) { try { socket.close(); } catch (e) { /* already closing */ } socket = null; }
      }
    };
  }

  // --- ADIF ----------------------------------------------------------------
  //
  // Protocol captured from info.adif.es on 2026-08-10:
  //   wss://info.adif.es/InfoStation
  //   -> JoinInfo("PRO-ECM-<stationCode>")
  //   -> GetLastMessage("PRO-ECM-<stationCode>")
  //   <- ReceiveMessage(<JSON string>)   repeated as the board changes
  // The group prefix is not station-specific: PRO-ECM-17000 (Chamartín),
  // PRO-ECM-71801 (Barcelona Sants) and PRO-ECM-60000 (Puerta de Atocha) all
  // answer, while a bare code with no prefix is silently ignored.
  const ADIF_HUB = 'wss://info.adif.es/InfoStation';
  const ADIF_GROUP_PREFIX = 'PRO-ECM-';

  function AdifSource(options) {
    const group = ADIF_GROUP_PREFIX + String((options && options.station) || '').trim();
    return SignalRSource({
      url: (options && options.url) || ADIF_HUB,
      subscribe: [
        { target: 'JoinInfo', arguments: [group] },
        { target: 'GetLastMessage', arguments: [group] }
      ],
      map: mapAdif
    });
  }

  /* One ReceiveMessage carries the station's whole board — settings, every
     train, and a station-code lookup table — as a JSON *string* argument. */
  function mapAdif(msg) {
    if (msg.type !== 1 || msg.target !== 'ReceiveMessage') return null;
    let payload;
    try { payload = JSON.parse(msg.arguments[0]); } catch (e) { return null; }
    if (!payload || !Array.isArray(payload.trains)) return null;

    const stations = payload.stations || {};
    const rows = [];
    for (const train of payload.trains) {
      // A train whose journey ends here is an arrival, not a departure.
      if (train.class_stop === 'destination') continue;
      const when = train.departure_time || train.arrival_time;
      const epoch = when ? Date.parse(when) / 1000 : NaN;
      if (!Number.isFinite(epoch)) continue;

      const destination = (train.destinations || [])
        .map(d => stationName(stations, d.code))
        .filter(Boolean)
        .join(' / ');
      if (!destination) continue;

      rows.push({
        destination: destination,
        epoch: epoch,
        // Empty until the platform is actually assigned, which is what the real
        // signs show too. platform_preview ("4 · 5") is a hint, not an
        // assignment, so it is deliberately not used here.
        platform: train.platform || null,
        line: lineLabel(train),
        train: trainNumber(train),
        delayMin: Number(train.delay_out || train.delay_in || 0) || null,
        // The calling points after this station, in order — what the LED
        // matrix announces as "PARA EN: …". journey_stops_origin is the
        // mirror image, the stops already made, and is not wanted here.
        stops: (train.journey_stops_destination || [])
          .map(s => stationName(stations, s.code))
          .filter(Boolean)
      });
    }
    return rows;
  }

  /* stations[code].name is a list of variants from longest to shortest, e.g.
     ["València Joaquín Sorolla", "València Joaquín Sorolla",
      "València Joaq. Sorolla"]. The flip-dot row is 16 cells, so take the
     shortest one ADIF itself publishes rather than truncating a long one. */
  function stationName(stations, code) {
    const entry = stations[code];
    if (!entry) return code || '';
    const names = entry.name;
    if (Array.isArray(names) && names.length) return String(names[names.length - 1]);
    return typeof names === 'string' ? names : (code || '');
  }

  /* Cercanías products are coded C<line>CER<núcleo> — "C03CERMAD" is line C-3
     in Madrid, "C4BCERMAD" is C-4B. Long-distance products are already their
     own name (AVE, ALVIA, AVANT, IRYO), so those pass through untouched. */
  function lineLabel(train) {
    const commercial = (train.commercial_id || [])[0] || {};
    const product = commercial.product || '';
    const cercanias = /^C(\d{1,2}[A-Z]?)CER/.exec(product);
    if (cercanias) return 'C' + cercanias[1].replace(/^0+(?=\d)/, '');
    return product || train.traffic_type || '';
  }

  function trainNumber(train) {
    const commercial = (train.commercial_id || [])[0] || {};
    const numbers = commercial.numbers || [];
    return numbers[0] || train.technical_number_out || train.technical_number_planif || null;
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

  global.Realtime = {
    SignalRSource: SignalRSource,
    AdifSource: AdifSource,
    DemoSource: DemoSource,
    mapAdif: mapAdif,
    lastFrames: lastFrames,
    ADIF_HUB: ADIF_HUB,
    /* Minutes until `epoch`, floored at 0. The panel shows whole minutes, and a
       train that is due now reads as 0 rather than going negative. */
    minutesTo(epoch) {
      return Math.max(0, Math.round((epoch - Date.now() / 1000) / 60));
    }
  };
})(window);

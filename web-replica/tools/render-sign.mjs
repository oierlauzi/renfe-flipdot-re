#!/usr/bin/env node
// Renders the static sign SVG (for the README) from the original font map.
//
//   node tools/render-sign.mjs --text "RENFE-FLIPDOT-RE" --min "" --via 1 --led "" --out sign.svg
//
// The glyph table is parsed from font-map.txt, so that file is the single
// source of truth for the font — no duplicated bitmaps anywhere in the repo.

import { readFileSync, writeFileSync } from 'node:fs';

const argv = process.argv.slice(2);
const arg = (name, dflt) => {
  const i = argv.indexOf('--' + name);
  return i === -1 ? dflt : argv[i + 1];
};

const P = {
  fontMap: arg('font-map', 'uploads/font-map.txt'),
  text: arg('text', 'RENFE-FLIPDOT-RE'),
  minutes: arg('min', ''),
  platform: arg('via', '1'),
  led: arg('led', ''),
  lit: arg('lit', '#d9e04a'),
  ledLit: arg('led-lit', '#ff3b2e'),
  out: arg('out', 'sign.svg')
};

// --- font -------------------------------------------------------------------
const GLYPHS = {};
for (const line of readFileSync(P.fontMap, 'utf8').split('\n')) {
  const m = line.match(/^0x([0-9A-Fa-f]{2})\s+.*?@0x[0-9A-Fa-f]+\s+([0-9a-f]{2}(?: [0-9a-f]{2}){4})/);
  if (m) GLYPHS[parseInt(m[1], 16)] = m[2].split(' ').join('');
}
// code page 437 for the characters a Spanish sign actually needs
const CP = { 'á': 0xA0, 'í': 0xA1, 'ó': 0xA2, 'ú': 0xA3, 'ñ': 0xA4, 'Ñ': 0xA5, '¿': 0xA8, '¡': 0xAD, 'é': 0x82, 'ü': 0x81, 'ç': 0x87, 'Á': 0x41, 'É': 0x45, 'Í': 0x49, 'Ó': 0x4F, 'Ú': 0x55 };
const code = (ch) => CP[ch] ?? (ch.charCodeAt(0) < 256 ? ch.charCodeAt(0) : 0x20);

// --- geometry (mirrors display.html) --------------------------------
const PITCH = 11, DOT = 4.3, GAP = 6, COLS = 5, ROWS = 7, CELLS = 16;
const LW = 1040, LH = 95, LP = 12, LROWS = 7;
const f = (n) => +n.toFixed(2);

const cells = String(P.text).slice(0, CELLS).split('');
while (cells.length < CELLS) cells.push(' ');
if (P.minutes) {
  const m = String(P.minutes);
  for (let i = 0; i < m.length; i++) cells[CELLS - m.length + i] = m[i];
}

let flip = '';
for (let i = 0; i < CELLS; i++) {
  const hex = GLYPHS[code(cells[i] || ' ')];
  const cx0 = i * (COLS * PITCH + GAP);
  for (let c = 0; c < COLS; c++) {
    const byte = hex ? parseInt(hex.slice(c * 2, c * 2 + 2), 16) : 0;
    for (let r = 0; r < ROWS; r++) {
      const on = (byte >> r) & 1;
      const x = cx0 + c * PITCH + PITCH / 2 - DOT, y = r * PITCH + PITCH / 2 - DOT;
      const s = DOT * 2, k = DOT * 0.5;
      const pts = [[x + k, y], [x + s - k, y], [x + s, y + k], [x + s, y + s - k], [x + s - k, y + s], [x + k, y + s], [x, y + s - k], [x, y + k]]
        .map((p) => f(p[0]) + ',' + f(p[1])).join(' ');
      flip += `<polygon points="${pts}" fill="${on ? P.lit : '#26261e'}"/>` +
        `<line x1="${f(x + 0.4)}" y1="${f(y + s - 0.4)}" x2="${f(x + s - 0.4)}" y2="${f(y + 0.4)}" stroke="${on ? 'rgba(0,0,0,0.32)' : 'rgba(255,255,255,0.07)'}" stroke-width="0.7"/>`;
    }
  }
}

const bmp = [];
for (const ch of String(P.led)) {
  const hex = GLYPHS[code(ch)];
  for (let c = 0; c < COLS; c++) bmp.push(hex ? parseInt(hex.slice(c * 2, c * 2 + 2), 16) : 0);
  bmp.push(0);
}
const lcols = Math.floor((LW - 12) / LP);
const pad = Math.max(0, Math.floor((lcols - bmp.length) / 2));
const lx0 = (LW - lcols * LP) / 2, ly0 = (LH - LROWS * LP) / 2;
let led = '';
for (let c = 0; c < lcols; c++) {
  const byte = bmp[c - pad] || 0;
  for (let r = 0; r < LROWS; r++) {
    const on = (byte >> r) & 1;
    led += `<circle cx="${f(lx0 + c * LP + LP / 2)}" cy="${f(ly0 + r * LP + LP / 2)}" r="${on ? 3.1 : 2.3}" fill="${on ? P.ledLit : '#40101a'}"/>`;
  }
}

const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="520" viewBox="0 0 1200 520" font-family="Helvetica Neue, Helvetica, Arial, sans-serif">
<rect width="1200" height="520" rx="58" fill="#24221f"/>
<rect x="14" y="14" width="1172" height="492" rx="46" fill="#1a1917"/>
<g transform="translate(34,34)">
  <clipPath id="glass"><rect width="1132" height="452" rx="30"/></clipPath>
  <g clip-path="url(#glass)">
    <rect width="1132" height="452" rx="30" fill="#0d1210"/>
    <g transform="translate(60,21) scale(0.45667)"><circle cx="150" cy="150" r="150" fill="rgb(239,44,48)"/><path d="M150,100C177.429,100 200,122.571 200,150C200,177.429 177.429,200 150,200C122.571,200 100,177.429 100,150L50,150C50,204.858 95.142,250 150,250C204.858,250 250,204.858 250,150C250,95.142 204.858,50 150,50L150,100Z" fill="#fff"/></g>
    <text x="286" y="145" fill="#f4f2e9" font-size="156" font-weight="700" letter-spacing="1">Vía</text>
    <text x="561" y="145" fill="#f4f2e9" font-size="156" font-weight="700" letter-spacing="1">${P.platform}</text>
    <text x="1092" y="71" fill="#f4f2e9" font-size="56" text-anchor="end">Próxima</text>
    <text x="1092" y="138" fill="#f4f2e9" font-size="56" text-anchor="end">Llegada</text>
    <rect x="70.5" y="178.5" width="991" height="94" fill="#080a08" stroke="#34342c"/>
    <g transform="translate(81,187)">${flip}</g>
    <text x="1072" y="232" fill="${P.lit}" font-size="30" font-weight="500">min</text>
    <rect x="46" y="307" width="${LW}" height="${LH}" rx="2" fill="#2c0a11"/>
    <g transform="translate(46,307)">${led}</g>
  </g>
</g>
</svg>
`;
writeFileSync(P.out, svg);
console.error(`wrote ${P.out} (${svg.length} bytes)`);

// Doi chieu firmware (C) voi trang web (JS): bang ma, bo cuc layer 3, ma phim, va thuat toan tren chuoi su kien ngau nhien.
"use strict";
const fs = require('fs'), path = require('path'), vm = require('vm'), { execFileSync } = require('child_process');
const root = path.join(__dirname, '..', 'firmware');
const html = fs.readFileSync(path.join(root, 'numpad21.html'), 'utf8');
const ino = fs.readFileSync(path.join(root, 'numpad21', 'numpad21.ino'), 'utf8');
const hdr = fs.readFileSync(path.join(root, 'numpad21', 'morse.h'), 'utf8');
let fails = 0, checks = 0;
const check = (c, msg) => { checks++; if (!c) { fails++; console.log('FAIL: ' + msg); } };

// --- nap engine JS tu trang web (giua 2 dau) cung cac hang so no dung ---
const eng = /\/\/ <morse-engine>([\s\S]*?)\/\/ <\/morse-engine>/.exec(html);
if (!eng) { console.log('FAIL: khong thay <morse-engine> trong numpad21.html'); process.exit(1); }
const pre = ['const KC_TRNS = 1;', /const CONS = [^\n]*/.exec(html)[0]].join('\n');   // dung dung dong khai bao that trong trang
const led = /\/\/ <led-engine>([\s\S]*?)\/\/ <\/led-engine>/.exec(html);
if (!led) { console.log('FAIL: khong thay <led-engine> trong numpad21.html'); process.exit(1); }
const js = vm.runInNewContext(pre + eng[1] + led[1] + '\n({ MORSE_TAB, MORSE_MAX, MORSE_LAYOUT, KC_MORSE, morseNew, morseCommit, morsePress, morseFind, LYN, TLK, ledFrame, NLED })', {});

// --- 1) bang ma ---
const cTab = [...hdr.matchAll(/\{"([.\-]+)",\s*'((?:\\.|[^'\\])+)'\}/g)].map(m => [m[2].replace(/^\\(.)$/, '$1'), m[1]]);
check(cTab.length === js.MORSE_TAB.length && cTab.length === 54, 'so muc bang: C ' + cTab.length + ' JS ' + js.MORSE_TAB.length);
cTab.forEach((e, i) => check(js.MORSE_TAB[i] && js.MORSE_TAB[i][0] === e[0] && js.MORSE_TAB[i][1] === e[1], 'bang ma muc ' + i + ': C ' + JSON.stringify(e) + ' JS ' + JSON.stringify(js.MORSE_TAB[i])));
const hm = /#define MORSE_MAX (\d+)/.exec(hdr); check(+hm[1] === js.MORSE_MAX, 'MORSE_MAX C ' + hm[1] + ' JS ' + js.MORSE_MAX);

// --- 2) ma phim ---
for (const [n, k] of [['DOT', 'dot'], ['DASH', 'dash'], ['OK', 'ok']]) {
  const v = parseInt(new RegExp('#define KC_MORSE_' + n + ' (0x[0-9A-Fa-f]+)').exec(ino)[1], 16);
  check(v === js.KC_MORSE[k], 'KC_MORSE_' + n + ' C 0x' + v.toString(16) + ' JS 0x' + js.KC_MORSE[k].toString(16));
}
check(/\{\s*0x7060,\s*'MorseDot'|add\(0x7060, 'MorseDot'/.test(html) && /add\(0x7061, 'MorseDash'/.test(html) && /add\(0x7062, 'MorseOk'/.test(html), 'danh muc phim cua trang thieu ma Morse');

// --- 3) bo cuc layer 3 mac dinh: C (n3) == JS (MORSE_LAYOUT) ---
const n3 = /static const uint16_t n3\[NK\] = \{([\s\S]*?)\};/.exec(ino)[1];
const sym = { 'KC_TRNS': 1, 'LYN': 0x4200, 'KC_MORSE_DOT': 0x7060, 'KC_MORSE_DASH': 0x7061, 'KC_MORSE_OK': 0x7062 };
const cl = n3.split(',').map(t => t.trim()).filter(Boolean).map(t => { let m; if ((m = /^TL\((\d+)\)$/.exec(t))) return 0x4100 | +m[1]; if (t in sym) return sym[t]; if (/^(0x[0-9a-f]+|\d+)$/i.test(t)) return Number(t); throw new Error('token la trong n3: ' + t); });
check(cl.length === 22 && js.MORSE_LAYOUT.length === 22, 'n3 co ' + cl.length + ' phan tu');
check(cl.every((v, i) => v === js.MORSE_LAYOUT[i]), 'bo cuc layer 3: C ' + JSON.stringify(cl.map(x => x.toString(16))) + ' != JS ' + JSON.stringify(js.MORSE_LAYOUT.map(x => x.toString(16))));
check(/for \(int l = 4; l <= 5; l\+\+\)/.test(ino) && /KM\(3, i\) = n3\[i\]/.test(ino), 'defaults() chua dat layer 3 = n3');

// --- 3b) LED: so luong che do, ma phim, tung khung hinh C == JS ---
const nfx = +/#define NFX (\d+)/.exec(ino)[1], fxN = (html.match(/^const FX = \[(.*)\];$/m)[1].match(/\['/g) || []).length;
check(nfx === 16 && fxN === nfx, 'so che do LED: firmware ' + nfx + ', trang web ' + fxN);
check(+/#define KC_LED_SET (0x[0-9A-Fa-f]+)/.exec(ino)[1].length > 0 && parseInt(/#define KC_LED_SET (0x[0-9A-Fa-f]+)/.exec(ino)[1], 16) === 0x7020 && /add\(0x7020 \+ i,/.test(html), 'ma phim LED 0x7020 giua firmware va trang web');
check(/#define LNL (\d+)/.exec(ino)[1] === /const LNL = (\d+)/.exec(html)[1], 'do dai ten layer C != JS');
check(/#define FW_VER 13/.test(ino), 'FW_VER phai la 13');
const ledC = execFileSync(path.join(__dirname, 'build', 'fw_sim'), ['--led']).toString().trim().split('\n');
for (const line of ledC) {
  const [mm, rest] = line.split(':'), mode = +mm, frames = rest.trim().split(' ').map(Number);
  if (mode === 8 || mode === 16) { check(frames.every(f => f <= 63 && (mode === 8 || (f & (f - 1)) === 0)), 'che do ngau nhien ' + mode + ' ngoai 6 bit/khong phai 1 den'); continue; }
  check(frames.every((f, t) => js.ledFrame(mode, t) === f), 'che do LED ' + mode + ': khung hinh C != JS');
  checks += frames.length;
}

// --- 4) thuat toan: chuoi su kien ngau nhien chay tren C va JS, so sanh may tinh sau tung su kien ---
let seed = 0x9E3779B9 >>> 0; const rnd = () => { seed ^= seed << 13; seed >>>= 0; seed ^= seed >>> 17; seed ^= seed << 5; seed >>>= 0; return seed; };
let stream = ''; for (let i = 0; i < 60000; i++) { const r = rnd() % 20; stream += r < 8 ? 'd' : r < 15 ? 'h' : r < 18 ? 'o' : r < 19 ? 's' : 'b'; }
const cOut = execFileSync(path.join(__dirname, 'build', 'morse_test'), ['--stream'], { input: stream, maxBuffer: 1 << 28 }).toString().split('\n');
const m = js.morseNew(); let host = ''; const io = { room: () => 99, bs() { host = host.slice(0, -1); }, put(ch) { host += ch; } };
let bad = 0;
for (let i = 0; i < stream.length; i++) {
  const e = stream[i];
  if (e === 'd' || e === 'h') js.morsePress(m, e === 'h', io); else if (e === 'o') js.morseCommit(m);
  else if (e === 's') { js.morseCommit(m); host += ' '; } else { js.morseCommit(m); host = host.slice(0, -1); }
  const mine = host.length + '|' + host.slice(-40);
  if (cOut[i] !== mine) { if (bad++ < 3) console.log('FAIL: su kien #' + i + ' (' + e + '): C ' + JSON.stringify(cOut[i]) + ' JS ' + JSON.stringify(mine)); }
}
check(bad === 0, bad + ' su kien C va JS khac nhau'); checks += stream.length;
console.log(checks + ' doi chieu C<->JS, ' + fails + ' loi');
process.exit(fails ? 1 : 0);

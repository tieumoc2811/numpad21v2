// Doi chieu firmware (C) voi trang web (JS): bang ma, bo cuc layer 3, ma phim, va thuat toan tren chuoi su kien ngau nhien.
"use strict";
const fs = require('fs'), path = require('path'), vm = require('vm'), { execFileSync } = require('child_process');
const root = path.join(__dirname, '..', 'firmware');
const fwdir = path.join(root, 'numpad21');
const html = fs.readFileSync(path.join(root, 'numpad21.html'), 'utf8');
const src = fs.readFileSync(path.join(fwdir, 'numpad21.ino'), 'utf8');
const hdr = fs.readFileSync(path.join(fwdir, 'morse.h'), 'utf8');
let fails = 0, checks = 0;
const check = (c, msg) => { checks++; if (!c) { fails++; console.log('FAIL: ' + msg); } };

const eng = /\/\/ <morse-engine>([\s\S]*?)\/\/ <\/morse-engine>/.exec(html);
if (!eng) { console.log('FAIL: khong thay <morse-engine> trong numpad21.html'); process.exit(1); }
const pre = ['const KC_TRNS = 1;', /const CONS = [^\n]*/.exec(html)[0]].join('\n');   // dung dung dong khai bao that trong trang (MO, TL, TD, NKK, CONS)
const js = vm.runInNewContext(pre + eng[1] + '\n({ morseLedFrame, MORSE_LED, MORSE_TAB, MORSE_MAX, MORSE_HINT_MAX, MORSE_OPT, MORSE_LAYOUT, KC_MORSE, morseNew, morseCommit, morseCommitIO, morseSetHint, morsePress, morseFind })', {});

// 1) bang ma
const cTab = [...hdr.matchAll(/\{"([.\-]+)",\s*'((?:\\.|[^'\\])+)'\}/g)].map(m => [m[2].replace(/^\\(.)$/, '$1'), m[1]]);
check(cTab.length === js.MORSE_TAB.length && cTab.length === 54, 'so muc bang: C ' + cTab.length + ' JS ' + js.MORSE_TAB.length);
cTab.forEach((e, i) => check(js.MORSE_TAB[i] && js.MORSE_TAB[i][0] === e[0] && js.MORSE_TAB[i][1] === e[1], 'bang ma muc ' + i));
check(+/#define MORSE_MAX (\d+)/.exec(hdr)[1] === js.MORSE_MAX, 'MORSE_MAX C != JS');
check(+/#define MORSE_HINT_MAX (\d+)/.exec(hdr)[1] === js.MORSE_HINT_MAX, 'MORSE_HINT_MAX C != JS');
check(+/#define MORSE_HINT_DEF (\d)/.exec(src)[1] === (js.MORSE_OPT.hint ? 1 : 0), 'goi y mac dinh: MORSE_HINT_DEF (C) != MORSE_OPT.hint (JS)');

// 2) ma phim + danh muc phim cua trang
for (const [n, k] of [['DOT', 'dot'], ['DASH', 'dash'], ['OK', 'ok'], ['HINT', 'hint'], ['LED', 'led']]) {
  const v = parseInt(new RegExp('#define KC_MORSE_' + n + ' (0x[0-9A-Fa-f]+)').exec(src)[1], 16);
  check(v === js.KC_MORSE[k], 'KC_MORSE_' + n + ' C 0x' + v.toString(16) + ' JS 0x' + js.KC_MORSE[k].toString(16));
}
check([0x7060, 0x7061, 0x7062, 0x7063, 0x7064].every(c => new RegExp('add\\(0x' + c.toString(16) + ',[^)]*\'Morse\'').test(html)), 'danh muc phim cua trang thieu ma Morse');
check(+/#define FW_VER (\d+)/.exec(src)[1] >= 13 && /info\.ver < 13/.test(html), 'FW_VER >= 12 va trang canh bao firmware < 12');
check(/IS_MORSE\(k\) \(\(k\) >= KC_MORSE_DOT && \(k\) <= KC_MORSE_LED\)/.test(src) && /c <= KC_MORSE\.hint\) return true/.test(html), 'IS_MORSE (C) / hasMorse (JS) chua gom phim Morse');
check(+/#define MORSE_LED_DEF (\d)/.exec(src)[1] === 1 && js.MORSE_LED.defOn === true, 'den Morse mac dinh: MORSE_LED_DEF (C) != JS');
for (const [n, k] of [['NONE', 'none'], ['ALL', 'all'], ['DOT_MS', 'dot'], ['DASH_MS', 'dash'], ['ACK_MS', 'ack'], ['HOLD_MS', 'hold']]) check(+new RegExp('#define MORSE_LED_' + n + ' (0x[0-9A-Fa-f]+|\\d+)').exec(hdr)[1] === js.MORSE_LED[k], 'MORSE_LED_' + n + ' C != JS');

// 3) bo cuc layer 3 mac dinh: C (mo[] trong defaults) == JS (MORSE_LAYOUT)
const mo = /static const uint16_t mo\[NK\] = \{([\s\S]*?)\};/.exec(src)[1].replace(/\/\/[^\n]*/g, '');
const NKc = +/#define NK (\d+)/.exec(fs.readFileSync(path.join(fwdir, 'board.h'), 'utf8'))[1];
const sym = { 'LYN': 0x4200, 'KC_TRNS': 1, 'KC_MORSE_DOT': 0x7060, 'KC_MORSE_DASH': 0x7061, 'KC_MORSE_OK': 0x7062, 'KC_MORSE_HINT': 0x7063 };
const cl = mo.split(',').map(t => t.trim()).filter(Boolean).map(t => { let m; if ((m = /^TL\((\d+)\)$/.exec(t))) return 0x4100 | +m[1]; if ((m = /^TD\((\d+)\)$/.exec(t))) return 0x5000 | +m[1]; if (t in sym) return sym[t]; if ((m = /^KC_LATCH \+ (\d)$/.exec(t))) return 0x7001 + +m[1]; if (/^(0x[0-9a-f]+|\d+)$/i.test(t)) return Number(t); throw new Error('token la trong mo[]: ' + t); });
check(cl.length === NKc && js.MORSE_LAYOUT.length === NKc && NKc === 22, 'mo[] co ' + cl.length + ' phan tu, NK = ' + NKc);
check(cl.every((v, i) => v === js.MORSE_LAYOUT[i]), 'bo cuc layer 3: C ' + JSON.stringify(cl.map(x => x.toString(16))) + ' != JS ' + JSON.stringify(js.MORSE_LAYOUT.map(x => x.toString(16))));
check(/KM\(3, i\) = mo\[i\]/.test(src) && /w\[K\(3, i\)\] = MORSE_LAYOUT\[i\]/.test(html), 'defaults() cua C hoac JS chua dat layer 3 = bo cuc Morse');
// ten layer co phim Morse: trang web hien MORSE
check(/hasMorse\(l\) \? 'MORSE'/.test(html), 'layerLabel cua trang khong hien MORSE');

// 4) thuat toan: chuoi su kien ngau nhien chay tren C va JS, so sanh may tinh sau tung su kien (1 lan tat goi y, 1 lan bat goi y + bat/tat giua chung)
let seed = 0x9E3779B9 >>> 0; const rnd = () => { seed ^= seed << 13; seed >>>= 0; seed ^= seed >>> 17; seed ^= seed << 5; seed >>>= 0; return seed; };
function runStream(withHint) {
  let stream = ''; for (let i = 0; i < 60000; i++) { const r = rnd() % 22; stream += r < 8 ? 'd' : r < 15 ? 'h' : r < 18 ? 'o' : r < 19 ? 's' : r < 20 ? 'b' : (withHint && r < 21) ? 't' : 'd'; }
  const cOut = execFileSync(path.join(__dirname, 'build', 'morse_test'), withHint ? ['--stream', '--hint'] : ['--stream'], { input: stream, maxBuffer: 1 << 28 }).toString().split('\n');
  js.MORSE_OPT.hint = withHint;
  const m = js.morseNew(); let host = ''; const io = { room: () => 99, bs() { host = host.slice(0, -1); }, put(ch) { host += ch; } };
  let bad = 0, maxHint = 0;
  for (let i = 0; i < stream.length; i++) {
    const e = stream[i];
    if (e === 'd' || e === 'h') js.morsePress(m, e === 'h', io); else if (e === 'o') js.morseCommitIO(m, io); else if (e === 't') js.morseSetHint(m, !js.MORSE_OPT.hint, io);
    else if (e === 's') { js.morseCommitIO(m, io); host += ' '; } else { js.morseCommitIO(m, io); host = host.slice(0, -1); }
    maxHint = Math.max(maxHint, m.hint);
    const mine = host.length + '|' + host.slice(-40);
    if (cOut[i] !== mine) { if (bad++ < 3) console.log('FAIL: (goi y ' + (withHint ? 'bat' : 'tat') + ') su kien #' + i + ' (' + e + '): C ' + JSON.stringify(cOut[i]) + ' JS ' + JSON.stringify(mine)); }
  }
  check(bad === 0, bad + ' su kien C va JS khac nhau (goi y ' + (withHint ? 'bat' : 'tat') + ')'); checks += stream.length;
  check(withHint ? maxHint === js.MORSE_HINT_MAX : maxHint === 0, 'do dai goi y lon nhat = ' + maxHint);
}
// 5) den LED bao Morse: C (morse_led_frame) va JS (morseLedFrame) tren cung luoi (len, dash, since_press, since_ack)
{
  const cOut = execFileSync(path.join(__dirname, 'build', 'morse_test'), ['--ledframes'], { maxBuffer: 1 << 28 }).toString().split('\n'); let i = 0, bad = 0;
  for (let len = 0; len <= js.MORSE_MAX; len++) for (let d = 0; d < 2; d++) for (let sp = 0; sp < 3000; sp += 7) for (let sa = 0; sa < 400; sa += 13) { const v = js.morseLedFrame(len, !!d, sp, sa); if (+cOut[i++] !== v && bad++ < 3) console.log('FAIL: den LED len=' + len + ' dash=' + d + ' sp=' + sp + ' sa=' + sa + ': C ' + cOut[i - 1] + ' JS ' + v); checks++; }
  check(bad === 0 && i > 10000, bad + ' khung den C va JS khac nhau (' + i + ' khung)');
}
// 6) 16 che do LED: C (led_frame) va JS (ledFrame), tru 2 che do ngau nhien (8, 16: chi kiem tra tap hop gia tri hop le)
{
  const le = /\/\/ <led-engine>([\s\S]*?)\/\/ <\/led-engine>/.exec(html);
  check(!!le, 'khong thay <led-engine> trong numpad21.html');
  const lj = vm.runInNewContext(le[1] + '\n({ ledFrame, NLED })', { Math });
  const cOut = execFileSync(path.join(__dirname, 'build', 'fw_sim'), [], { env: { LEDFRAMES: '1' }, maxBuffer: 1 << 24 }).toString().split('\n'); let i = 0, bad = 0;
  check(+/#define NFX (\d+)/.exec(src)[1] === 16 && /const FX = \[/.test(html), 'NFX (C) != 16');
  for (let m = 1; m <= 16; m++) for (let t = 0; t < 200; t++) { const c = +cOut[i++], j = lj.ledFrame(m, t); checks++; if (m === 8) { if (c > 63 || j > 63) bad++; } else if (m === 16) { if (!c || (c & (c - 1)) || c > 63 || !j || (j & (j - 1)) || j > 63) bad++; } else if (c !== j && bad++ < 3) console.log('FAIL: den LED mode ' + m + ' t=' + t + ': C ' + c + ' JS ' + j); }
  check(bad === 0, bad + ' khung den LED C va JS khac nhau');
}
runStream(false); runStream(true);
console.log(checks + ' doi chieu C<->JS, ' + fails + ' loi');
process.exit(fails ? 1 : 0);

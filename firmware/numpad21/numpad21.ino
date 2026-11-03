// Numpad21 - firmware tu viet cho Pro Micro nRF52840 (nice!nano v2), thay ban ZMK cu.
// Logic giong Macropad15: 16 layer, tap dance, combo, latch, Nokia multitap, 32 macro, cau hinh bang WebHID (numpad21.html).
// Hai che do ket noi: USB (TinyUSB) va BLE (Bluefruit, 5 profile). Cau hinh/raw HID chi qua USB.
// Phan cung giu nguyen: ma tran 7x7 co diode, 6 day LED (S8550), nut layer nho. Xem board.h.
#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <string.h>
#include "board.h"

using namespace Adafruit_LittleFS_Namespace;

#define L 16                       // so layer toi da (so layer dang dung nam trong cfg)
#define BASE_TD (L * NK)           // 4 tap dance x 3 o (tap, hold, 2x tap)
#define BASE_CB (BASE_TD + 12)     // 4 combo x 3 o (A, B, ket qua)
#define NLI (BASE_CB + 12)         // o luu so layer dang dung
#define LDB (NLI + 1)              // khoi LED
#define NLD 4
#define NW (LDB + NLD)
enum { LD_MODE, LD_TICK, LD_BAT, LD_IND };   // che do LED 1..9, ms moi buoc, sang ca khi chay pin (0/1), nhay bao layer (0/1)
#define LD(i) cfg.w[LDB + (i)]
#define NFX 9
#define MAGIC 0x4E323131u
#define FW_VER 11

#define KC_TRNS 1
#define CONS(x) (0x8000 | (x))
#define MO(n) (0x4000 | (n))
#define TL(n) (0x4100 | (n))       // toggle layer n (TL(0) = tat het toggle)
#define LYN 0x4200                 // layer ke tiep (vong), LYN+1 = layer truoc
#define TD(n) (0x5000 | (n))
#define NKK(i) (0x6000 | (i))
#define KC_LATCH 0x7001            // 0x7002..0x7005 = toggle Space/Shift/Ctrl/Alt
#define KC_LED_NEXT 0x7010
#define KC_LED_PREV 0x7011
#define KC_OUT_TOG 0x7030          // doi USB <-> BLE
#define KC_OUT_USB 0x7031
#define KC_OUT_BLE 0x7032
#define KC_BT_CLR 0x7033           // xoa toan bo ghep doi BLE
#define KC_BT_SEL 0x7040           // 0x7040..0x7044 = profile BLE 0..4
#define NBT 5
#define KC_MACRO 0x7100            // 0x7100 + n
#define KC_BOOT 0x7F00             // vao bootloader UF2
#define CYC_N 6                    // &LYN chi di vong qua layer 0..5 (layer 6 = FN bi bo qua)
#define TAP_TERM 200
#define COMBO_TERM 30
#define NK_TERM 600
#define SLEEP_MS 900000UL          // 15 phut khong dung (khi khong cam USB) -> ngu sau
#define NMAC 32
#define MSLOT 252                  // byte moi o macro (252: 32 macro + 4 byte magic = 8068 B, vua 2 khoi flash 4 KB): [0]=lap [1,2]=nghi ms [3]=do dai ten [4..15]=ten [16,17]=do dai buoc [18..]=buoc
#define MH 8
#define CFG_FILE "/n21cfg"
#define MAC_FILE "/n21mac"
#define ST_FILE "/n21st"
#define MAC_MAGIC 0x3143414Du
#define ST_MAGIC 0x3154534Eu
_Static_assert(NW < 1024, "cfg qua lon");

static struct { uint32_t magic; uint16_t w[NW]; } cfg;
#define KM(l, p) cfg.w[(l) * NK + (p)]
#define TDW(n, k) cfg.w[BASE_TD + (n) * 3 + (k)]
#define CB(n, k) cfg.w[BASE_CB + (n) * 3 + (k)]

static uint32_t ms(void) { return millis(); }

// ---------------- cau hinh mac dinh (giong keymap ZMK cu) ----------------
static void defaults(void) {
  memset(&cfg, 0, sizeof cfg); cfg.magic = MAGIC; cfg.w[NLI] = 7;
  LD(LD_MODE) = 2; LD(LD_TICK) = 100; LD(LD_BAT) = 0; LD(LD_IND) = 1;
  for (int l = 1; l < L; l++) for (int i = 0; i < NK; i++) KM(l, i) = KC_TRNS;
  // layer 0: Numpad
  static const uint16_t n0[NK] = {
    0x29, 0x2B, 0x2A, MO(6),
    0x53, 0x54, 0x55, 0x56,
    0x5F, 0x60, 0x61,
    0x5C, 0x5D, 0x5E, 0x57,
    0x59, 0x5A, 0x5B, 0x58,
    0x62, 0x63,
    LYN };
  // layer 1: Media / dieu huong
  static const uint16_t n1[NK] = {
    CONS(0xB6), CONS(0xCD), CONS(0xB5), KC_TRNS,
    CONS(0xE2), CONS(0xEA), CONS(0xE9), CONS(0x192),
    0x4A, 0x52, 0x4D,
    0x50, 0x51, 0x4F, 0x4B,
    0x49, 0x4C, 0x4E, 0x28,
    TL(0), KC_TRNS,
    LYN };
  // layer 2: Nokia multitap (* = giu Shift, / = Caps Lock, + = Backspace)
  static const uint16_t n2[NK] = {
    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
    0, 0x39, KC_LATCH + 2, 0x2D,
    NKK(6), NKK(7), NKK(8),
    NKK(3), NKK(4), NKK(5), 0x2A,
    NKK(0), NKK(1), NKK(2), 0x28,
    NKK(9), NKK(10),
    LYN };
  // layer 6: FN (giu phim FN)
  static const uint16_t n6[NK] = {
    KC_BOOT, LYN, KC_OUT_TOG, KC_TRNS,
    0, KC_BT_SEL + 0, KC_BT_SEL + 1, KC_BT_SEL + 2,
    0, 0, 0,
    0, 0, 0, KC_BT_SEL + 3,
    KC_BT_CLR, 0, 0, KC_BT_SEL + 4,
    KC_OUT_USB, KC_LED_NEXT,
    KC_TRNS };
  for (int i = 0; i < NK; i++) { KM(0, i) = n0[i]; KM(1, i) = n1[i]; KM(2, i) = n2[i]; KM(6, i) = n6[i]; }
  for (int l = 3; l <= 5; l++) { KM(l, 19) = TL(0); KM(l, 21) = LYN; }
  TDW(0, 0) = 0x2C; TDW(0, 1) = 0xE1; TDW(0, 2) = 0x28;
}

// ---------------- luu tru (LittleFS noi bo cua nRF52) ----------------
static uint8_t macimg[4 + NMAC * MSLOT] __attribute__((aligned(4)));
static bool mac_dirty;
static uint8_t *mslot(int i) { return macimg + 4 + i * MSLOT; }
static void mac_put(int i, const char *name, uint8_t rp, uint16_t gap, const uint8_t *st, uint16_t n) {
  uint8_t *m = mslot(i); memset(m, 0, MSLOT); uint8_t nl = strlen(name); if (nl > 12) nl = 12;
  m[0] = rp; m[1] = gap & 255; m[2] = gap >> 8; m[3] = nl; memcpy(m + 4, name, nl); m[16] = n & 255; m[17] = n >> 8; if (n) memcpy(m + 18, st, n);
}
static void mac_defaults(void) {
  static const uint8_t cp[] = {2, 0xE0, 0, 1, 6, 0, 3, 0xE0, 0};                                        // giu Ctrl, nhan C, tha Ctrl
  static const uint8_t hi[] = {5, 9, 'X', 'i', 'n', ' ', 'c', 'h', 'a', 'o', '!', 1, 40, 0};            // go "Xin chao!" roi Enter
  uint32_t mg = MAC_MAGIC; memset(macimg, 0, sizeof macimg); memcpy(macimg, &mg, 4);
  for (int i = 0; i < NMAC; i++) mac_put(i, "", 1, 0, NULL, 0);
  mac_put(0, "Sao chep", 1, 0, cp, sizeof cp); mac_put(1, "Xin chao", 1, 0, hi, sizeof hi);
}
static bool file_read(const char *path, void *buf, uint32_t n) {
  File f(InternalFS);
  if (!f.open(path, FILE_O_READ)) return false;
  bool ok = f.size() == n && f.read((uint8_t *)buf, n) == (int)n;
  f.close(); return ok;
}
static bool file_write(const char *path, const void *buf, uint32_t n) {
  InternalFS.remove(path);
  File f(InternalFS);
  if (!f.open(path, FILE_O_WRITE)) return false;
  bool ok = f.write((const uint8_t *)buf, n) == n;
  f.close(); return ok;
}
static uint8_t outsel;      // 0 = uu tien USB, 1 = uu tien BLE
static uint8_t bt_slot;     // profile BLE 0..4
static void state_save(void) { uint32_t st[2] = {ST_MAGIC, (uint32_t)outsel | (uint32_t)bt_slot << 8}; file_write(ST_FILE, st, sizeof st); }
static void load_all(void) {
  if (!file_read(CFG_FILE, &cfg, sizeof cfg) || cfg.magic != MAGIC) defaults();
  uint32_t mg = 0;
  if (!file_read(MAC_FILE, macimg, sizeof macimg)) mac_defaults(); else { memcpy(&mg, macimg, 4); if (mg != MAC_MAGIC) mac_defaults(); }
  uint32_t st[2] = {0, 0};
  if (file_read(ST_FILE, st, sizeof st) && st[0] == ST_MAGIC) { outsel = st[1] & 1; bt_slot = (st[1] >> 8) % NBT; }
  if (LD(LD_MODE) < 1 || LD(LD_MODE) > NFX || LD(LD_TICK) < 20 || LD(LD_TICK) > 2000) { LD(LD_MODE) = 2; LD(LD_TICK) = 100; }
  if (cfg.w[NLI] < 1 || cfg.w[NLI] > L) cfg.w[NLI] = 7;
}

// ---------------- trang thai phim / layer ----------------
static uint32_t layers, tl;
static int nl(void) { uint16_t v = cfg.w[NLI]; return v < 1 ? 1 : v > L ? L : v; }
static uint16_t pressed[NK], tap, q[16], lat; static uint8_t qh, qt, tap_phase, tg;
static bool dirty, latch_wait;
static uint32_t act_ms;
static void qpush(uint16_t k) { if ((uint8_t)(qh - qt) < 16) q[qh++ & 15] = k; }

static uint16_t res_key(int p) { for (int l = nl() - 1; l >= 0; l--) if (l == 0 || (layers >> l & 1)) { uint16_t k = KM(l, p); if (k != KC_TRNS) return k; } return 0; }
static void relayer(void) {
  int n = nl(); uint32_t mask = n >= 32 ? 0xFFFFFFFFu : (1u << n) - 1; tl &= mask; layers = tl;
  for (int i = 0; i < NK; i++) if ((pressed[i] & 0xFF00) == 0x4000) layers |= 1u << (pressed[i] & 31);
  layers &= mask;
}
static void cycle_layer(int dir) {   // chuyen toggle sang layer ke tiep/truoc theo vong (layer 0..CYC_N-1)
  int n = nl(); if (n > CYC_N) n = CYC_N;
  int cur = 0; for (int l = L - 1; l > 0; l--) if (tl >> l & 1) { cur = l; break; }
  int nx = (cur + dir + n) % n; tl = nx ? 1u << nx : 0;
  relayer(); dirty = true;
}
static bool layer_action(uint16_t kc) {   // TL(n), LYN: tra ve true neu da xu ly
  if ((kc & 0xFF00) == 0x4100) { int n = kc & 31; if (n) tl ^= 1u << n; else tl = 0; relayer(); dirty = true; return true; }
  if ((kc & 0xFF00) == LYN) { cycle_layer((kc & 1) ? -1 : 1); return true; }
  return false;
}

// ---------------- LED (6 day, active-low) ----------------
static bool vb_cache;   // cap nhat 100 ms/lan trong loop()
static bool vbus(void) {
  uint8_t en = 0; sd_softdevice_is_enabled(&en);
  if (en) { uint32_t s = 0; sd_power_usbregstatus_get(&s); return s & POWER_USBREGSTATUS_VBUSDETECT_Msk; }
  return NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk;
}
static void led_apply(uint8_t mask) { for (int i = 0; i < NLED; i++) nrf_gpio_pin_write(LEDP[i], (mask >> i & 1) ? 0 : 1); }
static uint32_t rnd = 0x1234ABCDu;
static uint32_t xorshift(void) { rnd ^= rnd << 13; rnd ^= rnd >> 17; rnd ^= rnd << 5; return rnd; }
static uint8_t led_frame(uint8_t mode, uint32_t t) {
  static const uint8_t burst[8] = {0x0C, 0x1E, 0x3F, 0x33, 0x21, 0x00, 0x00, 0x00};
  const uint8_t M = (1u << NLED) - 1; uint32_t p;
  switch (mode) {
    case 1: return 0;                                         // tat het
    case 2: return M;                                         // sang het
    case 3: return 1u << (t % NLED);                          // chay duoi mot chieu
    case 4: p = t % (2 * NLED - 2); return 1u << (p < NLED ? p : (2 * NLED - 2) - p);   // di qua di lai
    case 5: return (t % 10) < 5 ? M : 0;                      // nhap nhay 1 Hz
    case 6: return (t % 10) < 5 ? 0x15 : 0x2A;                // xen ke
    case 7: p = t % (2 * NLED); return p < NLED ? ((1u << (p + 1)) - 1u) : ((M << (p - NLED + 1)) & M);   // day dan roi tat dan
    case 8: return xorshift() & M;                            // lap lanh ngau nhien
    case 9: return burst[t % 8];                              // no tu giua ra
  }
  return M;
}
static void led_next(int d) { int m = LD(LD_MODE) + d; if (m > NFX) m = 1; if (m < 1) m = NFX; LD(LD_MODE) = m; }
static void led_task(uint32_t now) {
  static uint32_t nt, tick; static int blink = -1, last_ind; static uint8_t bstep;
  bool pw = vb_cache || LD(LD_BAT);
  int ind = 0; for (int l = CYC_N - 1; l >= 0; l--) if (l == 0 || (layers >> l & 1)) { ind = l; break; }
  if (ind != last_ind) { last_ind = ind; if (LD(LD_IND) && pw && now > 3000 && ind < NLED) { blink = ind; bstep = 0; nt = now; } }
  if ((int32_t)(now - nt) < 0) return;
  if (!pw) { led_apply(0); nt = now + 500; return; }
  if (blink >= 0) { led_apply(bstep % 2 == 0 ? 1u << blink : 0); if (++bstep >= 6) { blink = -1; tick = 0; } nt = now + 150; return; }
  led_apply(led_frame(LD(LD_MODE), tick++)); nt = now + LD(LD_TICK);
}

// ---------------- Nokia multitap ----------------
static const char *NKT[11] = {"1abc", "2def", "3ghi", "4jkl", "5mno", "6pqr", "7stu", "8vwx", "9yz", "0 ", ".,?!"};
static uint16_t ckc(char c) {
  if (c >= 'a') return 4 + c - 'a'; if (c == '0') return 39; if (c >= '1' && c <= '9') return 30 + c - '1';
  switch (c) { case ' ': return 0x2C; case '.': return 0x37; case ',': return 0x36; case '?': return 0x2038; default: return 0x201E; }
}
static void nokia(int i) {
  static int ki = -1; static uint32_t kt; static int kn;
  if (i > 10) return; uint32_t n = ms();
  if (i == ki && n - kt < NK_TERM) { kn++; qpush(0x2A); } else kn = 0;
  ki = i; kt = n; char ch = NKT[i][kn % strlen(NKT[i])]; qpush(ckc(ch));
}

// ---------------- tap dance (1 cai tai 1 thoi diem) ----------------
static uint8_t tds, tdp, tdn, tdc; static uint32_t tdt;
static void td_press(int p, int n) {
  if (tds == 0) { tds = 1; tdp = p; tdn = n; tdc = 1; tdt = ms(); }
  else if (tds == 3 && tdn == n) { tds = 1; tdc = 2; tdp = p; tdt = ms(); }
}
static void td_release(int p) {
  if (p != tdp) return;
  if (tds == 1) { if (tdc == 2) { qpush(TDW(tdn, 2)); tds = 0; } else { tds = 3; tdt = ms(); } }
  else if (tds == 2) tds = 0;
}
static void td_task(uint32_t now) {
  if (tds == 1 && now - tdt >= TAP_TERM) { pressed[tdp] = TDW(tdn, 1); tds = 2; relayer(); dirty = true; }
  else if (tds == 3 && now - tdt >= TAP_TERM) { qpush(TDW(tdn, 0)); tds = 0; }
}

// ---------------- macro: buoc 1 nhan, 2 giu, 3 tha (ma 16 bit), 4 cho (ms 16 bit), 5 go chu (n, ky tu ASCII) ----------------
static uint16_t mc_held[MH]; static int8_t mc_id = -1; static uint16_t mc_pos, mc_ti, mc_cur; static uint8_t mc_rep, mc_ph; static uint32_t mc_until;
static bool mc_ok(uint16_t c) { return c < 0x100 || (c & 0xF000) == 0x2000 || (c & 0x8000); }
static void mc_down(uint16_t c) {
  if (!c || !mc_ok(c)) return;
  for (int i = 0; i < MH; i++) if (mc_held[i] == c) return;
  for (int i = 0; i < MH; i++) if (!mc_held[i]) { mc_held[i] = c; dirty = true; return; }
}
static void mc_up(uint16_t c) { for (int i = 0; i < MH; i++) if (mc_held[i] == c) { mc_held[i] = 0; dirty = true; } }
static void mc_stop(void) { memset(mc_held, 0, sizeof mc_held); mc_id = -1; mc_ph = 0; mc_cur = 0; dirty = true; }
static uint16_t mc_ascii(uint8_t ch) {
  static const char U[] = " \n\t-=[]\\;'`,./"; static const uint8_t UK[] = {0x2C, 0x28, 0x2B, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
  static const char S[] = "!@#$%^&*()_+{}|:\"~<>?"; static const uint8_t SK[] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
  if (ch >= 'a' && ch <= 'z') return 4 + ch - 'a';
  if (ch >= 'A' && ch <= 'Z') return 0x2000 | (4 + ch - 'A');
  if (ch >= '1' && ch <= '9') return 30 + ch - '1';
  if (ch == '0') return 39;
  if (!ch) return 0;
  const char *p = strchr(U, ch); if (p) return UK[p - U];
  p = strchr(S, ch); if (p) return 0x2000 | SK[p - S];
  return 0;
}
static void mc_start(int id) {
  if (mc_id >= 0) { mc_stop(); return; }   // bam them 1 phim macro khi dang chay = dung
  const uint8_t *m = mslot(id); uint16_t n = m[16] | m[17] << 8;
  if (!n || n > MSLOT - 18) return;
  mc_id = id; mc_pos = 0; mc_ti = 0; mc_ph = 0; mc_rep = m[0] < 1 ? 1 : m[0] > 99 ? 99 : m[0]; mc_until = ms();
}
static void mc_task(uint32_t now) {
  if (mc_id < 0 || (int32_t)(now - mc_until) < 0) return;
  const uint8_t *m = mslot(mc_id), *st = m + 18; uint16_t len = m[16] | m[17] << 8;
  if (len > MSLOT - 18) len = 0;
  if (mc_ph == 1) { mc_up(mc_cur); mc_cur = 0; mc_ph = 0; mc_until = now + 20; return; }
  if (mc_pos >= len) {
    if (mc_rep > 1) { mc_rep--; mc_pos = 0; mc_ti = 0; mc_until = now + (m[1] | m[2] << 8); }
    else mc_stop();
    return;
  }
  uint16_t c = mc_pos + 3 <= len ? st[mc_pos + 1] | st[mc_pos + 2] << 8 : 0;
  switch (st[mc_pos]) {
    case 1: mc_pos += 3; mc_down(c); mc_cur = c; mc_ph = 1; mc_until = now + 30; break;
    case 2: mc_pos += 3; mc_down(c); mc_until = now + 10; break;
    case 3: mc_pos += 3; mc_up(c); mc_until = now + 10; break;
    case 4: mc_pos += 3; mc_until = now + c; break;
    case 5: {
      uint8_t n = mc_pos + 1 < len ? st[mc_pos + 1] : 0;
      if (mc_pos + 2 + n > len) { mc_pos = len; break; }
      if (mc_ti >= n) { mc_pos += 2 + n; mc_ti = 0; break; }
      uint16_t k = mc_ascii(st[mc_pos + 2 + mc_ti++]);
      if (k) { mc_down(k); mc_cur = k; mc_ph = 1; mc_until = now + 20; }
      break;
    }
    default: mc_pos = len; break;
  }
}

// ---------------- USB + BLE ----------------
static const uint8_t d_kb[] = { TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)), TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(2)) };
static const uint8_t d_raw[] = {
  HID_USAGE_PAGE_N(0xFF60, 2), HID_USAGE(0x61), HID_COLLECTION(HID_COLLECTION_APPLICATION),
  HID_USAGE(0x62), HID_LOGICAL_MIN(0), HID_LOGICAL_MAX_N(0xFF, 2), HID_REPORT_SIZE(8), HID_REPORT_COUNT(32),
  HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
  HID_USAGE(0x63), HID_LOGICAL_MIN(0), HID_LOGICAL_MAX_N(0xFF, 2), HID_REPORT_SIZE(8), HID_REPORT_COUNT(32),
  HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
  HID_COLLECTION_END };
static Adafruit_USBD_HID usb_kb(d_kb, sizeof d_kb, HID_ITF_PROTOCOL_NONE, 1, false);
static Adafruit_USBD_HID usb_raw(d_raw, sizeof d_raw, HID_ITF_PROTOCOL_NONE, 1, true);
static BLEDis bledis;
static BLEHidAdafruit blehid;

static volatile bool ble_conn; static volatile uint16_t ble_hdl;
static bool adv_reconf = true;
static void on_connect(uint16_t h) { ble_hdl = h; ble_conn = true; act_ms = ms(); }
static void on_disconnect(uint16_t h, uint8_t reason) { (void)h; (void)reason; ble_conn = false; }

static void ble_adv_setup(void) {   // dat dia chi + ten theo profile, roi quang ba lai
  Bluefruit.Advertising.stop();
  ble_gap_addr_t a = Bluefruit.getAddr();
  uint32_t lo = NRF_FICR->DEVICEADDR[0], hi = NRF_FICR->DEVICEADDR[1];
  a.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
  a.addr[0] = lo; a.addr[1] = lo >> 8; a.addr[2] = lo >> 16; a.addr[3] = lo >> 24; a.addr[4] = hi; a.addr[5] = (hi >> 8) | 0xC0;
  a.addr[1] += bt_slot * 0x11;   // moi profile 1 dia chi rieng (profile 0 giu dia chi goc)
  Bluefruit.setAddr(&a);
  char nm[16]; snprintf(nm, sizeof nm, "Numpad21 %u", bt_slot + 1); Bluefruit.setName(nm);
  Bluefruit.Advertising.clearData(); Bluefruit.ScanResponse.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.ScanResponse.addName();
  adv_reconf = false;
}
static void ble_select(uint8_t s) {
  bt_slot = s % NBT; outsel = 1; state_save();
  if (ble_conn) Bluefruit.disconnect(ble_hdl);
  adv_reconf = true; dirty = true;
}
static void out_set(uint8_t o) { outsel = o & 1; state_save(); dirty = true; }
static bool usb_ok(void) { return TinyUSBDevice.mounted() && !TinyUSBDevice.suspended(); }
static int transport(void) {   // 0 = khong co, 1 = USB, 2 = BLE ; uu tien theo outsel, neu khong san sang thi dung cai con lai
  bool u = usb_ok(), b = ble_conn;
  if (outsel == 0) return u ? 1 : b ? 2 : 0;
  return b ? 2 : u ? 1 : 0;
}
static bool tx_ready(int t) { return t == 1 ? usb_kb.ready() : true; }
static bool tx_kb(int t, uint8_t mod, uint8_t *k) { return t == 1 ? usb_kb.keyboardReport(1, mod, k) : blehid.keyboardReport(mod, k); }
static bool tx_cons(int t, uint16_t c) { return t == 1 ? usb_kb.sendReport16(2, c) : blehid.consumerReport(c); }

static void hid_task(void) {
  static const uint8_t tgk[4] = {0x2C, 0xE1, 0xE0, 0xE2};
  static uint8_t lm, lk[6]; static uint16_t lc; static int ct;
  int t = transport();
  if (t != ct) {   // doi duong ra: nha het phim o duong cu, gui lai trang thai o duong moi
    if (ct && ct != t) { uint8_t z[6] = {0}; if (tx_ready(ct)) { tx_kb(ct, 0, z); tx_cons(ct, 0); } }
    ct = t; lm = 0; memset(lk, 0, 6); lc = 0; dirty = true;
  }
  if (!t) { qh = qt; tap = 0; tap_phase = 0; return; }   // khong co ket noi: bo phim cho
  if (!tx_ready(t)) return;
  if (tap_phase == 2) { tap = 0; tap_phase = 0; dirty = true; }
  else if (tap_phase == 0 && qh != qt) { tap = q[qt++ & 15]; tap_phase = 1; dirty = true; }
  if (!dirty) return;
  uint8_t mod = 0, k[6] = {0}; int n = 0; uint16_t cons = 0;
  for (int i = 0; i < NK + 6 + MH; i++) {
    uint16_t c = i < NK ? pressed[i] : i == NK ? tap : i == NK + 1 ? lat : i < NK + 6 ? ((tg >> (i - NK - 2) & 1) ? tgk[i - NK - 2] : 0) : mc_held[i - NK - 6];
    if (c & 0x8000) { cons = c & 0x7FFF; continue; }
    if ((c & 0xF000) == 0x2000) { mod |= 2; c &= 0xFF; } else if (c >= 0x100) continue;
    if (c >= 0xE0 && c <= 0xE7) mod |= 1 << (c - 0xE0); else if (c >= 4 && n < 6) k[n++] = c;
  }
  if (mod != lm || memcmp(k, lk, 6)) { if (tx_kb(t, mod, k)) { lm = mod; memcpy(lk, k, 6); } return; }
  if (cons != lc) { if (tx_cons(t, cons)) lc = cons; return; }
  dirty = false; if (tap_phase == 1) tap_phase = 2;
}

// ---------------- xu ly nhan phim ----------------
static void out_action(uint16_t kc) {
  if (kc == KC_OUT_TOG) out_set(outsel ^ 1); else if (kc == KC_OUT_USB) out_set(0); else if (kc == KC_OUT_BLE) out_set(1);
  else if (kc == KC_BT_CLR) { if (ble_conn) Bluefruit.disconnect(ble_hdl); Bluefruit.Periph.clearBonds(); adv_reconf = true; }
}
static void fire(int p, uint16_t kc) {
  if (!kc || kc == KC_TRNS) return;
  if (kc == KC_LATCH) { if (lat) lat = 0; else latch_wait = !latch_wait; dirty = true; return; }
  if (kc > KC_LATCH && kc <= KC_LATCH + 4) { tg ^= 1 << (kc - KC_LATCH - 1); dirty = true; return; }
  if (kc == KC_BOOT) { enterUf2Dfu(); return; }
  if (kc == KC_LED_NEXT || kc == KC_LED_PREV) { led_next(kc == KC_LED_NEXT ? 1 : -1); return; }
  if (kc >= KC_OUT_TOG && kc <= KC_BT_CLR) { out_action(kc); return; }
  if (kc >= KC_BT_SEL && kc < KC_BT_SEL + NBT) { ble_select(kc - KC_BT_SEL); return; }
  if ((kc & 0xFF00) == KC_MACRO && (kc & 255) < NMAC) { if (!latch_wait) mc_start(kc & 255); return; }
  if ((kc & 0xF000) == 0x5000) { td_press(p, kc & 3); return; }
  if ((kc & 0xF000) == 0x6000) { nokia(kc & 15); return; }
  if (layer_action(kc)) return;
  if ((kc & 0xFF00) == 0x4000) { pressed[p] = kc; return; }
  if (kc >= 0x7000 && kc < 0x8000) return;   // ma dac biet khong biet: bo qua (consumer 0x8000+ van di tiep)
  if (latch_wait) { lat = kc; latch_wait = false; dirty = true; return; }
  pressed[p] = kc;
}
static int pend = -1; static uint16_t pkc; static uint32_t pt;
static bool in_combo(uint16_t kc) {
  if (!kc || kc == KC_TRNS || (kc >= 0x4000 && !(kc & 0x8000))) return false;
  for (int n = 0; n < 4; n++) if (CB(n, 0) && (CB(n, 0) == kc || CB(n, 1) == kc)) return true;
  return false;
}
static void apply(uint32_t raw) {
  static uint32_t st;
  for (int p = 0; p < NK; p++) {
    uint32_t b = 1u << p; if (!((raw ^ st) & b)) continue;
    act_ms = ms();
    if (raw & b) {
      uint16_t kc = res_key(p);
      if (pend >= 0) {
        int hit = -1;
        for (int n = 0; n < 4; n++) if (CB(n, 0) && ((CB(n, 0) == pkc && CB(n, 1) == kc) || (CB(n, 1) == pkc && CB(n, 0) == kc))) hit = n;
        if (hit >= 0) { qpush(CB(hit, 2)); pend = -1; continue; }
        fire(pend, pkc); pend = -1;
      }
      if (in_combo(kc)) { pend = p; pkc = kc; pt = ms(); } else fire(p, kc);
    } else {
      pressed[p] = 0;
      if (pend == p) { qpush(pkc); pend = -1; }
      td_release(p);
    }
    relayer();
  }
  st = raw; dirty = true;
}

// ---------------- quet ma tran (co diode) ----------------
#if DIODE_ROW2COL
static const uint8_t *DRVP = ROWP, *SNSP = COLP; static const int NDRV = NROW, NSNS = NCOL;
#define DRV_OF(p) KROW[p]
#define SNS_OF(p) KCOL[p]
#else
static const uint8_t *DRVP = COLP, *SNSP = ROWP; static const int NDRV = NCOL, NSNS = NROW;
#define DRV_OF(p) KCOL[p]
#define SNS_OF(p) KROW[p]
#endif
static void pins_init(void) {
  for (int i = 0; i < NDRV; i++) { nrf_gpio_cfg_output(DRVP[i]); nrf_gpio_pin_clear(DRVP[i]); }
  for (int i = 0; i < NSNS; i++) nrf_gpio_cfg_input(SNSP[i], NRF_GPIO_PIN_PULLDOWN);
  for (int i = 0; i < NLED; i++) { nrf_gpio_cfg_output(LEDP[i]); nrf_gpio_pin_set(LEDP[i]); }   // LED tat (active-low)
}
static uint32_t scan(void) {
  uint8_t rd[8] = {0};
  for (int d = 0; d < NDRV; d++) {
    nrf_gpio_pin_set(DRVP[d]); delayMicroseconds(6);
    uint8_t v = 0; for (int s = 0; s < NSNS; s++) if (nrf_gpio_pin_read(SNSP[s])) v |= 1 << s;
    nrf_gpio_pin_clear(DRVP[d]); rd[d] = v;
  }
  uint32_t m = 0;
  for (int p = 0; p < NK; p++) if (rd[DRV_OF(p)] >> SNS_OF(p) & 1) m |= 1u << p;
  return m;
}

// ---------------- Raw HID (USB): [cmd, off, n, data...] ----------------
// 1 doc cfg (word), 2 ghi cfg, 3 luu flash, 4 mac dinh, 5 info, 6 doc macro (byte), 7 ghi macro (byte, n<=28, off cao o byte 31),
// 11 dieu khien: byte1 = 0 USB / 1 BLE / 2 doi / 3 chon profile (byte2) / 4 xoa ghep doi / 5 vao bootloader
static uint8_t rep[32]; static bool rep_pending, save_req;
static volatile uint8_t rx_n; static uint8_t rx_buf[32];
static void raw_set_cb(uint8_t id, hid_report_type_t t, uint8_t const *b, uint16_t sz) {   // chay trong task USB: chi chep ra, xu ly o loop()
  (void)id; (void)t; if (rx_n || sz < 1) return; if (sz > 32) sz = 32; memcpy(rx_buf, b, sz); rx_n = sz;
}
static void raw_process(const uint8_t *b, uint16_t sz) {
  memset(rep, 0, 32); rep[0] = b[0];
  uint16_t off = sz >= 3 ? b[1] | (sz >= 32 ? b[31] << 8 : 0) : 0;
  if ((b[0] == 1 || b[0] == 2) && sz >= 3 && b[2] <= 14 && off + b[2] <= NW) {
    rep[1] = b[1]; rep[2] = b[2]; rep[31] = off >> 8;
    for (int i = 0; i < b[2]; i++) {
      if (b[0] == 2 && sz >= 3 + 2 * (i + 1)) cfg.w[off + i] = b[3 + 2 * i] | b[4 + 2 * i] << 8;
      rep[3 + 2 * i] = cfg.w[off + i] & 255; rep[4 + 2 * i] = cfg.w[off + i] >> 8;
    }
    if (b[0] == 2) relayer();
  } else if ((b[0] == 6 || b[0] == 7) && sz >= 3 && b[2] <= 28 && off + b[2] <= NMAC * MSLOT) {
    rep[1] = b[1]; rep[2] = b[2]; rep[31] = off >> 8;
    if (b[0] == 7) { mc_stop(); mac_dirty = true; }
    for (int i = 0; i < b[2]; i++) {
      if (b[0] == 7 && sz >= 4 + i) macimg[4 + off + i] = b[3 + i];
      rep[3 + i] = macimg[4 + off + i];
    }
  } else if (b[0] == 3) save_req = true;
  else if (b[0] == 4) { defaults(); relayer(); mc_stop(); mac_defaults(); mac_dirty = true; }
  else if (b[0] == 11 && sz >= 2) {
    if (b[1] == 0) out_set(0); else if (b[1] == 1) out_set(1); else if (b[1] == 2) out_set(outsel ^ 1);
    else if (b[1] == 3 && sz >= 3) ble_select(b[2]);
    else if (b[1] == 4) out_action(KC_BT_CLR);
    else if (b[1] == 5) enterUf2Dfu();
  } else if (b[0] == 5) { /* info ben duoi */ }
  if (b[0] == 5 || b[0] == 11) {
    rep[1] = L; rep[2] = NW & 255; rep[3] = NW >> 8; rep[4] = FW_VER; rep[5] = NFX; rep[6] = NMAC; rep[7] = MSLOT & 255; rep[8] = MSLOT >> 8;
    rep[9] = NK; rep[10] = outsel; rep[11] = bt_slot; rep[12] = ble_conn; rep[13] = usb_ok(); rep[14] = transport(); rep[15] = vbus();
  }
  rep_pending = true;
}
static void save_all(void) {
  file_write(CFG_FILE, &cfg, sizeof cfg);
  if (mac_dirty) { mac_dirty = false; file_write(MAC_FILE, macimg, sizeof macimg); }
  state_save();
}

// ---------------- khoi dong / ngu sau ----------------
static void uicr_nfc_as_gpio(void) {   // P0.09 / P0.10 mac dinh la chan NFC: chuyen thanh GPIO thuong cho LED0 / LED1 (chi ghi UICR 1 lan, roi reset)
  if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos; while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk; while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos; while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    NVIC_SystemReset();
  }
}
static void sleep_now(void) {   // ngu sau: bam bat ky phim nao de thuc day (khoi dong lai)
  led_apply(0);
  Bluefruit.Advertising.stop(); if (ble_conn) { Bluefruit.disconnect(ble_hdl); delay(100); }
  for (int i = 0; i < NDRV; i++) { nrf_gpio_cfg_output(DRVP[i]); nrf_gpio_pin_set(DRVP[i]); }
  for (int i = 0; i < NSNS; i++) nrf_gpio_cfg_sense_input(SNSP[i], NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
  delay(5);
  sd_power_system_off();
  while (1) { __WFE(); }
}

void setup() {
  uicr_nfc_as_gpio();
  pins_init();
  InternalFS.begin();
  load_all();
  relayer();

  TinyUSBDevice.setID(0xC1A0, 0x0021);
  TinyUSBDevice.setManufacturerDescriptor("TM");
  TinyUSBDevice.setProductDescriptor("Numpad21 nRF52840");
  usb_kb.begin();
  usb_raw.setReportCallback(NULL, raw_set_cb); usb_raw.begin();
  if (TinyUSBDevice.mounted()) { TinyUSBDevice.detach(); delay(10); TinyUSBDevice.attach(); }   // enumerate lai voi descriptor moi

  Bluefruit.autoConnLed(false);
  Bluefruit.begin();
  Bluefruit.setTxPower(8);
  Bluefruit.Periph.setConnInterval(6, 12);   // 7.5 - 15 ms
  Bluefruit.Periph.setConnectCallback(on_connect);
  Bluefruit.Periph.setDisconnectCallback(on_disconnect);
  bledis.setManufacturer("TM"); bledis.setModel("Numpad21"); bledis.begin();
  blehid.begin();
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  ble_adv_setup();
  act_ms = ms();
}

void loop() {
  static uint32_t raw_prev, raw_t, applied, last_slow;
  uint32_t now = ms();
  uint32_t raw = scan();
  if (raw != raw_prev) { raw_prev = raw; raw_t = now; }
  else if (raw != applied && now - raw_t >= 4) { applied = raw; apply(raw); }   // chong doi 4 ms
  if (pend >= 0 && now - pt >= COMBO_TERM) { fire(pend, pkc); pend = -1; relayer(); dirty = true; }
  td_task(now); mc_task(now); led_task(now);
  hid_task();

  if (rx_n && !rep_pending) { raw_process(rx_buf, rx_n); rx_n = 0; }
  if (rep_pending && usb_raw.ready() && usb_raw.sendReport(0, rep, 32)) rep_pending = false;
  if (save_req && !rep_pending) { save_req = false; save_all(); }

  if (now - last_slow >= 100) {   // viec cham: quang ba BLE, ngu sau
    last_slow = now; vb_cache = vbus();
    if (!ble_conn) {
      if (adv_reconf) ble_adv_setup();
      bool want = outsel == 1 || !usb_ok();
      if (want && !Bluefruit.Advertising.isRunning()) Bluefruit.Advertising.start(0);
      else if (!want && Bluefruit.Advertising.isRunning()) Bluefruit.Advertising.stop();
    }
    if (!vb_cache && !raw && mc_id < 0 && now - act_ms > SLEEP_MS) sleep_now();
  }
  delay(raw || now - act_ms < 1500 || mc_id >= 0 || tds || pend >= 0 ? 1 : 5);   // luc nghi quet cham de tiet kiem pin
}

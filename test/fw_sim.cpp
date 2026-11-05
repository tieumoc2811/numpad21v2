// Mo phong END-TO-END tren may tinh: nhung CHINH firmware/numpad21/numpad21.ino vao day (Arduino / Bluefruit / TinyUSB thay bang stub/),
// bam phim vat ly gia (apply), chay hid_task() that va giai ma bao cao HID nhu may tinh phim My. Chay: sh test/run_tests.sh
#include "Arduino.h"
#include "bluefruit.h"
#include "Adafruit_TinyUSB.h"
#include "InternalFileSystem.h"
#include <string>
static uint32_t fake_ms;
uint32_t millis(void) { return fake_ms; } void delay(uint32_t) {} void delayMicroseconds(uint32_t) {}
nrf_power_t nrf_power_o; nrf_ficr_t nrf_ficr_o; nrf_uicr_t nrf_uicr_o; nrf_nvmc_t nrf_nvmc_o; BluefruitClass Bluefruit; Adafruit_LittleFS_Namespace::FS_ InternalFS; TinyUSBDeviceClass TinyUSBDevice;
void enterUf2Dfu(void) {}
// ---- "may tinh": giai ma tung bao cao ban phim ----
static char host[512]; static int hn; static uint8_t prevk[6]; static bool caps; static bool usb_up = true, ble_up = false; static int sent_ble;
static void hput(char c) { if (hn < (int)sizeof host - 1) host[hn++] = c; host[hn] = 0; }
static char us(uint8_t k, bool sh) {
  if (k >= 4 && k <= 29) return (sh ? 'A' : 'a') + k - 4;
  if (k >= 30 && k <= 39) return sh ? "!@#$%^&*()"[k - 30] : "1234567890"[k - 30];
  static const struct { uint8_t k; char n, s; } T[] = { {0x28,'\n','\n'}, {0x2C,' ',' '}, {0x2D,'-','_'}, {0x2E,'=','+'}, {0x2F,'[','{'}, {0x30,']','}'}, {0x31,'\\','|'}, {0x33,';',':'}, {0x34,'\'','"'}, {0x35,'`','~'}, {0x36,',','<'}, {0x37,'.','>'}, {0x38,'/','?'} };
  for (unsigned i = 0; i < sizeof T / sizeof *T; i++) if (T[i].k == k) return sh ? T[i].s : T[i].n;
  return 0;
}
static bool host_report(uint8_t mod, const uint8_t k[6]) {
  for (int a = 0; a < 6; a++) {
    uint8_t c = k[a]; if (!c) continue;
    bool was = false; for (int j = 0; j < 6; j++) if (prevk[j] == c) was = true;
    if (was) continue;
    if (c == 0x2A) { if (hn) host[--hn] = 0; }
    else if (c == 0x39) caps = !caps;
    else { char ch = us(c, (mod & 0x22) != 0); if (ch) { if (caps && ch >= 'a' && ch <= 'z') ch -= 32; hput(ch); } }
  }
  memcpy(prevk, k, 6); return true;
}
bool Adafruit_USBD_HID::ready() { return true; }
bool Adafruit_USBD_HID::keyboardReport(uint8_t, uint8_t mod, uint8_t *k) { return host_report(mod, k); }
bool Adafruit_USBD_HID::sendReport16(uint8_t, uint16_t) { return true; }
bool BLEHidAdafruit::keyboardReport(uint8_t mod, uint8_t k[6]) { sent_ble++; return host_report(mod, k); }
bool BLEHidAdafruit::consumerReport(uint16_t) { return true; }
bool TinyUSBDeviceClass::mounted() { return usb_up; } bool TinyUSBDeviceClass::suspended() { return false; }

#define _Static_assert static_assert   // g++ moi khong nhan _Static_assert (toolchain Arduino thi nhan)
#include "../firmware/numpad21/numpad21.ino"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static void pump(int rounds) { for (int i = 0; i < rounds; i++) { fake_ms++; td_task(fake_ms); hid_task(); } }
static uint32_t down;
static void press2(int p, int settle) { fake_ms += 10; down |= 1u << p; apply(down); if (settle) pump(60); }
static void release2(int p, int settle) { fake_ms += 10; down &= ~(1u << p); apply(down); if (settle) pump(60); }
#define press(p) press2(p, 1)
#define release(p) release2(p, 1)
static void tapk(int p) { press(p); release(p); }
// vi tri phim (xem board.h): layer 3 mac dinh
enum { P_DOT = 15, P_DASH = 16, P_OK = 17, P_ENT = 18, P_SPC = 19, P_HINT = 20, P_BS = 14, P_CAPS = 5, P_SHIFT = 6, P_ESC = 0, P_FN = 3 };
static void morse(const char *code, int ok) { for (; *code; code++) tapk(*code == '-' ? P_DASH : P_DOT); if (ok) tapk(P_OK); }
static void expect(const char *want, const char *what) { CHECK(!strcmp(host, want), "%s: may tinh co '%s', mong '%s'", what, host, want); }
static void to_morse_layer(void) { for (int i = 0; i < 3; i++) { cycle_layer(1); pump(5); } }   // nut layer nho: L0 -> L1 -> L2 -> L3
static void tapH(int p) { press2(p, 0); pump(3); release2(p, 0); pump(400); }   // bam NGAN (nha khi goi y chua xoa xong) roi cho hang doi xa het
static void holdEnt(void) { press2(P_ENT, 0); pump(260); release2(P_ENT, 0); pump(400); }   // giu ENTER > TAP_TERM = Enter (Tap dance 4, o giu)
static void reset_all(void) { morse_hint_on = 0; hn = 0; host[0] = 0; memset(prevk, 0, 6); usb_up = true; ble_up = false; down = 0; tl = 0; memset(pressed, 0, sizeof pressed); defaults(); relayer(); morse_commit(&mrs); qh = qt = 0; tap = 0; tap_phase = 0; mgate = 0; memset(mgh, 0, sizeof mgh); caps = false; tds = 0; pend = -1; tg = 0; latch_wait = false; lat = 0; outsel = 0; pump(20); }

int main(void) {
  if (getenv("LEDFRAMES")) { for (int m = 1; m <= NFX; m++) for (uint32_t t = 0; t < 200; t++) printf("%d\n", led_frame(m, t)); return 0; }   // cross_check.js: so khung den C voi JS
  reset_all();
  CHECK(KM(3, P_DOT) == KC_MORSE_DOT && KM(3, P_DASH) == KC_MORSE_DASH && KM(3, P_OK) == KC_MORSE_OK && KM(3, P_HINT) == KC_MORSE_HINT, "defaults(): layer 3 chua dat phim Morse");
  CHECK(KM(3, 21) == LYN && KM(3, P_FN) == KC_TRNS && KM(0, P_FN) == MO(6), "layer 3 giu nut layer nho va FN");
  CHECK(KM(4, 19) == TL(0) && KM(5, 19) == TL(0), "layer 4, 5 van nhu cu");
  CHECK(MORSE_HINT_DEF == 1 && FW_VER == 14 && NFX == 16 && QN == 64, "hang so");
  to_morse_layer();
  CHECK(res_key(P_DOT) == KC_MORSE_DOT && res_key(P_OK) == KC_MORSE_OK, "chua vao layer Morse sau 3 lan");
  CHECK(res_key(1) == 0 && res_key(8) == 0, "phim khac bi chan");
  CHECK(res_key(21) == LYN, "nut layer nho van chuyen layer");

  tapk(P_DOT); expect("e", "1 cham");
  tapk(P_DOT); expect("i", "2 cham");
  tapk(P_DASH); expect("u", "..-");
  tapk(P_DASH); expect("", "..-- khong hop le: xoa het");
  tapk(P_DOT); tapk(P_DOT); expect("?", "..--.. = ?");
  tapk(P_OK); expect("?", "Xong chot, khong xoa");
  tapk(P_DOT); expect("?e", "ky tu moi sau Xong");        // sau Xong, cham phai ra e chu khong ra i
  tapk(P_BS); expect("?", "Backspace xoa ky tu dang hien");
  tapk(P_DOT); expect("?e", "sau Backspace la ky tu moi");

  reset_all(); to_morse_layer();
  morse("....", 1); morse(".", 1); morse(".-..", 1); morse(".-..", 1); morse("---", 1); tapk(P_SPC); morse(".--", 1); morse("---", 1); morse(".-.", 1); morse(".-..", 1); morse("-..", 1); holdEnt(); morse("...--", 1); morse(".--.-.", 0);
  expect("hello world\n3@", "cau hoan chinh");

  reset_all(); to_morse_layer(); tapk(P_DOT); tapk(P_SPC); tapk(P_DOT); expect("e e", "Space chot ky tu"); tapk(P_DASH); expect("e a", "khong xoa dau cach");

  reset_all(); to_morse_layer();   // bam nhanh khi hang doi chua kip xa
  for (int r = 0; r < 20; r++) {
    for (const char *c = "...-..-"; *c; c++) { int p = *c == '-' ? P_DASH : P_DOT; press2(p, 0); release2(p, 0); fake_ms++; hid_task(); }
    pump(60); tapk(P_OK);
  }
  { char w[32] = {0}; for (int i = 0; i < 20; i++) w[i] = '$'; expect(w, "bam nhanh 20 lan $"); }

  reset_all(); to_morse_layer(); tapk(P_CAPS); morse("...", 1); morse("---", 1); morse("...", 0); expect("SOS", "Caps Lock"); tapk(P_OK); tapk(P_CAPS); morse(".----", 0); expect("SOS1", "so khong doi");
  reset_all(); to_morse_layer(); tapk(P_SHIFT); morse(".-", 0); tapk(P_SHIFT); CHECK(host[0] == 'A', "giu Shift (TG) -> chu hoa, co '%s'", host);

  // layer 3 -> ve layer 0 bang TL(0) o phim ESC
  reset_all(); to_morse_layer(); tapk(P_ESC); CHECK(res_key(P_DOT) == 0x59, "TL(0) ve layer 0: phim 15 = '%x'", res_key(P_DOT));
  // FN (MO(6)) van dung duoc o layer Morse
  reset_all(); to_morse_layer(); press(P_FN); CHECK(layers >> 6 & 1, "giu FN o layer Morse bat layer 6"); release(P_FN);

  // khong co ket noi (USB rut, BLE chua noi): phim bi bo, Morse ve ky tu moi (khong de lech Backspace)
  reset_all(); to_morse_layer(); usb_up = false; tapk(P_DOT); tapk(P_DOT); pump(60); expect("", "chua co ket noi: khong gui gi"); CHECK(mrs.len == 0, "Morse ve trang thai moi khi mat ket noi"); usb_up = true; pump(20);
  tapk(P_DOT); expect("e", "cam lai: cham moi ra e (khong xoa nham)");
  // gui qua BLE
  reset_all(); to_morse_layer(); usb_up = false; ble_up = true; ble_conn = true; tapk(P_DOT); tapk(P_DASH); expect("a", "BLE: .-"); CHECK(sent_ble > 0, "bao cao qua BLE"); ble_conn = false;

  // Morse gan vao BAT KY phim o BAT KY layer (layer 0)
  reset_all(); KM(0, 1) = KC_MORSE_DOT; KM(0, 2) = KC_MORSE_DASH; KM(0, 3) = KC_MORSE_OK;
  tapk(1); expect("e", "layer 0 cham"); tapk(2); expect("a", ".-"); tapk(3); tapk(1); expect("ae", "Xong roi go tiep");
  // gan layer 2 (Nokia) + chuyen layer
  reset_all(); KM(2, 5) = KC_MORSE_DOT; KM(2, 6) = KC_MORSE_DASH; KM(2, 7) = KC_MORSE_OK; tl = 1u << 2; relayer();
  tapk(5); tapk(7); tapk(5); expect("ee", "gan layer 2: cham, Xong, cham"); tapk(7); tapk(6); expect("eet", "gach");
  // phim thuong chot ky tu
  reset_all(); KM(0, 1) = KC_MORSE_DOT; tapk(1); tapk(8); expect("e", "phim 7 (KP7 = 0x5F, may tinh gia khong in) khong xoa chu e"); CHECK(mrs.len == 0, "phim thuong chot ky tu Morse");
  KM(0, 9) = 0x04; hn = 0; host[0] = 0; tapk(1); tapk(9); expect("ea", "phim a chot; cham tiep"); tapk(1); expect("eae", "sau phim thuong, cham moi la ky tu moi");
  // tap dance: bam = cham, giu = gach
  reset_all(); KM(0, 1) = TD(1); TDW(1, 0) = KC_MORSE_DOT; TDW(1, 1) = KC_MORSE_DASH;
  press(1); release(1); fake_ms += 250; td_task(fake_ms); pump(60); expect("e", "tap dance: bam -> cham");
  press(1); fake_ms += 250; td_task(fake_ms); pump(60); release(1); expect("a", "tap dance: giu -> gach");
  // combo -> Morse
  reset_all(); KM(0, 1) = 0x04; KM(0, 2) = 0x05; CB(0, 0) = 0x04; CB(0, 1) = 0x05; CB(0, 2) = KC_MORSE_DASH;
  press(1); press(2); release(1); release(2); expect("t", "combo -> gach");
  // loi cu: Xong dat trong tap dance, cham NGAY sau do -> phai ra "ee", khong phai "i"
  reset_all(); KM(0, 1) = KC_MORSE_DOT; KM(0, 2) = TD(1); TDW(1, 0) = KC_MORSE_OK;
  tapk(1); expect("e", "TD-Xong: cham");
  press(2); release(2); fake_ms += 60; tapk(1); pump(60); fake_ms += 300; td_task(fake_ms); pump(60); expect("ee", "TD-Xong roi cham nhanh: Xong chay truoc");
  reset_all(); KM(0, 1) = KC_MORSE_DOT; KM(0, 2) = TD(1); TDW(1, 0) = KC_MORSE_OK;   // cuon phim
  tapk(1); press(2); fake_ms += 40; tapk(1); release(2); pump(60); fake_ms += 300; td_task(fake_ms); pump(60); expect("ee", "TD-Xong cuon phim");
  // tap dance thuong van chay dung
  reset_all(); KM(0, 2) = TD(1); TDW(1, 0) = 0x04; TDW(1, 2) = 0x05;
  press(2); release(2); fake_ms += 30; press(2); release(2); fake_ms += 300; td_task(fake_ms); pump(60); expect("b", "bam doi -> b");
  hn = 0; host[0] = 0; press(2); release(2); fake_ms += 300; td_task(fake_ms); pump(60); expect("a", "bam don -> a");
  // Nokia chot ky tu Morse
  reset_all(); KM(0, 1) = KC_MORSE_DOT; KM(0, 2) = NKK(0); tapk(1); tapk(2); expect("e1", "Nokia chot Morse"); tapk(1); expect("e1e", "sau Nokia, cham moi la ky tu moi");
  // macro chay: chot ky tu Morse
  reset_all(); KM(0, 1) = KC_MORSE_DOT; tapk(1); mc_start(0); CHECK(mrs.len == 0, "macro chot ky tu Morse"); mc_stop(); tapk(1); CHECK(mrs.len == 1, "sau macro cham moi la ky tu moi");
  // doi layer giua chung
  reset_all(); to_morse_layer(); tapk(P_DOT); cycle_layer(1); relayer(); tapk(P_DOT); CHECK(mrs.len <= 2, "doi layer khong lam hong trang thai");

  // ---------- GOI Y tren van ban (" [.i -a]") ----------
  reset_all(); morse_hint_on = 1; to_morse_layer();
  tapH(P_DOT); expect("e [.i -a]", "goi y: 1 cham");
  tapH(P_DOT); expect("i [.s -u]", "goi y: 2 cham");
  tapH(P_DASH); expect("u [.f]", "goi y: ..-");
  tapH(P_OK); expect("u", "Xong chi xoa goi y");
  tapH(P_SPC); expect("u ", "Space");
  tapH(P_DOT); expect("u e [.i -a]", "ky tu moi co goi y");
  tapH(P_SPC); expect("u e ", "Space chot: goi y bi xoa TRUOC, roi moi ra dau cach");
  tapH(P_DOT); tapH(P_DASH); expect("u e a [.r -w]", ".-");
  tapH(P_BS); expect("u e ", "Backspace chot: xoa goi y roi xoa 'a' (khong xoa nham goi y)");
  tapH(P_DOT); holdEnt(); expect("u e e\n", "Enter chot");
  tapH(P_DOT); press2(P_SPC, 0); pump(400); release2(P_SPC, 0); pump(60); expect("u e e\ne ", "giu Space trong luc con goi y");
  reset_all(); morse_hint_on = 1; to_morse_layer(); tapH(P_DOT); press2(P_SPC, 0); pump(1); release2(P_SPC, 0); pump(400); expect("e ", "Space bam ngan luc con goi y");
  tapH(P_DOT); press2(P_BS, 0); pump(1); release2(P_BS, 0); pump(400); expect("e ", "Backspace bam ngan: xoa 'e' moi, khong xoa nham goi y");
  // bat / tat bang phim Goi y
  reset_all(); morse_hint_on = 1; to_morse_layer();
  tapH(P_DOT); expect("e [.i -a]", "truoc khi tat"); tapH(P_HINT); expect("e", "phim Goi y: tat -> xoa goi y"); CHECK(!morse_hint_on, "co tat");
  tapH(P_DOT); expect("i", "tat: chi con ky tu, khong goi y"); tapH(P_HINT); expect("i [.s -u]", "phim Goi y: bat lai giua chung"); CHECK(morse_hint_on, "co bat");
  tapH(P_OK); expect("i", "Xong"); tapH(P_SPC); expect("i ", "Space");
  // tat goi y: hanh vi cu giu nguyen
  reset_all(); to_morse_layer(); tapk(P_DOT); tapk(P_DOT); tapk(P_SPC); expect("i ", "goi y tat: nhu cu");
  // bam rat nhanh
  reset_all(); morse_hint_on = 1; to_morse_layer();
  for (int r = 0; r < 12; r++) { for (const char *c = "...-..-"; *c; c++) { int p = *c == '-' ? P_DASH : P_DOT; press2(p, 0); release2(p, 0); pump(120); } tapH(P_OK); }
  { char w[32] = {0}; for (int i = 0; i < 12; i++) w[i] = '$'; expect(w, "goi y bat: bam nhanh 12 lan $"); }
  // mat ket noi luc dang co goi y: khong de lech (khong gui Backspace thua khi noi lai)
  reset_all(); morse_hint_on = 1; to_morse_layer(); tapH(P_DOT); usb_up = false; tapk(P_DOT); pump(60); usb_up = true; pump(20); CHECK(mrs.len == 0 && mrs.hint == 0 && !mgate, "mat ket noi: Morse + goi y + mgate ve 0");
  hn = 0; host[0] = 0; tapH(P_DOT); expect("e [.i -a]", "noi lai: ky tu moi");

  // ---------- DEN LED bao trang thai go Morse ----------
  {
    auto ledmask = []() { uint8_t m = 0; for (int i = 0; i < NLED; i++) if (!stub_pin_level[LEDP[i]]) m |= 1u << i; return m; };   // LED active-low
    auto ledrun = [&](uint32_t ms) { for (uint32_t i = 0; i <= ms; i++) { led_task(fake_ms); if (i < ms) fake_ms++; } return ledmask(); };   // chay led_task tung ms, tra mat na luc cuoi
    reset_all(); to_morse_layer(); vb_cache = true; LD(LD_MODE) = 1; LD(LD_TICK) = 100; LD(LD_IND) = 0; morse_led_on = MORSE_LED_DEF; mled_kick = 0; mled_tp = mled_ta = fake_ms - 100000u;
    CHECK(MORSE_LED_DEF == 1 && IS_MORSE(KC_MORSE_LED) && !IS_MORSE(KC_MORSE_LED + 1), "hang so den Morse");
    fake_ms += 3000; CHECK(ledrun(0) == 0, "hieu ung 'tat het' luc dau: den tat");
    morse_key(KC_MORSE_DOT); CHECK(ledrun(0) == 0x3F, "cham: nhay ngay (%x)", ledmask()); CHECK(ledrun(30) == 0x3F, "cham: con nhay luc +30 ms"); CHECK(ledrun(70) == 0x01, "cham: +100 ms thanh 1 den (%x)", ledmask());
    morse_key(KC_MORSE_DASH); CHECK(ledrun(0) == 0x3F, "gach: nhay ngay"); CHECK(ledrun(150) == 0x3F, "gach: con nhay luc +150 ms (dai hon cham)"); CHECK(ledrun(110) == 0x03, "gach: +260 ms thanh 2 den (%x)", ledmask());
    morse_key(KC_MORSE_OK); CHECK(mrs.len == 0, "Xong chot"); CHECK(ledrun(0) == 0x3F, "chot: nhay doi pha 1"); CHECK(ledrun(80) == 0, "chot: pha tat"); CHECK(ledrun(80) == 0x3F, "chot: pha sang 2"); CHECK(ledrun(140) == 0, "chot: het nhay, tra lai hieu ung (tat het)");
    morse_key(KC_MORSE_DOT); ledrun(500); CHECK(ledrun(0) == 0x01, "giu thanh den luc +500 ms"); CHECK(ledrun(2100) == 0, "qua 2.5 s khong bam them: tra lai hieu ung (ma van dang go: len=%d)", mrs.len); CHECK(mrs.len == 1, "ma van con khi den het bao");
    mcommit(); ledrun(400);
    morse_key(KC_MORSE_DOT); morse_key(KC_MORSE_DOT); ledrun(0); CHECK(ledrun(150) == 0x03, "2 cham: thanh 2 den"); mcommit(); ledrun(400);
    for (int i = 0; i < 7; i++) morse_key(KC_MORSE_DASH); ledrun(0); CHECK(ledrun(300) == 0x3F && mrs.len == 7, "7 net: thanh den day (6 den)"); mcommit(); ledrun(400);
    mcommit(); CHECK(ledrun(0) == 0, "chot khi chua go gi: khong nhay");
    // hieu ung binh thuong van chay khi khong go Morse
    LD(LD_MODE) = 2; ledrun(200); CHECK(ledrun(0) == 0x3F, "mode 'sang het' khi khong go Morse");
    LD(LD_MODE) = 1; ledrun(200);
    // tat / bat bang phim
    morse_key(KC_MORSE_LED); CHECK(!morse_led_on, "phim Den: tat"); ledrun(30);
    morse_key(KC_MORSE_DOT); CHECK(ledrun(0) == 0 && ledrun(100) == 0, "tat: go Morse khong lam den doi"); mcommit(); ledrun(400);
    morse_key(KC_MORSE_LED); CHECK(morse_led_on, "phim Den: bat lai");
    // phim vat ly -> keymap -> morse_key, va den chi sang khi co 5V USB (hoac bat 'sang ca khi chay pin')
    reset_all(); KM(0, 1) = KC_MORSE_LED; KM(0, 2) = KC_MORSE_DOT; morse_led_on = 1; tapk(1); CHECK(!morse_led_on, "phim vat ly gan Den: tat"); tapk(1); CHECK(morse_led_on, "phim vat ly gan Den: bat");
    reset_all(); to_morse_layer(); vb_cache = true; LD(LD_MODE) = 1; LD(LD_IND) = 0; morse_led_on = 1; mled_kick = 0; mled_tp = mled_ta = fake_ms - 100000u; ledrun(100);
    tapk(P_DOT); CHECK(mrs.len == 1 && ledrun(0) == 0x01, "phim vat ly cham: thanh 1 den (%x)", ledmask());
    mcommit(); ledrun(400); vb_cache = false; LD(LD_BAT) = 0; tapk(P_DOT); CHECK(ledrun(600) == 0, "chay pin (khong co 5V): den tat"); mcommit();
    vb_cache = true;
  }

    // ---------- v13: che do LED 1..16, phim LED, ten layer, Enter = tap dance 4 ----------
  {
    reset_all(); to_morse_layer();
    morse("...", 0); tapk(P_ENT); pump(400); expect("s", "Enter (TD4) bam 1 lan = Xong, khong gui gi them");
    reset_all(); to_morse_layer(); tapk(P_ENT); pump(10); tapk(P_ENT); pump(400); expect(" ", "Enter (TD4) bam 2 lan = Space");
    CHECK(led_frame(10, 0) == 3 && led_frame(10, 4) == 0x30 && led_frame(11, 37) == 37 && led_frame(13, 0) == 0x20 && led_frame(13, 5) == 1, "led 10/11/13");
    CHECK(led_frame(12, 0) == 0x3F && led_frame(12, 1) == 0 && led_frame(14, 0) == 0x21 && led_frame(14, 3) == 0x1E && led_frame(15, 0) == 7 && led_frame(15, 5) == 0x38, "led 12/14/15");
    for (int i = 0; i < 200; i++) { uint8_t f = led_frame(16, i); CHECK(f && !(f & (f - 1)) && f < 64, "led 16: dung 1 den"); }
    for (int m = 1; m <= NFX; m++) { led_set(m); CHECK(LD(LD_MODE) == m, "led_set %d", m); }
    reset_all(); KM(0, 1) = KC_LED_SEL + 15; KM(0, 2) = KC_LED_NEXT; KM(0, 4) = KC_LED_PREV; LD(LD_MODE) = 2;
    tapk(1); CHECK(LD(LD_MODE) == 16, "phim LED 16 chon thang che do 16"); tapk(2); CHECK(LD(LD_MODE) == 1, "LED + tu 16 quay ve 1"); tapk(4); CHECK(LD(LD_MODE) == 16, "LED - tu 1 ve 16");
    uint8_t r[32] = {0}; r[0] = 9; r[1] = 12; r[2] = 5; memcpy(r + 3, "Game\0", 5); raw_process(r, 32);
    CHECK(!strcmp(lname[1], "Game") && name_dirty, "ten layer 1 sau lenh 9");
    r[0] = 8; r[1] = 0; r[2] = 28; raw_process(r, 32); CHECK(!memcmp(rep + 3 + 12, "Game\0", 5), "lenh 8 doc lai ten layer");
    r[0] = 9; r[1] = 0; r[2] = 28; memset(r + 3, 'x', 28); raw_process(r, 32); CHECK(lname[0][NML - 1] == 0 && lname[1][NML - 1] == 0, "byte cuoi moi ten luon la 0");
    LD(LD_MODE) = 7; r[0] = 5; raw_process(r, 32); CHECK(rep[16] == 7, "info bao che do LED hien tai cho app");
    r[0] = 4; raw_process(r, 32); CHECK(lname[1][0] == 0, "ve mac dinh xoa ten layer");
    // FW_VER 14: ten profile BLE (lenh 12/13) + tat ngu sau (lenh 11/6, phim 0x7034)
    memset(r, 0, 32); r[0] = 13; r[1] = 12; r[2] = 7; memcpy(r + 3, "Laptop\0", 7); raw_process(r, 32);
    CHECK(!strcmp(bname[1], "Laptop") && bname_dirty && adv_reconf, "ten profile BLE 2 sau lenh 13");
    memset(r, 0, 32); r[0] = 12; r[1] = 0; r[2] = 28; raw_process(r, 32); CHECK(!memcmp(rep + 3 + 12, "Laptop\0", 7) && rep[3] == 0, "lenh 12 doc lai ten profile BLE");
    memset(r, 0, 32); r[0] = 13; r[1] = 0; r[2] = 28; memset(r + 3, 'y', 28); raw_process(r, 32); CHECK(bname[0][NML - 1] == 0 && bname[1][NML - 1] == 0, "byte cuoi moi ten BLE luon la 0");
    memset(r, 0, 32); r[0] = 13; r[1] = 50; r[2] = 28; raw_process(r, 32); CHECK(bname[4][NML - 1] == 0, "ghi ten BLE vuot vung bi tu choi");
    memset(r, 0, 32); r[0] = 11; r[1] = 6; r[2] = 1; raw_process(r, 32); CHECK(nosleep == 1 && rep[17] == 1, "lenh 11/6 tat ngu sau, info bao nosleep");
    r[2] = 0; raw_process(r, 32); CHECK(nosleep == 0 && rep[17] == 0, "lenh 11/6 bat lai ngu sau");
    reset_all(); KM(0, 1) = KC_SLEEP_TOG; nosleep = 0; tapk(1); CHECK(nosleep == 1, "phim SLEEP_TOG bat nosleep"); tapk(1); CHECK(nosleep == 0, "phim SLEEP_TOG tat lai");
    nosleep = 1; memset(r, 0, 32); r[0] = 4; raw_process(r, 32); CHECK(bname[1][0] == 0 && nosleep == 0, "ve mac dinh xoa ten BLE va bat lai ngu sau");
  }
printf("%d kiem tra end-to-end, %d loi\n", checks, fails);
  return fails ? 1 : 0;
}

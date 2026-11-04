// Mo phong END-TO-END tren may tinh: nhung CHINH numpad21.ino vao day, bam phim vat ly gia (apply), chay hid_task() that,
// giai ma bao cao HID nhu 1 may tinh phim My. Header trong stub/ chi la khung gia cho Arduino/Bluefruit/TinyUSB.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>
#include "Arduino.h"
#include "nrf_gpio.h"
#include "bluefruit.h"
#include "Adafruit_TinyUSB.h"
#include "Adafruit_LittleFS.h"
#include "InternalFileSystem.h"
static uint32_t fake_ms; static bool usb_up = true;
static std::string hostbuf;
// ---- cai dat khung gia ----
uint32_t millis() { return fake_ms; } void delay(uint32_t) {} void delayMicroseconds(uint32_t) {}
void nrf_gpio_cfg_output(uint32_t) {} void nrf_gpio_cfg_input(uint32_t, int) {} void nrf_gpio_cfg_sense_input(uint32_t, int, int) {}
void nrf_gpio_pin_set(uint32_t) {} void nrf_gpio_pin_clear(uint32_t) {} void nrf_gpio_pin_write(uint32_t, int) {} int nrf_gpio_pin_read(uint32_t) { return 0; }
PowerRegs pr; FicrRegs fr; UicrRegs ur; NvmcRegs nr; PowerRegs *NRF_POWER = &pr; FicrRegs *NRF_FICR = &fr; UicrRegs *NRF_UICR = &ur; NvmcRegs *NRF_NVMC = &nr;
void NVIC_SystemReset() {} void __WFE() {} int sd_softdevice_is_enabled(uint8_t *e) { *e = 0; return 0; } int sd_power_usbregstatus_get(uint32_t *) { return 0; } void sd_power_system_off() {} void enterUf2Dfu() {}
bool BLEHidAdafruit::begin() { return true; } bool BLEHidAdafruit::keyboardReport(uint8_t, uint8_t[6]) { return true; } bool BLEHidAdafruit::consumerReport(uint16_t) { return true; }
void BLEDis::setManufacturer(const char *) {} void BLEDis::setModel(const char *) {} bool BLEDis::begin() { return true; }
void Adv::stop() {} void Adv::clearData() {} void Adv::addFlags(uint8_t) {} void Adv::addTxPower() {} void Adv::addAppearance(uint16_t) {} void Adv::addService(BLEHidAdafruit &) {} void Adv::restartOnDisconnect(bool) {} void Adv::setInterval(int, int) {} void Adv::setFastTimeout(int) {} bool Adv::isRunning() { return false; } bool Adv::start(int) { return true; }
void Scan::clearData() {} void Scan::addName() {}
void PeriphT::clearBonds() {} void PeriphT::setConnInterval(int, int) {} void PeriphT::setConnectCallback(void (*)(uint16_t)) {} void PeriphT::setDisconnectCallback(void (*)(uint16_t, uint8_t)) {}
ble_gap_addr_t BluefruitClass::getAddr() { return ble_gap_addr_t(); } bool BluefruitClass::setAddr(ble_gap_addr_t *) { return true; } bool BluefruitClass::setName(const char *) { return true; } void BluefruitClass::autoConnLed(bool) {} bool BluefruitClass::begin() { return true; } void BluefruitClass::setTxPower(int) {} bool BluefruitClass::disconnect(uint16_t) { return true; }
BluefruitClass Bluefruit;
// ---- "may tinh": giai ma tung bao cao ban phim ----
static uint8_t prevk[6];
static bool caps;
static char us(uint8_t k, bool sh) {
  if (k >= 4 && k <= 29) return (sh ? 'A' : 'a') + k - 4;
  if (k >= 30 && k <= 39) return sh ? "!@#$%^&*()"[k - 30] : "1234567890"[k - 30];
  struct { uint8_t k; char n, s; } T[] = { {0x28,'\n','\n'}, {0x2C,' ',' '}, {0x2D,'-','_'}, {0x2E,'=','+'}, {0x2F,'[','{'}, {0x30,']','}'}, {0x31,'\\','|'}, {0x33,';',':'}, {0x34,'\'','"'}, {0x35,'`','~'}, {0x36,',','<'}, {0x37,'.','>'}, {0x38,'/','?'} };
  for (auto &t : T) if (t.k == k) return sh ? t.s : t.n;
  return 0;
}
bool Adafruit_USBD_HID::ready() { return true; }
bool Adafruit_USBD_HID::keyboardReport(uint8_t, uint8_t mod, uint8_t *k) {
  for (int i = 0; i < 6; i++) {
    uint8_t c = k[i]; if (!c) continue;
    bool was = false; for (int j = 0; j < 6; j++) if (prevk[j] == c) was = true;
    if (was) continue;   // phim dang giu tu bao cao truoc: khong sinh ky tu moi
    if (c == 0x2A) { if (!hostbuf.empty()) hostbuf.pop_back(); }
    else if (c == 0x39) caps = !caps;
    else { char ch = us(c, (mod & 0x22) != 0); if (ch) { if (caps && ch >= 'a' && ch <= 'z') ch -= 32; hostbuf += ch; } }
  }
  memcpy(prevk, k, 6); return true;
}
bool Adafruit_USBD_HID::sendReport16(uint8_t, uint16_t) { return true; } bool Adafruit_USBD_HID::sendReport(uint8_t, const void *, uint8_t) { return true; }
void Adafruit_USBD_HID::setReportCallback(void *, void (*)(uint8_t, hid_report_type_t, uint8_t const *, uint16_t)) {} bool Adafruit_USBD_HID::begin() { return true; }
Adafruit_USBD_HID::Adafruit_USBD_HID(const uint8_t *, int, int, int, bool) {}
bool TinyUSBDev::mounted() { return usb_up; } bool TinyUSBDev::suspended() { return false; } void TinyUSBDev::detach() {} void TinyUSBDev::attach() {} void TinyUSBDev::setID(int, int) {} void TinyUSBDev::setManufacturerDescriptor(const char *) {} void TinyUSBDev::setProductDescriptor(const char *) {}
TinyUSBDev TinyUSBDevice;
bool Adafruit_LittleFS_Namespace::FileSys::begin() { return true; } bool Adafruit_LittleFS_Namespace::FileSys::remove(const char *) { return true; }
Adafruit_LittleFS_Namespace::File::File(Adafruit_LittleFS_Namespace::FileSys &) {} bool Adafruit_LittleFS_Namespace::File::open(const char *, int) { return false; } uint32_t Adafruit_LittleFS_Namespace::File::size() { return 0; } int Adafruit_LittleFS_Namespace::File::read(uint8_t *, uint32_t) { return 0; } uint32_t Adafruit_LittleFS_Namespace::File::write(const uint8_t *, uint32_t) { return 0; } void Adafruit_LittleFS_Namespace::File::close() {}
Adafruit_LittleFS_Namespace::FileSys InternalFS;

#include "../firmware/numpad21/numpad21.ino"   // <- chinh firmware that (stub/ dat truoc trong -I)

// ---- dieu khien ----
static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static void pump(int rounds = 60) { for (int i = 0; i < rounds; i++) { fake_ms++; hid_task(); } }   // chay hid_task den khi het phim
static uint32_t down;
static void press(int p, bool settle = true) { fake_ms += 10; down |= 1u << p; apply(down); if (settle) pump(); }
static void release(int p, bool settle = true) { fake_ms += 10; down &= ~(1u << p); apply(down); if (settle) pump(); }
static void tapk(int p) { press(p); release(p); }
enum { P_DOT = 15, P_DASH = 16, P_OK = 17, P_BS = 14, P_ENT = 18, P_SPC = 19, P_CAPS = 5, P_LYN = 21, P_ESC = 0, P_FN = 3 };
static void morse(const char *code, bool ok = true) { for (; *code; code++) tapk(*code == '-' ? P_DASH : P_DOT); if (ok) tapk(P_OK); }
static void expect(const char *want, const char *what) { CHECK(hostbuf == want, "%s: may tinh co '%s', mong '%s'", what, hostbuf.c_str(), want); }
static void to_morse_layer(void) { for (int i = 0; i < 3; i++) tapk(P_LYN); }   // nut layer nho 3 lan: L0 -> L1 -> L2 -> L3
static void reset_all(void) { hostbuf.clear(); memset(prevk, 0, 6); usb_up = true; down = 0; tl = 0; defaults(); relayer(); morse_commit(&mrs); qh = qt = 0; tap = 0; tap_phase = 0; caps = false; pump(); }

int main(int argc, char **argv) {
  if (argc > 1 && !strcmp(argv[1], "--led")) {   // in tung khung hinh LED de doi chieu voi ban JS (cross_check.js)
    for (int m = 1; m <= NFX; m++) { printf("%d:", m); for (uint32_t t = 0; t < 300; t++) printf(" %u", (unsigned)led_frame(m, t)); printf("\n"); }
    return 0;
  }
  reset_all();
  CHECK(KM(3, 15) == KC_MORSE_DOT && KM(3, 16) == KC_MORSE_DASH && KM(3, 17) == KC_MORSE_OK, "defaults(): layer 3 chua dat phim Morse");
  to_morse_layer();
  CHECK(res_key(P_DOT) == KC_MORSE_DOT && res_key(P_OK) == KC_MORSE_OK, "chua vao layer Morse sau 3 lan nhan nut layer");
  CHECK(res_key(8) == 0 && res_key(P_FN) == MO(6), "phim so 7 phai bi chan, FN phai con dung duoc");

  // yeu cau goc: 1 cham -> E ; them cham/gach -> xoa ky tu truoc, hien ky tu moi
  tapk(P_DOT); expect("e", "1 cham");
  tapk(P_DOT); expect("i", "2 cham");
  tapk(P_DASH); expect("u", "..-");
  tapk(P_DASH); expect("", "..-- khong hop le: phai xoa het");
  tapk(P_DOT); tapk(P_DOT); expect("?", "..--.. = ?  (Shift + /)");
  tapk(P_OK); expect("?", "Xong chot ky tu, khong xoa");
  tapk(P_DOT); expect("?e", "ky tu moi sau khi chot");
  tapk(P_BS); expect("?", "Backspace xoa ky tu dang hien");
  tapk(P_DOT); expect("?e", "sau Backspace go tiep la ky tu moi");

  // 1 cau day du, dung phim Space / Enter that
  reset_all(); to_morse_layer();
  morse("...."); morse("."); morse(".-.."); morse(".-.."); morse("---"); tapk(P_SPC); morse(".--"); morse("---"); morse(".-."); morse(".-.."); morse("-.."); tapk(P_ENT); morse("...--"); morse(".--.-.", false);
  expect("hello world\n3@", "cau hoan chinh");

  // phim khac chot ky tu: cham (e), Space, cham -> "e e", khong duoc xoa dau cach
  reset_all(); to_morse_layer();
  tapk(P_DOT); tapk(P_SPC); tapk(P_DOT); expect("e e", "Space chot ky tu"); tapk(P_DASH); expect("e a", "sua ky tu moi, khong dung Space");

  // bam nhanh: nhieu lan bam lien tiep khi hang doi chua kip xa
  reset_all(); to_morse_layer();
  for (int r = 0; r < 20; r++) {
    for (const char *c = "...-..-"; *c; c++) { int p = *c == '-' ? P_DASH : P_DOT; press(p, false); release(p, false); fake_ms++; hid_task(); }
    pump(); tapk(P_OK);
  }
  { std::string w; for (int i = 0; i < 20; i++) w += '$'; expect(w.c_str(), "bam nhanh 20 lan $"); }

  // mat ket noi giua chung: ky tu tam khong toi may tinh -> khong xoa nham ky tu truoc
  reset_all(); to_morse_layer();
  hostbuf = "abc"; usb_up = false; tapk(P_DOT); pump(); usb_up = true; pump();
  tapk(P_DASH); expect("abct", "sau khi mat ket noi, lan bam ke tiep khong duoc Backspace 'c'");

  // combo ghi thang vao hang doi (khong qua fire): van phai chot ky tu Morse dang go
  reset_all(); to_morse_layer(); KM(3, 12) = 0x04; KM(3, 13) = 0x05; CB(0, 0) = 0x04; CB(0, 1) = 0x05; CB(0, 2) = 0x1D;   // a + b -> z
  tapk(P_DOT); expect("e", "truoc combo"); press(12); press(13); release(12); release(13); expect("ez", "combo a+b -> z");
  tapk(P_DOT); expect("eze", "sau combo, cham moi la ky tu moi (khong Backspace 'z')");

  // Caps Lock (phim '/') tren layer Morse: chu in hoa, so khong doi
  reset_all(); to_morse_layer();
  tapk(P_CAPS); morse("..."); morse("---"); morse("...", false); expect("SOS", "Caps Lock"); tapk(P_OK); tapk(P_CAPS); morse(".----", false); expect("SOS1", "so khong doi");

  // ESC (pos 0) ve layer 0: phim Morse khong con tac dung
  reset_all(); to_morse_layer();
  tapk(P_ESC); CHECK(res_key(P_DOT) == 0x59, "ESC phai ve layer 0 (phim 1 = KP1)");

  // ===== Morse gan vao BAT KY phim o BAT KY layer =====
  reset_all();   // layer 0 (khong vao layer Morse): gan thang len 3 phim so 7 8 9
  KM(0, 8) = KC_MORSE_DOT; KM(0, 9) = KC_MORSE_DASH; KM(0, 10) = KC_MORSE_OK;
  tapk(8); expect("e", "Morse tren layer 0: cham"); tapk(9); expect("a", "Morse tren layer 0: .-"); tapk(10); tapk(8); expect("ae", "Xong roi go tiep");
  reset_all();   // layer 1 (Media) dang bat, phim Morse nam o layer 1, phim con lai xuyen xuong layer 0
  KM(1, 12) = KC_MORSE_DOT; KM(1, 13) = KC_MORSE_DASH; tl = 1u << 1; relayer();
  tapk(12); tapk(12); tapk(13); expect("u", "Morse tren layer 1");
  tapk(P_ENT); tapk(12); expect("u\ne", "phim thuong (Enter layer 1) chot ky tu");
  reset_all();   // 3 phim Morse o cac layer khac nhau cung luc: layer 2 bat, phim cham o layer 5 (bat bang tl)
  KM(5, 4) = KC_MORSE_DOT; KM(2, 1) = KC_MORSE_DASH; tl = (1u << 2) | (1u << 5); relayer();
  tapk(4); tapk(1); expect("a", "Morse rai tren nhieu layer");
  // tap dance: bam = cham, giu = gach
  reset_all(); KM(0, 8) = TD(1); TDW(1, 0) = KC_MORSE_DOT; TDW(1, 1) = KC_MORSE_DASH;
  press(8); release(8); fake_ms += 250; td_task(fake_ms); pump(); expect("e", "tap dance: bam -> cham");
  press(8); fake_ms += 250; td_task(fake_ms); pump(); release(8); expect("a", "tap dance: giu -> gach (.-)");
  // combo cho ra phim Morse
  reset_all(); KM(0, 12) = 0x04; KM(0, 13) = 0x05; CB(0, 0) = 0x04; CB(0, 1) = 0x05; CB(0, 2) = KC_MORSE_DASH;
  press(12); press(13); release(12); release(13); expect("t", "combo -> gach");

  // ===== LED: gan che do bat ky vao phim bat ky =====
  reset_all();
  for (int m = 1; m <= NFX; m++) { KM(0, 8) = KC_LED_SET + m - 1; tapk(8); CHECK(LD(LD_MODE) == m, "phim LED %d: che do = %d", m, LD(LD_MODE)); }
  CHECK(NFX == 16, "phai co 16 che do LED");
  KM(0, 9) = KC_LED_NEXT; KM(0, 10) = KC_LED_PREV; tapk(9); CHECK(LD(LD_MODE) == 1, "NEXT tu 16 -> 1, ra %d", LD(LD_MODE)); tapk(10); CHECK(LD(LD_MODE) == 16, "PREV tu 1 -> 16");
  CHECK(st_pend, "doi che do bang phim phai dat co luu tre");
  KM(0, 8) = TD(2); TDW(2, 0) = KC_LED_SET + 4; press(8); release(8); fake_ms += 250; td_task(fake_ms); CHECK(LD(LD_MODE) == 5, "LED qua tap dance, ra %d", LD(LD_MODE));
  for (int m = 1; m <= NFX; m++) {   // moi che do: ra trong 6 bit; che do 2..16 co luc sang
    unsigned any = 0; for (uint32_t t = 0; t < 400; t++) { uint8_t f = led_frame(m, t); CHECK(f <= 0x3F, "che do %d tick %u ra 0x%X", m, t, f); any |= f; }
    CHECK(m == 1 ? any == 0 : any != 0, "che do %d: any=0x%X", m, any);
  }
  { unsigned seen[64] = {0}; for (uint32_t t = 0; t < 64; t++) seen[led_frame(11, t)] = 1; int n = 0; for (int i = 0; i < 64; i++) n += seen[i]; CHECK(n == 64, "che do 11 dem nhi phan du 64 gia tri, co %d", n); }
  { bool a = false, b = false; for (uint32_t t = 0; t < 6; t++) { if (led_frame(3, t) == (1u << t)) a = true; if (led_frame(13, t) == (1u << (5 - t))) b = true; } CHECK(a && b, "che do 3 xuoi / 13 nguoc"); }

  // ===== ten layer: doc / ghi qua raw HID =====
  reset_all(); ln_clear();
  { uint8_t b[32] = {0}; b[0] = 9; b[1] = 12; b[2] = 5; memcpy(b + 3, "Nokia", 5); raw_process(b, 32);
    CHECK(ln_dirty, "ghi ten phai danh dau can luu"); uint8_t r[32] = {0}; r[0] = 8; r[1] = 12; r[2] = 5; raw_process(r, 32);
    CHECK(rep[0] == 8 && rep[1] == 12 && rep[2] == 5 && !memcmp(rep + 3, "Nokia", 5), "doc lai ten layer 1");
    uint8_t u[32] = {0}; u[0] = 9; u[1] = 24; u[2] = 6; memcpy(u + 3, "\xC4\x90\xC3\xA8n", 5); raw_process(u, 32);   // "Đèn" UTF-8 (5 byte) + 0
    uint8_t q2[32] = {0}; q2[0] = 8; q2[1] = 24; q2[2] = 6; raw_process(q2, 32); CHECK(!memcmp(rep + 3, "\xC4\x90\xC3\xA8n\0", 6), "ten UTF-8 giu nguyen byte");
    uint8_t bad[32] = {0}; bad[0] = 9; bad[1] = 190; bad[2] = 5; memset(bad + 3, 'x', 5); uint8_t before[4 + L * LNL]; memcpy(before, lnimg, sizeof before); raw_process(bad, 32);
    CHECK(!memcmp(before, lnimg, sizeof before), "ghi vuot 192 byte phai bi tu choi");
    uint8_t full[32] = {0}; full[0] = 9; full[1] = 36; full[2] = 12; memset(full + 3, 'A', 12); raw_process(full, 32);   // ten 12 byte khong co byte 0
    save_all(); CHECK(lnimg[4 + 36 + 11] == 0 && lnimg[4 + 36 + 10] == 'A', "luu: ten luon ket thuc bang 0 (cat byte cuoi)");
    CHECK(!ln_dirty, "sau luu het dirty");
    uint8_t d[32] = {0}; d[0] = 4; raw_process(d, 32); CHECK(lnimg[4 + 12] == 0 && lnimg[4 + 24] == 0 && ln_dirty, "mac dinh firmware xoa ten layer");
    uint8_t in[32] = {0}; in[0] = 5; raw_process(in, 32); CHECK(rep[4] == 13, "info bao FW_VER 13, ra %d", rep[4]); CHECK(rep[5] == 16, "info bao 16 che do LED");
  }

  printf("%d kiem tra end-to-end, %d loi\n", checks, fails);
  return fails ? 1 : 0;
}

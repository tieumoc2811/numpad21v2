# Numpad21 - firmware tu viet (nRF52840, USB + BLE)

Thay ban ZMK cu. Logic giong **Macropad15**: 16 layer, tap dance (4), combo (4), latch/toggle Space-Shift-Ctrl-Alt,
Nokia multitap, go Morse 3 phim + goi y (xem `MORSE.md`), 32 macro (moi macro toi da 234 byte buoc), cau hinh luu vao flash bang trang web `firmware/numpad21.html` (WebHID, Chrome/Edge, qua cap USB).
Phan cung giu nguyen: Pro Micro nRF52840 (nice!nano v2), ma tran 7x7 co diode, 6 day LED (S8550), nut layer nho. So do chan trong `firmware/numpad21/board.h`.

## Ket noi USB / BLE
- Gui qua **USB** khi cam cap, qua **BLE** khi khong cam (mac dinh uu tien USB). Duong uu tien chua san sang thi tu dung duong con lai.
- 5 profile BLE (moi profile 1 dia chi + ten rieng: "Numpad21 1".."Numpad21 5"). Ghep doi: chon profile, vao Bluetooth tren may, chon "Numpad21 n".
- Giu **FN** (o tren cung ben phai) de dung layer FN:
  | Phim | Chuc nang |
  |---|---|
  | hang 2: `/` `*` `-` | BT1, BT2, BT3 |
  | `+` / `ENTER` | BT4 / BT5 |
  | `1` | xoa toan bo ghep doi BLE |
  | `0` | ep dung USB |
  | `BSPC` (cot 3 hang tren) | doi USB <-> BLE |
  | `TAB` | layer ke tiep |
  | `ESC` | bootloader (nap UF2) |
  | `.` | doi che do LED |
- Nut layer nho: chuyen layer vong 0..5 (Numpad > Media > Nokia > **Morse** > L4 > L5).
- Ngu sau sau 15 phut khong dung khi khong cam USB; bam phim bat ky de thuc (phim dau tien bi mat).
- Cau hinh (raw HID) CHI chay qua USB. BLE chi de go phim.

## Layer mac dinh (giong keymap ZMK cu)
L0 Numpad, L1 Media/dieu huong, L2 Nokia (`*` = giu Shift, `/` = Caps Lock, `+` = Backspace), **L3 Morse** (Cham `1`, Gach `2`, Xong `3`, Goi y `.`, Space `0`, Enter, Backspace `+`; xem `MORSE.md`), L4-L5 trong, L6 FN.

## Build
Day len GitHub -> Actions -> "Build Numpad21 firmware" -> tai artifact `numpad21_nrf52840_uf2`. Hoac dung ban build san `prebuilt/numpad21_nrf52840.uf2`.
Build tay: Arduino CLI + core `adafruit:nrf52@1.7.0`, board `adafruit:nrf52:feather52840`, sketch `firmware/numpad21`, roi doi hex -> UF2 bang `uf2conv.py -c -f 0xADA52840`.

## Nap
Bam RESET 2 lan nhanh -> o `NICENANO` -> keo file `.uf2` vao. Firmware nap tai 0x26000 (SoftDevice S140 6.1.1): mo `INFO_UF2.TXT` tren o do de kiem tra bootloader dung S140 6.1.1.

## Luu y
- Neu cac phim bi dao (diode lap nguoc huong), dat `DIODE_ROW2COL` = 1 trong `board.h` (hoac `-DDIODE_ROW2COL=1`).
- Khong co OLED / encoder nhu Macropad15 (numpad21 khong co phan cung do). Doi ten layer co (tu FW_VER 13).
- LED chi sang khi co 5V USB, tru khi bat "Sang ca khi chay pin" trong app.
- VID/PID: `0xC1A0` / `0x0021` (app tim theo VID + usage page 0xFF60).

## Nang cap 16 layer / 32 macro (FW_VER 11)
- Firmware doi tu 32 layer/16 macro sang **16 layer/32 macro**. Dinh dang cau hinh thay doi nen sau khi nap firmware moi, cau hinh va macro cu se tu ve mac dinh.
- Moi o macro 252 byte (234 byte buoc); toan bo macro 8068 byte, van gon trong 2 khoi flash.
- Trang `firmware/numpad21.html` can ban moi (kiem tra firmware v11). `prebuilt/` da bi go vi chua build lai: hay build bang GitHub Actions.

## Go Morse (FW_VER 12)
- 3 phim Cham / Gach / Xong + phim Goi y (`0x7060..0x7063`): ky tu hien ngay tren may tinh, bam them thi Backspace roi thay bang ky tu cua ma dai hon. Chi tiet: `MORSE.md`.
- Logic thuan C trong `firmware/numpad21/morse.h` (dung chung voi Macropad15); trang `numpad21.html` co tab **Morse** (o thu, bang ma, gan phim).
- Cau hinh cu trong flash duoc giu nguyen (khong doi dinh dang): layer 3 cu van trong, vao tab Morse -> «Bo cuc Morse day du -> Layer 3» -> Luu.
- Kiem thu: `sh test/run_tests.sh`.
- Den LED bao khi go Morse (nhay ngan = cham, nhay dai = gach, thanh den = so net, nhay doi = chot ky tu), bat/tat bang phim Morse `Den` `0x7064`: xem `MORSE.md`.

## FW_VER 13: 16 che do LED + ten layer
- 16 che do LED (them: hai den chay duoi, dem nhi phan, nhip tim, chay nguoc, co vao giua, hai nua doi phien, chop ngau nhien). Phim `0x7020..0x702F` = chon thang che do 1..16; `LED +` / `LED -` doi tiep / lui. Che do doi bang phim duoc nho sau khi tat nguon (ghi flash sau 2 giay).
- Ten layer (toi da 11 byte UTF-8 moi ten) luu o file `/n21nm` qua raw HID lenh 8 (doc) / 9 (ghi); app bam dup vao ten layer hoac nut "Doi ten". Can firmware v13.
- Layer Morse mac dinh: ENTER = Tap dance 4 (bam 1 lan = Xong, bam 2 lan = Space, giu = Enter).

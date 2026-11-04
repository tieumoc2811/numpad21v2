# Go Morse 3 phim + goi y (firmware v12)

Port tu Macropad15 (RP2350). Logic thuan C trong `firmware/numpad21/morse.h`, dung chung 100% voi ban Macropad15.

- 3 phim: **Cham** (`0x7060`), **Gach** (`0x7061`), **Xong** (`0x7062`) + phim thu 4 **Goi y** (`0x7063`). Gan duoc vao phim bat ky o layer bat ky, tap dance, ket qua combo (Numpad21 khong co encoder).
- Bam cham/gach: ky tu hien NGAY tren may tinh (`.` -> e, `..` -> i, `..-` -> u). Bam them: gui Backspace xoa ky tu cu roi gui ky tu cua ma dai hon. Ma chua co ky tu (vd `..--`) chi xoa. Toi da 7 net.
- **Xong** chot ky tu (chi xoa phan goi y neu dang bat). Space / Backspace / Enter / moi phim khac / macro cung chot ky tu dang go.
- **Goi y tren van ban**: sau ky tu go them ` [.i -a]` = them cham ra `i`, them gach ra `a`. Vi du bam `.` -> `e [.i -a]`; bam `.` nua -> `i [.s -u]`; Xong -> `i`. Phan goi y tu xoa o lan bam ke tiep, khi bam Xong, hoac khi bam bat ky phim khac (phim do duoc giu lai den khi xoa xong, nha som van khong mat phim). Mac dinh bat, doi bang `MORSE_HINT_DEF` trong `numpad21.ino` hoac bang phim Morse **Goi y**; trang thai bat/tat KHONG luu flash.
  - Luu y: Word / Google Docs / macOS tu doi `...` thanh `…` (hoac `--` thanh gach dai) lam lech so lan Backspace -> dung o van ban thuan (Notepad, trinh duyet, terminal) hoac tat tu sua. Tat bo go tieng Viet (Unikey/Telex) khi go Morse. Giu Shift thi goi y hien dau Shift, khong anh huong ket qua vi bi xoa.
  - Hang doi phim 64 o (truoc 16). Nhan qua nhanh thi lan bam do bi bo qua (khong lech trang thai).
- **Layer 3 mac dinh** (thiet bi moi / sau «Mac dinh firmware»):

  | | cot 1 | cot 2 | cot 3 | cot 4 |
  |---|---|---|---|---|
  | hang 1 | ve layer 0 | - | - | FN |
  | hang 2 | - | Caps Lock | giu Shift | - |
  | hang 3 | - | - | - | |
  | hang 4 | - | - | - | Backspace (`+`) |
  | hang 5 | **Cham** (`1`) | **Gach** (`2`) | **Xong** (`3`) | Enter |
  | hang 6 | Space (`0`, rong 2 o) | | **Goi y** (`.`) | |

  Nut layer nho chuyen vong L0 > L1 > L2 > **L3 Morse** > L4 > L5: bam 3 lan tu layer 0 de vao Morse.
- Thiet bi da co cau hinh cu (layer 3 van trong, vi cau hinh trong flash duoc giu khi nap firmware moi): mo `numpad21.html` -> tab **Morse** -> «Bo cuc Morse day du -> Layer 3» -> Luu vao ban phim. Hoac tab Ban phim -> chon phim -> nhom Morse.
- Mat ket noi (rut USB, BLE chua noi): phim cho bi bo, trang thai Morse ve ky tu moi, nen noi lai khong bi Backspace thua.
- Tap dance: khi bam phim khac luc tap dance dang cho, tap dance duoc chot NGAY theo thu tu bam (truoc day tre 200 ms nen "Xong" co the chay sau "Cham" -> `.` sau Xong ra `i` thay vi `e`). Thay doi nay ap dung cho moi tap dance, khong chi Morse.
- Khac Macropad15: khong co OLED (khong hien `E .` / `OK`), khong co encoder.
- Kiem thu: `sh test/run_tests.sh` (gcc + g++ + node): test don vi/fuzz `morse.h`, mo phong end-to-end chinh `numpad21.ino` (stub Arduino / Bluefruit / TinyUSB), doi chieu C <-> JS (`numpad21.html`) tren 120000 su kien.

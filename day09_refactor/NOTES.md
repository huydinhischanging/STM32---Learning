# Day 9: Refactor (tách driver thành module)

## Mục tiêu
Không thêm tính năng mới, không cần linh kiện mới. Dọn lại code Day 8 (SPI flash
W25Q32) thành các module driver độc lập, tái sử dụng được, thay vì dồn hết vào
`main.c` như các bài trước.

## Việc đã làm
- Copy nguyên project `day08_spi` (đã xác nhận chạy đúng) làm nền, đổi tên toàn bộ
  file cấu hình (`.project`, `.cproject`, `.ioc`, `.mxproject`) từ `day08_spi` sang
  `day09_refactor`.
- Tách 2 module:
  - **`uart_printf.h/.c`**: gói `setvbuf` + `__io_putchar` (lặp lại y hệt từ Day 3
    đến Day 8) thành 1 hàm `UART_Printf_Init(UART_HandleTypeDef *huart)`. Biến
    `s_huart` (con trỏ UART) khai báo `static` ở mức file — không lộ ra ngoài.
  - **`w25qxx.h/.c`**: gói 6 hàm thao tác flash của Day 8 thành API dùng
    **struct handle** (`W25QXX_HandleTypeDef` chứa `hspi`, `cs_port`, `cs_pin`)
    thay vì viết cứng tên `hspi4`/`FLASH_CS_Pin` — nhờ vậy dùng lại được cho chip
    flash khác trên chân SPI/CS khác mà không sửa code bên trong module. 5 hàm phụ
    trợ (`CS_Low`, `CS_High`, `WriteEnable`, `ReadStatus`, `WaitBusy`) khai báo
    `static`, không xuất hiện trong `.h`, `main.c` không gọi được trực tiếp.
- `main.c` sau khi dọn chỉ còn gọi 6 hàm public: `UART_Printf_Init`, `W25QXX_Init`,
  `W25QXX_ReadJEDEC`, `W25QXX_SectorErase`, `W25QXX_PageProgram`, `W25QXX_Read` —
  không còn thao tác CS hay gửi/nhận SPI thô nằm lẫn trong logic chính.
- Import project vào CubeIDE (File → Open Projects from File System), build —
  CubeIDE tự quét `Core/Src` và nhận diện 2 file mới, không cần khai báo thủ công.
- Nạp lên board, xác nhận kết quả **giống hệt Day 8**: `Manufacturer = 0xEF,
  DeviceID = 0x40 0x16`, ghi/đọc lại chuỗi test khớp (`TRUNG KHOP`).

## Vấn đề gặp phải

1. **Copy project giữ nguyên tên cũ `day08_spi` trong mọi file cấu hình**: CubeIDE
   Project Explorer hiện đúng tên thư mục `day09_refactor` khi import, nhưng project
   name bên trong (`.project`, `.cproject`) vẫn là `day08_spi` — dễ gây nhầm khi có
   nhiều project mở cùng lúc. Sửa bằng `sed` thay toàn bộ chuỗi `day08_spi` thành
   `day09_refactor` trong `.project`, `CA7/.project`, `CM4/.project`, `CM4/.cproject`,
   và đổi tên + sửa nội dung file `.ioc`, `.mxproject`.

2. **Project mới copy chưa có thư mục `Debug`** (đã xóa khi copy để tránh lẫn build
   cũ): không thể build bằng `make` dòng lệnh ngay như Day 7/8, vì chưa có
   `subdir.mk` do CubeIDE tự sinh khi quét lần đầu. Phải import và build qua CubeIDE
   GUI ít nhất 1 lần để nó tự phát hiện 2 file `.c` mới trong `Core/Src`.

## Giải thích cơ chế

- **Compile riêng, link chung**: mỗi file `.c` được biên dịch độc lập thành 1 file
  `.o` (bước có cờ `-c` trong lệnh `arm-none-eabi-gcc`), sau đó bước cuối (không có
  `-c`) gộp toàn bộ `.o` lại thành 1 file `.elf` — đây là bước "link". `static` ảnh
  hưởng tới nội dung file `.o`.
- **Linkage (external vs internal)**: hàm/biến không có `static` được đưa vào
  "danh sách xuất khẩu" của file `.o` (external linkage) — file `.o` khác gọi được
  qua bước link. Có `static` thì không được xuất khẩu (internal linkage) — file `.o`
  khác không cách nào gọi/truy cập được, dù có viết `extern` cũng bị linker báo
  "undefined reference".
- **Hai lớp bảo vệ**: lớp 1 (sớm hơn) là compiler — `main.c` không thấy khai báo của
  biến/hàm `static` vì nó không nằm trong file `.h` được `#include`, nên gõ sai sẽ
  báo lỗi biên dịch ngay. Lớp 2 (nếu cố lách qua `extern` thủ công) là linker, chặn
  bằng lỗi liên kết.
- **`static` có 2 nghĩa khác nhau tùy vị trí đặt**: bên trong 1 hàm (như biến
  `last_press` ở Day 5) nghĩa là "giữ giá trị qua các lần gọi hàm". Ở mức file (bên
  ngoài mọi hàm, như `s_huart` ở Day 9) nghĩa là "giấu khỏi file khác" (internal
  linkage) — hai ý nghĩa không liên quan nhau, chỉ trùng từ khóa.
- **Đóng gói (encapsulation)** trong C không cần class như C++/Java: dùng cặp
  `.h` (giao diện public, những gì file khác được phép gọi) + `.c` với `static`
  (chi tiết cài đặt private, giấu khỏi bên ngoài).
- **Struct handle cho phép tái sử dụng**: `W25QXX_HandleTypeDef` là khuôn mẫu học
  từ chính cách HAL của ST tự thiết kế (`hspi4`, `huart7` cũng là handle) — SPI và
  chân CS trở thành *tham số*, không phải *tên cố định* viết chết trong hàm.

## TODO
- Có thể tách thêm module cho Day 7 (`bh1750.h/.c`) hoặc Day 6 (`adc_reader.h/.c`)
  nếu muốn luyện tập thêm mẫu refactor tương tự.
- Tồn đọng: Debug Authentication chặn SWD, RPMsg chưa truyền dữ liệu (Day 2), LED_B
  bị đèn heartbeat của Linux ghi đè (Day 5, đã cập nhật NOTES Day 3), dây SPI vẫn
  chưa ổn định lâu dài (Day 8).

## Kết quả
Refactor xong, build sạch, nạp lên board cho kết quả **giống hệt Day 8** — xác nhận
chỉ tổ chức lại code, không đổi hành vi chương trình. `main.c` từ hơn 100 dòng logic
rút gọn còn khoảng 10 dòng gọi hàm.

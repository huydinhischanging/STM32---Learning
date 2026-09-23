# Day 8: SPI (đọc/ghi chip nhớ flash W25Q32)

## Mục tiêu
Đọc JEDEC ID để xác nhận đúng chip, sau đó xóa/ghi/đọc lại 1 chuỗi dữ liệu thật vào
W25Q32 qua SPI, in kết quả qua UART7 (giống Day 3/6/7).

## Việc đã làm
- Tạo project `day08_spi` qua CubeMX (Board Selector, STM32MP157D-DK1).
- **SPI4 giao cho M4**: bỏ tích A7NS trước, tích M4, Mode = Full-Duplex Master.
  SPI4 là khối SPI duy nhất được board DK1 nối ra connector Arduino (D10-D13, CN13),
  tra từ UM2637 Table 23/24 — các khối SPI khác trên chip không có lối ra vật lý.
  Chân: **PE11 = D10 = NSS**, **PE12 = D13 = SCK**, **PE13 = D12 = MISO**,
  **PE14 = D11 = MOSI**.
- Sửa 2 chỗ CubeMX sinh sai mặc định:
  - **Data Size 4-bit → 8-bit**: W25Q32 giao tiếp theo byte, để 4-bit sẽ cắt lệnh sai.
  - **PE11 đổi từ `SPI4_NSS` (phần cứng tự động) sang `GPIO_Output`** (nhãn
    `FLASH_CS`), tự điều khiển CS bằng tay để gộp được nhiều bước (lệnh + địa chỉ +
    dữ liệu) vào đúng 1 giao dịch. Nhớ gán Pin Context = Cortex-M4 FW cho PE11 (lần
    đầu quên bước này, `PinAttribute` vẫn là `Free`).
- Bật **UART7** như các bài trước (PE7/PE8), lần này CubeMX gán đúng ngay từ đầu.
- Code: đọc JEDEC ID (lệnh `0x9F` + 3 byte dummy), rồi các hàm phụ trợ
  `flash_write_enable` (`0x06`), `flash_read_status` (`0x05`), `flash_wait_busy`
  (chờ bit WIP về 0), `flash_sector_erase` (`0x20` + địa chỉ 24-bit),
  `flash_page_program` (`0x02` + địa chỉ + dữ liệu), `flash_read` (`0x03` + địa chỉ).
  Toàn bộ đặt trong `USER CODE BEGIN 4`, khai báo prototype ở `USER CODE BEGIN PFP`
  (vì được gọi trong `main()` ở `USER CODE BEGIN 2`, nằm phía trên phần định nghĩa).
- Xóa sector 0x000000, ghi chuỗi `"Hello Flash from Day 8!"`, đọc lại, so sánh bằng
  `strcmp`.
- Build bằng `make` dòng lệnh (như Day 7), nạp qua `scp` + remoteproc.

## Vấn đề gặp phải

1. **Nhầm giữa "Ball", "Pin", "Assignment" khi tra sổ tay**: bảng "STM32MP157x-DKx
   I/O assignment" (ball-out toàn chip) và bảng "I/O configuration of the ARDUINO
   connector" (Table 23/24) là 2 bảng khác nhau, dễ nhầm vì tình cờ trùng số (ví dụ
   "Ball D10" và "Arduino D10" không liên quan gì nhau). Phải dùng đúng bảng Arduino
   connector để biết chân nào thật sự lộ ra ngoài cho mình dùng.

2. **CubeMX mặc định Data Size = 4-bit và NSS phần cứng**: cả hai đều không phù hợp
   cho giao tiếp byte-oriented với flash chip, phải tự đổi thủ công (xem mục Việc đã
   làm).

3. **Quên gán Pin Context cho PE11 sau khi đổi sang GPIO_Output**: `.ioc` cho thấy
   `PinAttribute=Free` dù đã đổi Signal đúng — lặp lại đúng bài học Day 1/4/5/6/7:
   đổi tín hiệu chân không tự động giao chân đó cho M4.

4. **`status = 0` (HAL_OK) không đảm bảo có chip trả lời — khác hẳn I2C**: SPI không
   có cơ chế ACK như I2C, nên `HAL_SPI_TransmitReceive`/`Transmit`/`Receive` luôn báo
   thành công miễn là không lỗi phần cứng của chính STM32, bất kể có chip nối vào hay
   không. Phải nhìn vào **nội dung dữ liệu** (JEDEC ID có đúng `0xEF/0x40/0x16`
   không, thanh ghi trạng thái có hợp lý không), không thể chỉ tin vào status code
   như cách chẩn đoán I2C ở Day 7.

5. **Kết nối vật lý chập chờn nghiêm trọng, tái diễn nhiều lần**: qua nhiều lần chạy
   lại (không đổi code), kết quả JEDEC ID lần lượt ra `0xFF` (lơ lửng), rồi đúng
   `0xEF/0x40/0x16`, rồi `0x00`, rồi lại đúng — có lúc **ngay trong cùng 1 lần chạy**,
   hai lần đọc thanh ghi trạng thái liên tiếp (`write_enable` gọi 2 lần) ra 2 giá trị
   khác nhau (`0xFF` rồi `0x00`) dù code giống hệt nhau. Đây là bằng chứng dứt khoát
   cho tiếp xúc điện không ổn định (dây jumper hoặc breadboard), không phải lỗi
   logic — đã xác nhận bằng cách đo continuity từng dây (Day 7) và tìm ra dây **CS**
   là nghi phạm chính lúc đầu, nhưng ngay cả sau khi chỉnh lại, hiện tượng chập chờn
   vẫn còn xuất hiện lẻ tẻ ở các dây khác.
   Bài học: khi HAL báo "thành công" nhưng dữ liệu vô lý/nhảy giữa các lần chạy giống
   hệt nhau, nghi ngay phần cứng (tiếp xúc dây), đừng tìm lỗi trong code trước.

## Giải thích cơ chế
- **SPI dùng 4 dây** (MOSI, MISO, SCK, CS/NSS), không có địa chỉ như I2C — mỗi thiết
  bị cần 1 dây CS riêng, Master hạ CS xuống thấp để "chọn" đúng 1 thiết bị.
- **Gửi và nhận đồng thời (full-duplex)**: mỗi xung SCK dịch chuyển 1 bit ra MOSI
  *và* 1 bit vào MISO cùng lúc. Muốn nhận dữ liệu, Master vẫn phải gửi gì đó (byte
  dummy `0x00`) để tạo đủ xung nhịp — SPI không có cách nào "chỉ nhận" mà không phát.
- **Data Size** quyết định 1 lần truyền gồm bao nhiêu bit; phải khớp với chuẩn giao
  tiếp của thiết bị (8-bit cho hầu hết flash chip).
- **Write Enable Latch (WEL)**: chip tự khóa ghi sau mỗi lần ghi/xóa xong, phải gửi
  `0x06` mở khóa lại trước mỗi lần ghi/xóa mới — cơ chế chống ghi nhầm do nhiễu.
- **Write In Progress (WIP)**: bit 0 của thanh ghi trạng thái, đọc bằng lệnh `0x05`,
  báo chip có đang bận xóa/ghi hay không — phải chờ về 0 mới gửi lệnh tiếp theo.
- **Địa chỉ 24-bit (3 byte)**: đủ đánh số tới hơn 4 triệu byte (32Mbit = 4MB).
- **Phải xóa (erase) trước khi ghi đè**: đặc tính vật lý của NOR flash — ghi chỉ đổi
  bit 1→0 được, muốn đổi 0→1 phải xóa cả khối (đưa về toàn `0xFF`).

## TODO
- Dây/kết nối vật lý cho SPI vẫn chưa ổn định lâu dài — cần dây jumper tốt hơn hoặc
  hàn cố định nếu muốn dùng tiếp về sau.
- Tồn đọng: Debug Authentication chặn SWD, RPMsg chưa truyền dữ liệu (Day 2), LED_B
  bị đèn heartbeat của Linux ghi đè (Day 5, đã cập nhật NOTES Day 3).

## Kết quả
Đọc đúng JEDEC ID (`Manufacturer = 0xEF, DeviceID = 0x40 0x16`), xóa sector, ghi
chuỗi `"Hello Flash from Day 8!"`, đọc lại và so khớp chính xác (`Ket qua: TRUNG
KHOP`) — xác nhận bằng terminal thật (PowerShell đọc COM6), không phải suy đoán.
Video demo trong `demo/`.

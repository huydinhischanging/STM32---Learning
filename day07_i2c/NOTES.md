# Day 7: I2C (đọc cảm biến ánh sáng BH1750)

## Mục tiêu
Đọc độ sáng (lux) từ cảm biến BH1750 qua I2C, in kết quả qua UART7 (giống Day 3/6).

## Việc đã làm
- Tạo project `day07_i2c` qua CubeMX (Board Selector, STM32MP157D-DK1).
- **I2C5 giao cho M4**: bỏ tích cột A7NS (Linux) trước, tích M4, Mode = I2C,
  Standard Mode 100 kHz. Chân **PA11 = I2C5_SCL**, **PA12 = I2C5_SDA**.
- **UART7** cấu hình như các bài trước: PE7 = RX, PE8 = TX, Asynchronous, 115200 8N1.
  Lần này CubeMX gán đúng ngay từ đầu, không bị lệch sang PB3/PB4 như Day 3/6.
- Đấu BH1750 vào connector **CN2** (40 chân kiểu Raspberry Pi, tra từ UM2637 Table 25):
  VCC → chân 1 (3V3), GND → chân 6, SDA → chân 3 (PA12), SCL → chân 5 (PA11).
- Code (`USER CODE BEGIN 2`): quét toàn bộ địa chỉ I2C 7-bit bằng
  `HAL_I2C_IsDeviceReady()`, gửi lệnh Power On (`0x01`) rồi lệnh đo liên tục độ phân
  giải cao (`0x10`), chờ 180ms cho lần đo đầu.
  (`USER CODE BEGIN 3`): đọc 2 byte bằng `HAL_I2C_Master_Receive`, ghép thành số
  16-bit, chia 1.2 ra lux, `printf` mỗi 500ms.
- Build bằng `make` dòng lệnh (toolchain của CubeIDE), không cần mở GUI CubeIDE mỗi lần
  sửa code — nhanh hơn để lặp lại thử nghiệm khi debug phần cứng.

## Vấn đề gặp phải

1. **Không thấy dữ liệu trên COM6 lần đầu nạp**: hóa ra module CH340 chưa cắm USB vào
   máy tính (bài học lặp lại từ các Day trước — luôn kiểm tra `Get-CimInstance
   Win32_SerialPort` xem COM6 có tồn tại chưa trước khi nghi ngờ gì khác).

2. **Khởi động lại M4 bằng `stop`/`start` mà không ghi lại tên file firmware**: M4 nạp
   nhầm về firmware mặc định (`rproc-m4-fw`) thay vì `day07_i2c.elf`. Xác nhận qua dmesg
   thấy dòng `Booting fw image rproc-m4-fw`, size khác hẳn file build. Bài học: mỗi lần
   `stop` rồi `start` lại phải ghi lại `echo <tên file> > .../firmware` trước khi `start`.

3. **I2C không giao tiếp được — toàn bộ lệnh trả về `HAL_ERROR`, quét 127 địa chỉ
   không thấy thiết bị nào**: đây là phần tốn công nhất bài này. Cách chẩn đoán:
   - Thêm "giá trị canh gác" (`buf = {0xEE, 0xEE}`) trước khi gọi `HAL_I2C_Master_Receive`
     — nếu đọc thất bại, `buf` giữ nguyên giá trị canh gác, số `raw` đọc ra sẽ khớp đúng
     `0xEEEE = 61166`. Thấy đúng số này xác nhận: I2C chưa từng nhận được byte nào,
     không phải dữ liệu sai mà là "hoàn toàn chưa nhận được gì".
   - In thêm `HAL_StatusTypeDef` của từng lệnh — thấy `status = 1` (lỗi) ngay từ bước
     `power_on` đơn giản nhất, nên khoanh vùng vấn đề nằm ở **tầng vật lý** (dây/mạch),
     không phải logic đọc dữ liệu.
   - Viết vòng quét toàn bộ 127 địa chỉ (`HAL_I2C_IsDeviceReady`) để không phải đoán
     đúng/sai địa chỉ `0x23` hay `0x5C`.
   - Loại trừ nghi ngờ Linux giữ I2C5 (giống ADC ở Day 6): kiểm tra
     `/proc/device-tree/soc/.../i2c@40015000/status` = `disabled`, xác nhận Linux không
     đụng tới I2C5.
   - Đo điện trở giữa SDA và SCL (không phải giữa tín hiệu và nguồn) ra khoảng 21kΩ —
     khớp với 2 điện trở pull-up ~10kΩ trên module nối tiếp qua đường VCC, xác nhận
     module có pull-up thật và 2 dây tín hiệu đã tới đúng chân.
   - Sau khi tắt/bật lại board (không chỉnh gì thêm mà mình xác nhận được), lần quét
     tiếp theo **tìm thấy thiết bị ở đúng `0x23`** và mọi lệnh chạy `status = 0`. Nguyên
     nhân gốc cụ thể (dây lỏng do rung động, hay tiếp xúc breadboard) chưa xác định chắc
     chắn — bài học quan trọng nhất là **cách chẩn đoán** (canh gác + status code + quét
     địa chỉ), không phải nguyên nhân cụ thể lần này.

4. **Nhầm lẫn giữa "đo điện áp" và "đo thông mạch/điện trở" khi debug dây**: đo điện áp
   ở cả hai đầu ra "bình thường" (3.3V) không chứng minh được dây có nối liền mạch, vì
   module có pull-up riêng tự kéo điện áp lên ngay cả khi dây bị đứt/lỏng phía board.
   Phải đo điện trở/thông mạch dọc theo đúng 1 sợi dây (2 đầu của cùng dây) mới xác nhận
   được tính liền mạch.

## Giải thích cơ chế
- **I2C dùng 2 dây (SDA, SCL) cho nhiều thiết bị chung bus**, phân biệt bằng địa chỉ
  7-bit gửi kèm mỗi lần giao tiếp — khác UART cần dây riêng cho mỗi thiết bị.
- **Byte địa chỉ = (địa chỉ 7-bit) << 1 | (bit đọc/ghi)**: HAL của ST yêu cầu tự dịch
  trái 1 bit trước khi truyền vào hàm (`0x23 << 1`), vì hàm không tự làm việc này.
- **Cần bật cảm biến (Power On) trước khi đo**: BH1750 khởi động ở chế độ ngủ để tiết
  kiệm điện.
- **Chờ 180ms sau lệnh đo**: cảm biến cần thời gian tích lũy ánh sáng (giống phơi sáng
  máy ảnh), đọc ngay sẽ ra dữ liệu chưa sẵn sàng.
- **Ghép 2 byte thành số 16-bit theo big-endian** (`(buf[0] << 8) | buf[1]`): BH1750 gửi
  byte cao trước, quy ước ghi trong datasheet của chip.
- **Chia 1.2 ra lux**: hệ số hiệu chuẩn cố định của riêng chip BH1750 ở chế độ
  H-Resolution, không phải công thức vật lý tổng quát.
- **Pull-up là bắt buộc trên I2C**: cả SDA và SCL đều là kiểu open-drain (chỉ kéo được
  xuống thấp), cần điện trở kéo lên để có mức cao khi rảnh — do module tự có sẵn.

## TODO
- Xác định nguyên nhân gốc vì sao lần đầu I2C không chạy rồi tự hết sau khi tắt/bật lại
  (nghi tiếp xúc dây lỏng, chưa xác nhận).
- Dọn code debug (bỏ vòng quét địa chỉ, giá trị canh gác) nếu muốn bản "sạch" hơn — hiện
  giữ nguyên vì hữu ích cho việc học.
- Tồn đọng: Debug Authentication chặn SWD, RPMsg chưa truyền dữ liệu (Day 2), LED_B bị
  đèn heartbeat của Linux ghi đè (Day 5, đã cập nhật vào NOTES Day 3).

## Kết quả
Đọc được độ sáng thật qua BH1750: `raw = 158, lux = 131` ổn định qua nhiều lần đọc,
xác nhận bằng terminal thật (PowerShell đọc COM6), không phải suy đoán. Video demo
trong `demo/`.

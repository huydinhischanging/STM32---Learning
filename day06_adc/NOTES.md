# Day 6: ADC (đọc điện áp analog)

## Mục tiêu
Đọc điện áp từ biến trở WH148 10K bằng ADC của M4, đổi ra mili-vôn rồi in ra máy tính
qua UART7 (dùng lại `printf` + `__io_putchar` của Day 3). Vặn biến trở thì số phải đổi.

## Việc đã làm
- Tạo project `day06_adc` qua CubeMX (Board Selector, STM32MP157D-DK1). Project này
  build ra `STM32CubeIDE/CM4/Debug/day06_adc_CM4.elf` (khác các bài trước).
- Chân đo: **A0 = PF14 = ADC2_INP6** (connector CN17). Tra từ sổ tay UM2637 và database
  chip của CubeMX (`STM32MP157DACx.xml`); hai nguồn khác nhau ở chân PF12 (sổ tay ghi
  ADC1_IN16, database ghi ADC1_INP6), tin database.
- **ADC2 giao cho M4**: mặc định ADC1/ADC2 thuộc Linux (A7NS). Phải bỏ tích cột A7NS
  trước thì ô M4 mới chọn được. Bật kênh IN6 Single-ended.
- Độ phân giải mặc định **16 bit** (giá trị 0..65535), không chạy liên tục, sampling
  time đổi từ 1.5 lên **387.5 chu kỳ** vì nguồn tín hiệu là biến trở (trở kháng cao).
- Bật **UART7** Asynchronous cho M4, gán PE7 = RX và PE8 = TX.
- Code trong `main.c`:
  - `USER CODE BEGIN 2`: `setvbuf(stdout, NULL, _IONBF, 0);` và hiệu chuẩn ADC
    `HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);`
  - `USER CODE BEGIN 3`: `HAL_ADC_Start` → `HAL_ADC_PollForConversion` →
    `HAL_ADC_GetValue`, đổi `mV = adc_value * 3300 / 65535`, `printf`, `HAL_Delay(500)`.
  - `USER CODE BEGIN 4`: `__io_putchar` gọi `HAL_UART_Transmit(&huart7, ...)`.
- Đấu biến trở: một chân ngoài → 3V3 của module CH340, chân ngoài kia → GND, chân giữa
  (con trượt) → A0. Dùng 3V3 của module vì lỗ 3V3 trên CN16 nằm kẹp giữa RST và 5V.

## Vấn đề gặp phải

1. **ADC thuộc Linux theo mặc định**: ô M4 bị khóa cho đến khi bỏ tích A7NS.

2. **CubeMX lại tự gán UART7 TX sang PB4** (giống Day 3). Reset PB4 làm UART7 mất luôn
   chế độ Asynchronous và chân PE7. Cách đúng: gán PE7 = UART7_RX rồi vào
   Connectivity → UART7 đặt Mode = Asynchronous, sau đó kiểm tra file `.ioc` và
   `stm32mp1xx_hal_msp.c` thấy PE7/PE8 mới tin. Một lần build lỗi `huart7 undeclared`
   là dấu hiệu UART7 chưa được sinh code.

3. **Cắm dây 3V3 vào lỗ 3V3 của CN16 thì board reset** (đèn heartbeat tắt, máy tính
   kêu tiếng ngắt USB, `uptime` chỉ vài phút). Lỗ 3V3 nằm kẹp giữa RST và 5V nên đầu
   dây lệch là chạm RST hoặc 5V. Nguyên nhân chính xác chưa xác nhận. Giải quyết:
   dùng chân 3V3 của module USB-TTL.

4. **Thẻ SD lỗi `Input/output error`** sau nhiều lần board bị reset/mất nguồn đột ngột:
   mọi lệnh đọc file đều lỗi, SSH bị từ chối (ping vẫn thông vì nhân nằm trong RAM).
   Kernel đã tự chuyển hệ thống file sang chế độ chỉ đọc. Tắt an toàn bằng
   `echo s/u/o > /proc/sysrq-trigger` qua cổng COM5, rồi rút nguồn, cắm lại thẻ, cấp
   nguồn: Linux tự phục hồi (`recovery complete`), thẻ không hỏng.

5. **COM6 im lặng dù firmware chạy**: nghi module rồi nghi dây. Kiểm tra bằng loopback
   (nối TXD với RXD trên module, gửi chuỗi, nhận lại đúng chuỗi) cho thấy module tốt.
   Đấu lại từng dây (GND, D1→RXD, D0→TXD) thì UART chạy. Bài học: kiểm tra từng đoạn
   của đường truyền độc lập, và đừng kết luận thiết bị hỏng khi chưa loopback.

6. **Số ADC nhảy lung tung / cố định 65535 khi chưa đấu biến trở**: chân A0 để trống
   ("lơ lửng") thì ADC đọc giá trị không xác định. Khi đấu biến trở đúng thì số đổi
   theo lúc vặn. Nguyên nhân cụ thể của lần đọc nhảy `0/32767/49151` chưa xác định.

7. **Nhầm về nơi hiển thị dữ liệu**: firmware chỉ gửi ra UART, máy tính chỉ hiện khi có
   chương trình đọc cổng COM6 (PowerShell `SerialPort` hoặc phần mềm terminal). Cổng COM
   chỉ một chương trình mở tại một thời điểm.

## Giải thích cơ chế
- **ADC (Analog-to-Digital Converter)** đổi điện áp liên tục (analog) thành một con số
  (digital) để CPU xử lý. CPU chỉ hiểu số nên không "đọc" được điện áp trực tiếp.
- **Lấy mẫu rồi chuyển đổi**: ADC có một tụ nhỏ bên trong. Giai đoạn lấy mẫu nối chân
  đo vào tụ để tụ nạp tới điện áp của chân; sau đó ngắt ra và đo điện áp trên tụ. Số
  đọc chỉ đúng nếu tụ nạp đủ đầy trong thời gian lấy mẫu.
- **Sampling time**: nguồn có trở kháng cao (biến trở, tối đa khoảng R/4 = 2.5 kΩ ở giữa)
  nạp tụ chậm, nên cần sampling time dài (387.5 chu kỳ). Độ phân giải càng cao thì càng
  cần nạp chính xác. Chưa so sánh thực nghiệm 1.5 với 387.5 chu kỳ.
- **Biến trở là bộ chia áp**: hai chân ngoài nối 3V3 và GND, chân giữa cho điện áp từ 0
  đến 3.3V theo vị trí vặn. ADC đo điện áp đó.
- **Công thức**: 16 bit thì `mV = giá trị * 3300 / 65535` (3300 là điện áp tham chiếu
  của board, 65535 là giá trị lớn nhất). `printf` số thực trên chip nhúng thường không
  hoạt động nên dùng số nguyên mV.
- **Đất chung**: ADC đo điện áp so với GND của board, nên GND nguồn của biến trở phải
  nối GND board.
- **UART cần nối chéo**: D1 (TX board) → RXD module, D0 (RX board) → TXD module.

## TODO
- So sánh thực nghiệm sampling time 1.5 và 387.5 chu kỳ (độ nhiễu của số đọc).
- Xác định nguyên nhân số đọc nhảy `0/32767/49151` lúc đầu (mình từng nghi nguồn analog
  `vdda` bị Linux tắt, chưa xác nhận).
- Tìm hiểu vì sao cắm dây vào lỗ 3V3 của CN16 làm board reset.
- Kiểm tra Linux có còn giữ ADC2 không (`/sys/bus/iio/devices`).
- Tồn đọng: Debug Authentication chặn SWD, RPMsg chưa truyền dữ liệu (Day 2), LED_B bị
  đèn heartbeat của Linux ghi đè (Day 5), sửa kết luận Day 3 theo phát hiện này.

## Kết quả
Vặn biến trở thì giá trị `ADC = ..., mV = ...` in ra qua UART7 đổi theo, xác nhận bằng
quan sát thật trên terminal (PowerShell đọc COM6). Chưa ghi lại số liệu cụ thể (giá trị
đầu và cuối thang), nên chưa đối chiếu độ chính xác với đồng hồ vạn năng.

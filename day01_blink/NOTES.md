# Day 1: GPIO Output - Blink LED_B trên Cortex-M4

## Mục tiêu
Viết firmware chạy trên nhân Cortex-M4 của STM32MP157D-DK1, dùng HAL_GPIO_TogglePin
để nháy LED_B (PD11) mỗi 500ms, nạp và chạy thử trên board thật.

## Việc đã làm
- Cài STM32CubeIDE 2.2.0 + STM32CubeMX standalone trên Windows.
- Tạo project `day01_blink` cho board STM32MP157D-DK1 (project con CA7 + CM4).
- Sửa vòng lặp `while(1)` trong `CM4/Core/Src/main.c`:
  ```c
  HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
  HAL_Delay(500);
  ```
- Build project CM4: 0 errors, 0 warnings.
- Nạp firmware lên M4 qua **remoteproc từ Linux** (SSH vào board, copy `.elf` vào
  `/lib/firmware/rproc-m4-fw`, `echo start > /sys/class/remoteproc/remoteproc0/state`).
  LED nháy đúng nhịp 500ms — **xác nhận chạy code thật, có kiểm chứng qua log kernel**.

## Nguyên nhân gốc mất nhiều thời gian nhất (bài học quan trọng)
**STM32MP157D-DK1 không tự boot Linux từ eMMC — nó boot demo mặc định từ thẻ microSD
đi kèm trong hộp**, với switch boot SW1 phải để **cả 2 ON ("11")**. Ban đầu không cắm
thẻ SD (không biết là cần thẻ này), nên board "im lặng" trên MỌI kênh kiểm tra (HDMI,
serial console, mạng, cả SWD) — trông y hệt board bị hỏng, dẫn tới một chuỗi dài đi
tìm nguyên nhân sai (nghi bug CubeIDE, nghi Debug Authentication khóa chip, nghi board
DOA...). Tất cả các phát hiện phụ đó đều có thật (xem mục dưới) nhưng **không phải là
nguyên nhân chặn đường chính** — chỉ cần cắm thẻ SD + đúng switch là mọi thứ hoạt động.

**Bài học**: với board STM32MP1 Discovery Kit, luôn kiểm tra thẻ SD đi kèm trong hộp
và cắm vào TRƯỚC khi debug bất kỳ vấn đề kết nối nào.

## Các vấn đề phụ gặp phải (đều có thật, không phải nguyên nhân chính)
1. **STM32CubeIDE Debug As bị treo (Not Responding)** khi debug project CM4 của board MP1.
   Dùng jdb lấy thread dump xác nhận: hàm `MPUWorkbenchWindowControlContribution.doSyncStart()`
   trong plugin của ST gọi `Thread.sleep()` ngay trên UI thread (qua `syncExec` tự gọi
   từ chính UI thread) để chờ "trạng thái kết nối board" — không có timeout. Khi board
   chưa kết nối được (do chưa có thẻ SD ở trên), điều kiện chờ không bao giờ thỏa mãn
   nên treo vô thời hạn. Đây là bug thật trong code của ST, không phải do cấu hình sai.
2. **Board tưởng như mất điện** — ST-LINK báo `Voltage: 0.00V` một lần do quên cắm nguồn
   USB-C chính (cổng ST-LINK micro-USB không đủ nuôi cả board).
3. **ST-LINK bị kẹt ở chế độ DFU (update mode)** sau khi mở tool STLinkUpgrade rồi tắt
   đột ngột — khắc phục bằng cách rút/cắm lại cáp ST-LINK.
4. **STM32_Programmer_CLI không dò được core qua SWD** — vì Cortex-A7/Linux giữ quyền
   kiểm soát reset của M4 ở chế độ Production (bình thường, không phải lỗi).

## Giải thích cơ chế
- STM32MP157 là chip đa nhân (Cortex-A7 x2 chạy Linux + Cortex-M4 đồng xử lý).
- Ở Production mode (mặc định), Cortex-A7 boot Linux, và chính **Linux dùng cơ chế
  remoteproc để nạp/khởi động firmware cho M4** — không cần SWD/debugger ngoài:
  ```
  scp firmware.elf root@<ip_board>:/lib/firmware/rproc-m4-fw
  ssh root@<ip_board> "echo start > /sys/class/remoteproc/remoteproc0/state"
  ```
- Board tự tạo mạng ảo qua USB (USB NCM gadget) khi Linux đã boot xong, PC nhận IP
  dạng `192.168.7.x`, board ở `192.168.7.1`, SSH vào bằng `root` không cần mật khẩu.

## Kết quả
Firmware Day 1 chạy đúng trên board thật qua remoteproc, xác nhận bằng log kernel
Linux thật (không phải suy đoán). Quy trình remoteproc qua SSH là cách nạp code cho
M4 nhanh và đáng tin cậy nhất hiện tại — không cần dùng tới Debug As của CubeIDE (vẫn
còn bug UI chưa khắc phục, nhưng không còn là đường duy nhất để nạp code nữa).

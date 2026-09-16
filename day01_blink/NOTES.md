# Day 1: GPIO Output — Blink LED_B

## Mục tiêu
Cấu hình GPIO output trên STM32MP157D-DK1 (lõi Cortex-M4), điều khiển LED_B (PD11)
nháy mỗi 500ms bằng HAL, nạp và chạy thử trên board thật (có kiểm chứng qua log kernel,
không chỉ nhìn đèn nháy suy đoán).

## Vấn đề gặp phải

- **CubeIDE 2.2.0 không có sẵn wizard tạo file `.ioc`/CubeMX cho project Empty Project
  của board MP1** — phải cài STM32CubeMX bản standalone riêng để tạo/generate project.

- **Nguyên nhân chính mất nhiều thời gian nhất**: STM32MP157D-DK1 không tự boot Linux
  từ eMMC — nó boot demo mặc định từ **thẻ microSD đi kèm trong hộp**, với switch boot
  SW1 phải để **cả 2 ON ("11")**. Ban đầu không cắm thẻ SD, nên board "im lặng" trên
  MỌI kênh kiểm tra (HDMI, serial console, mạng, cả SWD) — trông y hệt board bị hỏng.
  Bài học: luôn kiểm tra thẻ SD đi kèm hộp và cắm vào TRƯỚC khi debug bất kỳ vấn đề
  kết nối nào với board MP1.

- **Debug qua SWD (STM32CubeIDE Debug As / STM32_Programmer_CLI) không kết nối được**
  ở cả 2 chế độ boot:
  - Production mode (SD-Card, Linux chạy): `Unable to get core ID` — vì Cortex-A7/Linux
    giữ quyền kiểm soát reset của M4, đúng thiết kế, không phải lỗi.
  - Engineering mode (BOOT0=OFF, BOOT2=OFF): dò được access port của M4 (AP1) nhưng bị
    chặn bởi **Debug Authentication** (`Cannot connect to access port 1 ... device which
    supports Debug Authentication with certificate or password`) — tính năng bảo mật
    thật của chip, chưa có cách vượt qua. **CHƯA GIẢI QUYẾT, xem TODO.**

- **STM32CubeIDE bị treo (Not Responding) khi bấm Debug As** — không liên quan gì tới
  J-Link (board này không dùng J-Link). Lấy thread dump thật bằng `jdb` (JDWP) xác nhận
  nguyên nhân: hàm `MPUWorkbenchWindowControlContribution.doSyncStart()` trong plugin
  của ST gọi `Thread.sleep()` ngay trên UI thread (qua `syncExec` tự gọi từ chính UI
  thread) để chờ "trạng thái kết nối board" qua SWD — không có timeout. Vì SWD không
  bao giờ kết nối được (xem mục trên), điều kiện chờ không bao giờ thỏa mãn nên treo
  vô thời hạn. Đây là bug thật trong code của ST, đã xác minh bằng bằng chứng kỹ thuật,
  không phải do cấu hình sai.

- **Chuyển hướng**: vì SWD không dùng được, nạp firmware M4 qua **remoteproc/SSH** thay
  vì debug trực tiếp — board tự tạo mạng ảo qua USB (USB NCM gadget) khi Linux đã boot
  xong, PC nhận IP dạng `192.168.7.x`, board ở `192.168.7.1`, SSH bằng `root` không cần
  mật khẩu.

## Giải thích cơ chế

- STM32MP157 là chip 2 lõi (Cortex-A7 x2 chạy Linux + Cortex-M4 bare-metal). Cả 2 lõi
  chia sẻ cùng silicon nhưng M4 chạy code hoàn toàn tách biệt (RAM riêng, không đụng
  tới GPIO của A7 trong bài này).
- M4 không có flash riêng — code chạy trong RETRAM/SRAM, nên **mất nguồn là mất code**,
  phải nạp lại mỗi lần khởi động lại (khác hẳn MCU thường có flash nội bộ).
- Quy trình production để nạp code cho M4 (không cần debugger ngoài):
  ```bash
  scp day01_blink_CM4.elf root@192.168.7.1:/lib/firmware/rproc-m4-fw
  ssh root@192.168.7.1 "echo start > /sys/class/remoteproc/remoteproc0/state"
  ```

## TODO
- [ ] Tìm hiểu cách vượt qua Debug Authentication ở Engineering mode để debug SWD trực
      tiếp (đặt breakpoint, step code) — cần certificate/password DA mà hiện chưa có.
      Quay lại vấn đề này sau khi hiểu rõ hơn về TF-A/OP-TEE và cơ chế bảo mật STM32MP1.

## Kết quả
Firmware Day 1 chạy đúng trên board thật qua remoteproc, xác nhận bằng log kernel
Linux thật (không phải suy đoán). Quy trình remoteproc qua SSH là cách nạp code cho
M4 nhanh và đáng tin cậy nhất hiện tại — dùng cho các ngày tiếp theo cho tới khi giải
quyết được vấn đề Debug Authentication.

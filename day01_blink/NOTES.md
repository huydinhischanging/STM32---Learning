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
- Nạp firmware lên board, quan sát LED LD6 nháy ~1 lần/giây — khớp đúng chu kỳ code.

## Vấn đề gặp phải
1. **STM32CubeIDE Debug As bị treo (Not Responding)** khi debug project CM4 của board MP1.
   Nguyên nhân: dialog "Edit Configuration" (wizard riêng của ST cho debug board MPU)
   bị lỗi hiển thị (window được tạo nhưng không set visible), khiến UI thread bị chặn.
   Đã thử: tăng timeout mạng cho JVM (`stm32cubeide.ini`), dùng Run > Debug Configurations
   thay vì Debug As — vẫn tái diễn lỗi tương tự nhiều lần.
2. **Board không có điện** — lúc đầu chỉ cắm cáp ST-LINK (micro-USB, chỉ đủ nuôi mạch debug),
   quên cắm cáp nguồn chính USB-C. ST-LINK báo `Voltage: 0.00V`. Sau khi cắm nguồn USB-C,
   điện áp lên 3.23V bình thường.
3. **ST-LINK bị kẹt ở chế độ DFU (update mode)** sau khi thử mở tool STLinkUpgrade rồi
   tắt đột ngột — khiến cổng COM ảo (Virtual COM Port) biến mất khỏi hệ thống. Khắc phục
   bằng cách rút/cắm lại cáp ST-LINK để thiết bị enum lại đúng chế độ bình thường.
4. **STM32_Programmer_CLI không dò được core qua SWD** (`No STM32 target found`) dù
   nguồn và ST-LINK đều ổn — do board đang chạy Linux ở chế độ boot mặc định (Production
   mode), nhân Cortex-M4 bị Cortex-A7/Linux giữ ở trạng thái reset. Không kịp đổi sang
   Engineering boot mode (switch SW1/BOOT0/BOOT2) vì phát hiện LED đã nháy đúng —
   tức là một trong các lần Debug As trước đó đã nạp (flash) thành công vào M4 trước khi
   UI treo ở bước gắn debugger, nên firmware vẫn chạy dù session debug không mở được.

## Giải thích cơ chế
- STM32MP157 là chip đa nhân (Cortex-A7 x2 chạy Linux + Cortex-M4 đồng xử lý).
  Theo mặc định (Production mode), Cortex-A7 boot Linux trước và giữ quyền kiểm soát
  reset/clock của Cortex-M4 — muốn debug trực tiếp M4 qua SWD cần board ở Engineering
  mode (switch boot vật lý) hoặc phải qua cơ chế remoteproc của Linux để A7 tự nạp
  firmware cho M4.
- Bước "flash" (ghi code vào RAM của M4) và bước "attach debugger" là hai giai đoạn
  tách biệt trong quy trình Debug As của CubeIDE — flash có thể đã thành công dù
  giai đoạn sau (mở GDB session) bị lỗi UI.

## Kết quả
Firmware Day 1 chạy đúng trên board thật (LED LD6 nháy ~1Hz). Debug session sống qua
CubeIDE GUI chưa dùng được do bug UI — để giải quyết triệt để ở ngày sau, cần cân nhắc
chuyển board sang Engineering boot mode hoặc dùng quy trình remoteproc qua Linux console.

# Day 4: GPIO Input (đọc nút nhấn)

## Mục tiêu
Đọc trạng thái một chân GPIO ở chế độ input (nút nhấn USER1 trên board) rồi điều
khiển LED_B (LD8) theo trạng thái đó: nhấn nút thì sáng, thả nút thì tắt. Đây là
bài đầu tiên M4 nhận tín hiệu từ bên ngoài thay vì chỉ xuất tín hiệu ra.

## Việc đã làm
- Tạo project `day04_gpio_input` qua CubeMX (Board Selector, STM32MP157D-DK1).
- Cấu hình 2 chân trong tab Pinout & Configuration → GPIO:
  - **PA14** (nút nhấn `SW-PUSH-TS-02H-Blue` theo board database): Input mode,
    Pull-up, Pin Context Assignment = **Cortex-M4 FW**.
  - **PD11** (LED_B): Output Push Pull, mức ban đầu Low (LED_B active High nên
    khởi động là tắt), No pull, tốc độ Low, Pin Context = Cortex-M4 FW.
- Bấm GENERATE CODE, CubeMX sinh `MX_GPIO_Init()` và các `#define`
  (`PA14_Pin`, `PA14_GPIO_Port`, `LED_B_Pin`, `LED_B_GPIO_Port`) trong `main.h`.
- Tự viết logic trong `USER CODE BEGIN 3` (trong vòng `while(1)`):
  ```c
  if (HAL_GPIO_ReadPin(PA14_GPIO_Port, PA14_Pin) == GPIO_PIN_RESET)
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
  else
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
  ```
- Build trong CubeIDE, nạp qua `scp` + remoteproc như Day 1-3.

## Vấn đề gặp phải

1. **Không biết viết code vào đâu trong `main.c` dài**: giải quyết bằng các
   marker `USER CODE BEGIN/END` do CubeMX chèn sẵn. Code cần chạy lặp mãi đặt ở
   `BEGIN 3` (trong vòng lặp), code chạy 1 lần đặt ở `BEGIN 2`, hàm mới đặt ở
   `BEGIN 4`. Chỉ code nằm trong marker mới được giữ lại khi generate lại.

2. **Gõ sai tên hàm `HAL_GPIO_Readpin`** (chữ p thường) thay vì `HAL_GPIO_ReadPin`.
   C phân biệt hoa thường nên hàm không tồn tại, build lỗi.

3. **Build nhưng chưa lưu file**: code có trong editor nhưng chưa Ctrl+S, nên
   file trên đĩa vẫn trống và bản build là firmware không làm gì. Cách phát hiện:
   so thời gian sửa đổi của `main.c` với file `.elf` (.elf phải mới hơn).

4. **Bấm nhầm nút reset (B2)** thay vì USER: reset cả board, Linux khởi động
   lại và firmware M4 mất, phải chờ ping thông rồi nạp lại.

5. **Nhấn USER1 thì LD8 lẫn một LED khác cùng sáng**: nhiều khả năng do chân
   PA14 vừa là nút, vừa nối chung với một LED xanh lá active low (nhấn nút kéo
   chân xuống thấp thì LED đó sáng theo). Đây là giả thuyết suy từ sổ tay
   UM2637, chưa xác nhận bằng sơ đồ mạch. Sổ tay ghi LED là LD4 còn thực tế
   trên board là LD5, chưa rõ nguyên nhân lệch số hiệu.

6. **USER2 không có phản ứng**: code chỉ đọc PA14 (USER1). Chân của USER2 chưa
   xác định (bảng pinout trong sổ tay bị vỡ chữ khi trích ra).

## Giải thích cơ chế
- **Input + Pull-up**: chân input không có ai điều khiển mức khi nút không nhấn
  sẽ "lơ lửng" và đọc giá trị ngẫu nhiên. Điện trở pull-up bên trong chip kéo
  chân lên mức cao khi thả nút; nhấn nút nối chân xuống GND nên đọc ra mức thấp.
  Vì vậy "nhấn" ứng với `GPIO_PIN_RESET` (mức 0), thả ứng với `GPIO_PIN_SET`.
- **Pin Context Assignment**: chip có 2 nhân (A7 chạy Linux, M4). Mỗi chân phải
  được giao cho đúng một nhân điều khiển, nếu không code M4 không dùng được.
- **Polling**: code liên tục đọc chân trong vòng lặp để biết nút có nhấn không.
  Đơn giản nhưng CPU luôn bận; cách khác là dùng ngắt ngoài (EXTI), để sau.
- **Output push-pull**: chân tự kéo lên và xuống, dùng cho LED. Output level
  ban đầu chọn theo cực tính LED (active High thì Low = tắt).

## TODO
- Xác định chân USER2 (từ sơ đồ mạch) và đọc thử.
- Xác nhận bằng sơ đồ mạch việc PA14 nối chung với LED.
- Thử đọc nút bằng ngắt ngoài (EXTI) thay cho polling.
- Tồn đọng từ trước: Debug Authentication chặn SWD (vẫn nạp bằng remoteproc/SSH),
  RPMsg chưa truyền được dữ liệu (Day 2).

## Kết quả
Nhấn USER1 thì LD8 (LED_B) sáng, thả thì tắt, xác nhận bằng quan sát thật trên
board. USER1 là chân PA14, nhấn ứng với mức thấp.

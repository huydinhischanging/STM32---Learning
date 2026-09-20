# Day 5: Ngắt ngoài (EXTI + NVIC)

## Mục tiêu
Dùng ngắt ngoài để xử lý nút USER1: mỗi lần nhấn thì LED_B (LD8) đổi trạng thái,
vòng `while(1)` để trống (CPU không phải đọc nút liên tục như Day 4), có chống dội
bằng `HAL_GetTick()`.

## Việc đã làm
- Tạo project `day05_exti` qua CubeMX (Board Selector, STM32MP157D-DK1).
- **PD11** (LED_B): Output Push Pull, Low, No pull, Cortex-M4 FW (giống Day 4).
- **PA14** (nút USER1):
  - Ở Pinout view đổi chức năng chân từ `GPIO_Input` sang **GPXTI14** (nếu không
    thì ô GPIO mode chỉ có "Input mode", không có chế độ ngắt).
  - GPIO mode = **External Interrupt Mode with Falling edge trigger detection**,
    Pull-up, nhãn `USER1`, Pin Context = Cortex-M4 FW.
  - Tab **NVIC**: tích Add cho "EXTI line14 interrupt".
- GENERATE CODE. CubeMX sinh: `USER1_Pin` (main.h), `GPIO_MODE_IT_FALLING` và
  `HAL_NVIC_EnableIRQ(EXTI14_IRQn)` (main.c), `EXTI14_IRQHandler` (stm32mp1xx_it.c).
- Tự viết callback trong `USER CODE BEGIN 4`, có chống dội:
  ```c
  void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
    static uint32_t last_press = 0;
    uint32_t current_time = HAL_GetTick();
    if(GPIO_Pin == USER1_Pin)
    {
      if (current_time - last_press >= 100)
      {
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
        last_press = current_time;
      }
    }
  }
  ```
- Build, nạp qua `scp` + remoteproc như các bài trước.

## Vấn đề gặp phải

1. **Không chọn được chế độ ngắt**: dropdown GPIO mode chỉ có "Input mode" vì chân
   vẫn khai báo là `GPIO_Input`. Phải đổi chức năng chân sang `GPXTI14` ở Pinout view.

2. **Mode Rising thay vì Falling**: nút nhấn kéo chân từ cao xuống thấp nên cần bắt
   **cạnh xuống**. Chọn Rising thì ngắt xảy ra lúc thả nút, và callback
   `..._Falling_Callback` không bao giờ được gọi.

3. **Quên bật NVIC**: cấu hình chân xong nhưng chưa tích EXTI14 trong tab NVIC, nên
   `stm32mp1xx_it.c` không có `EXTI14_IRQHandler` và ngắt không tới CPU. Cách phát
   hiện: tìm `EXTI14` trong code, không có thì NVIC chưa bật.

4. **Nhãn chân bị mất khi đổi sang GPXTI14**: ở Day 4 nhãn `PA14 [SW-PUSH...]` do
   board database điền sẵn. Đổi chức năng chân thì nhãn trống, `PA14_Pin` không được
   sinh. Đặt nhãn `USER1` để có `USER1_Pin`.

5. **Gõ sai tên callback**: `HAL_GPIO_EXIT_Callback` (EXIT thay vì EXTI, thiếu
   `Falling`). HAL không có hàm tên đó nên không bao giờ được gọi, và compiler không
   báo lỗi vì đây chỉ là hàm mới. Ngoài ra từng viết dòng
   `uint32_t HAL_GetTick(void);` trong hàm: đó là **khai báo**, không phải **gọi**
   (gọi là `HAL_GetTick()`).

6. **Build chưa cập nhật**: nhiều lần `.elf` cũ hơn `main.c` (chưa lưu hoặc chưa build
   lại), nạp lên chỉ là bản cũ. Cách kiểm tra: so thời gian sửa đổi của `main.c` và `.elf`.

7. **LED_B nháy đều bất kể nhấn nút**: nguyên nhân là **Linux (nhân A7) đang điều
   khiển PD11** làm đèn `heartbeat`. Kiểm tra trên board:
   `/sys/class/leds/heartbeat` có trigger `[heartbeat]`, và
   `/sys/kernel/debug/gpio` ghi `gpio-571 (PD11 |heartbeat) out lo`. Linux liên tục
   nháy chân này theo nhịp tim, ghi đè lên thứ M4 đặt.
   Đây rất có thể cũng là nguyên nhân của hiện tượng LD8 nháy dù code Day 3 đặt sáng
   cố định (Day 3 lúc đó ghi "chưa tìm ra nguyên nhân").

8. **LD5 sáng theo khi nhấn USER1**: hiện tượng phần cứng như Day 4 (chân PA14 dùng
   chung nút và một LED), chưa xác nhận bằng sơ đồ mạch.

## Giải thích cơ chế
- **Chuỗi gọi khi nhấn nút**: cạnh xuống trên PA14 → phần cứng EXTI phát hiện, báo
  NVIC → CPU nhảy vào `EXTI14_IRQHandler` (stm32mp1xx_it.c) → gọi
  `HAL_GPIO_EXTI_IRQHandler(USER1_Pin)` (HAL của ST: kiểm tra và xóa cờ) → gọi
  `HAL_GPIO_EXTI_Falling_Callback(GPIO_Pin)` (hàm của mình).
- **Handler riêng cho từng đường ngắt, callback chung**: mỗi số chân dùng đường EXTI
  cùng số (chân 14 dùng EXTI14, tên handler cố định `EXTI14_IRQHandler`). Callback
  chỉ có một, dùng chung cho mọi đường, nên phải kiểm tra `GPIO_Pin` để biết chân nào.
- **`__weak`**: HAL có sẵn bản rỗng của callback đánh dấu `__weak`. Viết hàm cùng tên
  trong `main.c` thì hàm của mình thay thế bản rỗng, không cần đăng ký gì thêm.
- **Ai tạo handler**: CubeMX sinh `EXTI14_IRQHandler` khi bật EXTI14 trong NVIC (trước
  đó không có). Bảng vector ngắt trong file startup thì đã có sẵn tên hàm này.
- **Chống dội**: nút cơ khí nảy vài ms nên một lần nhấn tạo nhiều ngắt. Bỏ qua ngắt
  nào đến trong vòng 100 ms kể từ lần được chấp nhận trước (`HAL_GetTick()` là số ms
  từ lúc khởi động). `static` giữ giá trị `last_press` giữa các lần gọi.
- **Ngắt so với polling**: Day 4 CPU phải đọc nút liên tục trong `while(1)`; Day 5 để
  phần cứng báo, `while(1)` trống.

## TODO
- Xác nhận lại bằng cách tắt heartbeat của Linux
  (`echo none > /sys/class/leds/heartbeat/trigger`) rồi nhấn nút, LED_B phải đổi
  đúng theo nút.
- Sửa lại kết luận trong NOTES Day 3 (LD8 nháy) theo phát hiện heartbeat.
- Xác nhận bằng sơ đồ mạch việc PA14 nối chung với LED, và chân USER2.
- Tồn đọng: Debug Authentication chặn SWD, RPMsg chưa truyền được dữ liệu (Day 2).

## Kết quả
Nhấn USER1 kích hoạt ngắt EXTI14 và callback được gọi (LD5 sáng theo nút là hiện tượng
phần cứng). LED_B bị đèn heartbeat của Linux ghi đè nên chưa quan sát được rõ hiệu ứng
đảo trạng thái; cần tắt trigger heartbeat để xác nhận. Video demo trong `demo/`.

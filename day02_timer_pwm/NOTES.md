# Day 2: Timer & PWM

## Mục tiêu
Dùng Timer phần cứng (TIM) để tạo tín hiệu PWM, làm LED rời sáng dần/mờ dần
("breathing effect") — hiểu cách Timer đếm và tạo xung tự động, không cần CPU
liên tục toggle như GPIO thường.

## Việc đã làm
- Tạo project `day02_timer_pwm` mới qua CubeMX (Board Selector → STM32MP157D-DK1).
- Ban đầu cấu hình PWM ra chân PB14 (TIM12_CH1) — nhưng chân này **không có LED
  gắn sẵn trên board**, không thấy được kết quả bằng mắt.
- Chuyển hướng: gắn LED rời qua breadboard vào chân **D10 (PE11) trên connector
  CN13** — chân này hỗ trợ **TIM1_CH2** thật (tra từ database chip
  `STM32MP157DACx.xml` và board `M10_..._Board_AllConfig.ioc` trong CubeMX).
- Viết lại tay code Timer (main.c + stm32mp1xx_hal_msp.c) chuyển từ TIM12/PB14
  sang **TIM1/PE11 (AF1, GPIO_AF1_TIM1)** — không qua lại CubeMX GUI, chỉnh trực
  tiếp code theo đúng thông số tra được từ database.
- Kết quả cuối: **LED rời sáng dần/mờ dần thật trên breadboard**, đúng hiệu ứng
  PWM mong muốn. Có quay video demo (`demo/`).

## Vấn đề gặp phải

1. **Chân PWM_LED (PB14) là tên tự đặt, không có LED thật** — nhãn "PWM_LED" là
   do tự gán trong CubeMX lúc cấu hình, không phải tên board chính thức của ST.
   Board DK1 chỉ có đúng 1 LED lộ ra cho M4 là LED_B (PD11/LD8), và PD11 tra ra
   **không hỗ trợ bất kỳ kênh TIM nào cả** (chỉ có ADC, I2C, FMC, SAI, USART,
   GPIO — xem file `STM32MP157DACx.xml`) — nên không thể PWM trực tiếp trên LED
   có sẵn của board, bắt buộc phải dùng LED rời qua breadboard.

2. **Mạch LED sai nhiều lớp liên tiếp**:
   - Điện trở đầu tiên dùng 10kΩ (nâu-đen-cam) — quá lớn, dòng qua LED chỉ
     ~0.1mA ở 3.3V, không đủ sáng. Đổi sang 220-330Ω mới đủ dòng.
   - Nhầm lẫn giữa chân tín hiệu **D10** (GPIO do code điều khiển, đổi theo PWM)
     và chân nguồn cố định **3V3/5V** (điện áp không đổi, không liên quan code)
     — đo nhầm/nối nhầm gây tưởng lầm mạch hỏng.
   - **LED xanh dương/xanh lá "cao cấp" có Vf ~3-3.4V** không sáng rõ ở 3.3V
     (điện áp GPIO tối đa của STM32) — cần đổi sang LED đỏ/vàng thường (Vf thấp
     ~1.8-2.2V) mới chắc chắn đủ sáng.

3. **Thêm code RPMsg (gửi debug text qua `/dev/ttyRPMSG0`) rồi phải bỏ lại**:
   dùng `VIRT_UART_Init()` + `OPENAMP_Wait_EndPointready()` (driver có sẵn
   `virt_uart.c` của ST cho kênh OpenAMP) để thử debug không cần nhìn LED. Kênh
   tạo được thành công (dmesg xác nhận `creating channel rpmsg-tty`,
   `/dev/ttyRPMSG0` xuất hiện), nhưng **không nhận được dữ liệu gửi từ M4** dù
   `VIRT_UART_Transmit` không báo lỗi — chưa rõ nguyên nhân (có thể do timing,
   do cách đọc phía Linux, hoặc thiếu bước nào đó). Quan trọng hơn:
   `OPENAMP_Wait_EndPointready()` là **vòng lặp chờ vô thời hạn, không timeout**
   — nếu điều kiện không thỏa mãn, cả firmware treo cứng ngay từ đầu, không
   chạy tới được phần code chính (GPIO/Timer) nữa. Đã **gỡ bỏ hoàn toàn** đoạn
   RPMsg này khỏi code hiện tại để đảm bảo độ tin cậy — để làm lại sau khi có
   thời gian tìm hiểu kỹ hơn về cơ chế RPMsg.

## Giải thích cơ chế

- **Timer PWM**: thanh ghi Compare (CCR) của Timer so sánh liên tục với bộ đếm
  (Counter) chạy tự động theo xung clock — khi Counter < CCR thì chân ra HIGH,
  ngược lại LOW — tạo xung vuông có "độ rộng" (duty cycle) tỷ lệ với giá trị
  CCR, hoàn toàn tự động ở tầng phần cứng, CPU chỉ cần ghi giá trị CCR mới
  (`__HAL_TIM_SET_COMPARE`) mỗi lần muốn đổi độ sáng, không cần toggle tay.
- **Mỗi chân GPIO chỉ nối được với một số kênh Timer cố định** (do thiết kế
  silicon, không đổi được bằng phần mềm) — tra trong file `<chip>.xml` của
  CubeMX database (`db/mcu/STM32MP157DACx.xml`) để biết chân nào hỗ trợ Timer
  nào, và file `db/mcu/IP/GPIO-STM32MPU_gpio_v1_0_Modes.xml` để biết đúng số
  hiệu Alternate Function (AF) cần cấu hình.
- **Mạch LED cần đúng 3 yếu tố**: điện áp nguồn đủ lớn hơn Vf của LED, điện trở
  đủ nhỏ để có dòng nhìn thấy được (thường 220-330Ω cho LED chỉ báo hiệu), và
  đúng chiều LED (chân dài về phía dương).

## TODO
- [ ] Tìm hiểu lại vì sao RPMsg tạo kênh được nhưng không truyền được dữ liệu
      thật — có thể liên quan tới việc buffer/timing giữa lúc gửi và lúc Linux
      mở `/dev/ttyRPMSG0` để đọc.
- [ ] Debug Authentication (tồn đọng từ Day 1) vẫn chưa giải quyết — vẫn dùng
      remoteproc/SSH để nạp code thay vì debug SWD trực tiếp.

## Kết quả
LED rời gắn qua breadboard vào chân D10 (PE11/TIM1_CH2) sáng dần/mờ dần đúng
hiệu ứng PWM — xác nhận bằng mắt thật qua video demo, không phải suy đoán.

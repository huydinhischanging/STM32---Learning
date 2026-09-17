# Day 3: UART + printf

## Mục tiêu
Cho firmware chạy trên nhân M4 gửi được text debug (qua `printf`) ra một cổng
UART vật lý thật, đọc được trên PC qua module USB-to-TTL — thay vì chỉ suy đoán
trạng thái chương trình qua đèn LED nháy.

## Việc đã làm
- Mua module USB-to-TTL CH340G, nối vào connector Arduino **CN14** của board:
  D0 (RX board) ↔ TXD module, D1 (TX board) ↔ RXD module, GND ↔ GND.
- Tạo project `day03_uart_printf` qua CubeMX, ban đầu định dùng USART2 mặc định
  nhưng phát hiện chân mặc định (PD5/PD6) **không được đưa ra bất kỳ connector
  nào** trên board (tra trong file `M10_..._Board_AllConfig.ioc`, không có
  `GPIO_Label` cho 2 chân này) — đổi hướng sang **UART7**, vì theo tài liệu
  chính thức UM2637 (bảng Arduino connector), D0 = PE7 = `USART7_RX`,
  D1 = PE8 = `USART7_TX`.
- Cấu hình lại UART7 trong CubeMX, viết `int __io_putchar(int ch)` gọi
  `HAL_UART_Transmit(&huart7, ...)` để `printf()` chuẩn C tự động được định
  tuyến ra UART7 (dựa trên cơ chế `_write()` có sẵn trong `syscalls.c` của
  CubeIDE, gọi `__io_putchar` cho từng ký tự).
- Build, nạp qua remoteproc/SSH (`scp` + ghi `firmware`/`state` trong
  `/sys/class/remoteproc/remoteproc0/`), test bằng cách đọc cổng COM (CH340)
  trên PC bằng PowerShell `System.IO.Ports.SerialPort`.
- Sau rất nhiều lần không thấy gì trên COM, tìm ra nguyên nhân gốc (xem mục
  Vấn đề), sửa lại đúng chân trong CubeMX, build lại — **nhận được dữ liệu
  UART thật, đếm số tăng dần liên tục trên PC**.
- Thêm `HAL_Delay(500)` vào cuối vòng lặp để dòng in chậm lại, dễ đọc/quay demo
  (ban đầu để không delay, in liên tục tối đa, nhằm dễ bắt tín hiệu bằng
  multimeter khi đang debug).

## Vấn đề gặp phải (đây là phần tốn thời gian nhất trong 3 ngày)

1. **USART2 mặc định (PD5/PD6) không dùng được** — tra ra 2 chân này không có
   nhãn `GPIO_Label` trong board database, nghĩa là không được nối ra bất kỳ
   connector vật lý nào của DK1 (dead end). Phải đổi sang UART7/PE7-PE8 vì đó
   là cặp chân UART duy nhất được xác nhận đưa ra Arduino header (D0/D1).

2. **Nhầm connector D0/D1**: ban đầu cắm module vào **CN13**, nhưng theo tài
   liệu chính thức UM2637, D0-D7 nằm ở **CN14**, còn CN13 là D8-D15. Phải tải
   bằng được bản PDF gốc (link ST bị timeout liên tục, phải đổi sang mirror) để
   đối chiếu đúng bảng pinout, vì một AI khác đưa thông tin sai.

3. **LED_B (LD8) nháy dù code set `GPIO_PIN_SET` cố định — dùng làm tín hiệu
   chẩn đoán nhưng gây nhiễu**: nghi ngờ firmware cũ còn sót (loại bỏ bằng
   cách so khớp MD5 file .elf), nghi crash-loop (loại bỏ bằng dmesg timestamp
   cho thấy chỉ boot 1 lần), nghi watchdog (loại bỏ vì không cấu hình
   IWDG2/WWDG). **Không tìm ra nguyên nhân**, cuối cùng bỏ qua tín hiệu LED này
   hoàn toàn, chuyển hẳn sang xác minh trực tiếp bằng UART — đây là bài học
   quan trọng: **đèn LED không phải lúc nào cũng là kênh debug đáng tin cậy**,
   nhất là khi bản thân cơ chế điều khiển nó chưa được xác minh độc lập.

4. **Đo bằng đồng hồ vạn năng (multimeter) trên D0/D1 luôn ra 3.58V cố định,
   bất kể firmware có chạy hay không** — tưởng là mạch lỗi, nhưng thực chất do
   hai lý do: (a) bản thân đồng hồ có sai số hiệu chuẩn ~+0.25V (xác nhận bằng
   cách đo luôn chân 3.3V có nhãn sẵn trên module, cũng ra 3.58V); (b) đồng hồ
   đo điện áp DC trung bình, **không thể bắt được xung UART tồn tại chỉ vài ms
   với duty cycle thấp** — kết luận: multimeter hoàn toàn không phải công cụ
   phù hợp để xác minh tín hiệu UART, phải đọc bằng phần mềm serial thật.

5. **Nguyên nhân gốc thật sự, tìm ra cuối cùng**: mở file
   `stm32mp1xx_hal_msp.c` (do CubeMX sinh ra) thì phát hiện UART7 thực tế đang
   được cấu hình dùng **PB3 (RX) / PB4 (TX)**, KHÔNG PHẢI PE7/PE8 (D0/D1) như
   vẫn tưởng — nghĩa là toàn bộ thời gian trước đó, firmware chưa từng dùng
   đúng 2 chân đã nối dây ra module CH340. Đây là do lúc cấu hình UART7 trong
   CubeMX, chỉ chọn "Mode: Asynchronous" mà không tự kiểm tra lại chân RX/TX
   mặc định được CubeMX gán (CubeMX có thể tự chọn chân AF khác nếu chân mặc
   định đang bị chiếm bởi tính năng khác).
   - Sửa: vào lại CubeMX, click trực tiếp vào từng chân PE7/PE8 trên Pinout
     view, gán thủ công tín hiệu `UART7_RX`/`UART7_TX`.
   - Gặp thêm một lớp lỗi nữa: chỉ gán tín hiệu ở mức chân (Pinout view) mà
     **chưa bật peripheral UART7 ở mức Mode (Connectivity → UART7 → Mode =
     Asynchronous)**, nên `GENERATE CODE` chỉ sinh phần cấu hình GPIO AF cho
     2 chân, còn thiếu hẳn `UART_HandleTypeDef huart7`, hàm `MX_UART7_Init()`
     và lệnh gọi nó trong `main()` — build báo lỗi `huart7` chưa khai báo.
     Sửa bằng cách vào Connectivity → UART7, đổi Mode từ Disable sang
     Asynchronous, xác nhận lại RX/TX vẫn là PE7/PE8, generate code lại.

## Giải thích cơ chế

- **printf redirect trên bare-metal (không OS)**: hàm `printf` chuẩn thư viện
  C cuối cùng gọi `_write()` (syscall giả lập, có sẵn trong `syscalls.c` do
  CubeIDE sinh ra) — hàm này lặp qua từng ký tự gọi `__io_putchar()`. Chỉ cần
  tự định nghĩa `__io_putchar()` để đẩy 1 byte ra UART bằng
  `HAL_UART_Transmit()` là toàn bộ `printf()` tự động hoạt động qua UART, mà
  không cần viết lại driver UART từ đầu.
- **CubeMX sinh code theo 2 lớp tách biệt**: (1) lớp *Pinout* — chỉ định
  nghĩa chân nào nối vào chức năng AF nào (ảnh hưởng `MX_GPIO_Init`), và (2)
  lớp *Mode/Configuration* của từng peripheral (ảnh hưởng việc có sinh
  `MX_<peripheral>_Init()` và handle tương ứng hay không). Gán đúng chân ở
  Pinout view mà quên bật Mode ở Configuration panel là lỗi dễ mắc, vì CubeMX
  không báo lỗi ngay — chỉ lộ ra khi build thiếu symbol.
- **UART cần đối xứng dây chéo**: chân TX của board phải nối vào chân RX của
  module CH340 và ngược lại — vì đây là truyền tín hiệu một chiều (TX chỉ
  phát, RX chỉ nhận).

## TODO
- Tồn đọng từ Day 1: Debug Authentication vẫn chặn SWD, tiếp tục dùng
  remoteproc/SSH để nạp code.
- Tồn đọng từ Day 2: chưa hiểu vì sao kênh RPMsg tạo được nhưng không truyền
  được dữ liệu.
- Tìm hiểu tiếp vì sao LED_B nháy dù code set cố định (Day 3) — có thể liên
  quan đến việc chưa hiểu rõ cơ chế Pin Context Assignment ảnh hưởng lên GPIO
  chia sẻ giữa 2 nhân.

## Kết quả
Đọc được text debug thật qua UART7 (PE7/PE8 = D0/D1) trên PC, qua module
CH340, hiển thị đúng chuỗi khởi động và số đếm tăng dần liên tục — xác nhận
bằng dữ liệu thật nhận trên cổng COM, không còn phải suy đoán qua đèn LED hay
đo vôn kế nữa. Ảnh/video demo: xem thư mục `demo/`.

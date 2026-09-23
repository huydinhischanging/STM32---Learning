#ifndef UART_PRINTF_H
#define UART_PRINTF_H

#include "main.h"

/* Gọi 1 lần lúc khởi động: tắt buffer của printf và ghi nhớ UART handle
   sẽ dùng để gửi ký tự đi. */
void UART_Printf_Init(UART_HandleTypeDef *huart);

#endif /* UART_PRINTF_H */

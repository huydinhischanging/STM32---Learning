#include "uart_printf.h"
#include <stdio.h>

/* printf() chỉ gọi được __io_putchar(int ch), không truyền huart vào được,
   nên phải giữ lại UART handle ở đây (biến static: chỉ file này thấy được). */
static UART_HandleTypeDef *s_huart = NULL;

void UART_Printf_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  setvbuf(stdout, NULL, _IONBF, 0);
}

int __io_putchar(int ch)
{
  if (s_huart != NULL)
  {
    HAL_UART_Transmit(s_huart, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  }
  return ch;
}

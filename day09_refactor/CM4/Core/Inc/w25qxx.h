#ifndef W25QXX_H
#define W25QXX_H

#include "main.h"

/* Gói mọi thông tin cần thiết để nói chuyện với 1 chip W25Qxx cụ thể:
   dùng SPI nào, chân CS nào. Nhờ vậy các hàm bên dưới không cần biết trước
   "hspi4" hay "FLASH_CS_Pin" là gì -- có thể dùng lại cho chip flash khác,
   trên chân CS khác, chỉ cần đổi giá trị trong struct này. */
typedef struct
{
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef      *cs_port;
  uint16_t           cs_pin;
} W25QXX_HandleTypeDef;

void     W25QXX_Init(W25QXX_HandleTypeDef *dev, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin);
void     W25QXX_ReadJEDEC(W25QXX_HandleTypeDef *dev, uint8_t *manufacturer,
                           uint8_t *devid_hi, uint8_t *devid_lo);
void     W25QXX_SectorErase(W25QXX_HandleTypeDef *dev, uint32_t addr);
void     W25QXX_PageProgram(W25QXX_HandleTypeDef *dev, uint32_t addr,
                             uint8_t *data, uint16_t len);
void     W25QXX_Read(W25QXX_HandleTypeDef *dev, uint32_t addr,
                      uint8_t *buf, uint16_t len);

#endif /* W25QXX_H */

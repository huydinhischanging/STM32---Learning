#include "w25qxx.h"

#define CMD_WRITE_ENABLE   0x06
#define CMD_READ_STATUS    0x05
#define CMD_SECTOR_ERASE   0x20
#define CMD_PAGE_PROGRAM   0x02
#define CMD_READ_DATA      0x03
#define CMD_JEDEC_ID       0x9F

static void CS_Low(W25QXX_HandleTypeDef *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void CS_High(W25QXX_HandleTypeDef *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static void WriteEnable(W25QXX_HandleTypeDef *dev)
{
  uint8_t cmd = CMD_WRITE_ENABLE;
  CS_Low(dev);
  HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
  CS_High(dev);
}

static uint8_t ReadStatus(W25QXX_HandleTypeDef *dev)
{
  uint8_t cmd = CMD_READ_STATUS;
  uint8_t status;
  CS_Low(dev);
  HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
  HAL_SPI_Receive(dev->hspi, &status, 1, 100);
  CS_High(dev);
  return status;
}

static void WaitBusy(W25QXX_HandleTypeDef *dev)
{
  while (ReadStatus(dev) & 0x01)  // bit 0 = Write In Progress
  {
    HAL_Delay(1);
  }
}

void W25QXX_Init(W25QXX_HandleTypeDef *dev, SPI_HandleTypeDef *hspi,
                  GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
  dev->hspi = hspi;
  dev->cs_port = cs_port;
  dev->cs_pin = cs_pin;
  CS_High(dev);  // CS rảnh = mức cao
}

void W25QXX_ReadJEDEC(W25QXX_HandleTypeDef *dev, uint8_t *manufacturer,
                       uint8_t *devid_hi, uint8_t *devid_lo)
{
  uint8_t tx[4] = {CMD_JEDEC_ID, 0x00, 0x00, 0x00};
  uint8_t rx[4] = {0};

  CS_Low(dev);
  HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 4, 100);
  CS_High(dev);

  *manufacturer = rx[1];
  *devid_hi = rx[2];
  *devid_lo = rx[3];
}

void W25QXX_SectorErase(W25QXX_HandleTypeDef *dev, uint32_t addr)
{
  WriteEnable(dev);
  uint8_t cmd[4] = {CMD_SECTOR_ERASE, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
  CS_Low(dev);
  HAL_SPI_Transmit(dev->hspi, cmd, 4, 100);
  CS_High(dev);
  WaitBusy(dev);
}

void W25QXX_PageProgram(W25QXX_HandleTypeDef *dev, uint32_t addr,
                         uint8_t *data, uint16_t len)
{
  WriteEnable(dev);
  uint8_t cmd[4] = {CMD_PAGE_PROGRAM, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
  CS_Low(dev);
  HAL_SPI_Transmit(dev->hspi, cmd, 4, 100);
  HAL_SPI_Transmit(dev->hspi, data, len, 100);
  CS_High(dev);
  WaitBusy(dev);
}

void W25QXX_Read(W25QXX_HandleTypeDef *dev, uint32_t addr,
                  uint8_t *buf, uint16_t len)
{
  uint8_t cmd[4] = {CMD_READ_DATA, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
  CS_Low(dev);
  HAL_SPI_Transmit(dev->hspi, cmd, 4, 100);
  HAL_SPI_Receive(dev->hspi, buf, len, 100);
  CS_High(dev);
}

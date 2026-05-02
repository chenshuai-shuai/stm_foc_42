#include "drv_mt6816.h"

#include <stddef.h>

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

#define DRV_MT6816_CMD_READ        0x80U
#define DRV_MT6816_REG_ANGLE_HI    0x03U
#define DRV_MT6816_REG_ANGLE_LO    0x04U
#define DRV_MT6816_FULL_SCALE      16384UL
#define DRV_MT6816_HALF_SCALE      8192

static uint16_t g_last_raw_angle;
static int32_t g_mechanical_turn_count;
static bool g_has_sample;

static void drvMt6816CsHigh(void) {
  DRV_MT6816_CS_GPIO->BSRR = DRV_MT6816_CS_PIN;
}

static void drvMt6816CsLow(void) {
  DRV_MT6816_CS_GPIO->BRR = DRV_MT6816_CS_PIN;
}

static uint8_t drvMt6816TransferByte(uint8_t tx_data) {
  while (SPI_I2S_GetFlagStatus(DRV_MT6816_SPI, SPI_I2S_FLAG_TXE) == RESET) {
  }

  SPI_I2S_SendData(DRV_MT6816_SPI, tx_data);

  while (SPI_I2S_GetFlagStatus(DRV_MT6816_SPI, SPI_I2S_FLAG_RXNE) == RESET) {
  }

  return (uint8_t)SPI_I2S_ReceiveData(DRV_MT6816_SPI);
}

static uint8_t drvMt6816ReadRegister(uint8_t reg_addr) {
  uint8_t reg_value;

  drvMt6816CsLow();
  (void)drvMt6816TransferByte((uint8_t)(DRV_MT6816_CMD_READ | reg_addr));
  reg_value = drvMt6816TransferByte(0x00U);

  while (SPI_I2S_GetFlagStatus(DRV_MT6816_SPI, SPI_I2S_FLAG_BSY) != RESET) {
  }

  drvMt6816CsHigh();

  return reg_value;
}

void drvMt6816Init(void) {
  GPIO_InitTypeDef gpio_init;
  SPI_InitTypeDef spi_init;

  RCC_APB2PeriphClockCmd(DRV_MT6816_CS_GPIO_CLK |
                         DRV_MT6816_SCK_GPIO_CLK |
                         DRV_MT6816_MISO_GPIO_CLK |
                         DRV_MT6816_MOSI_GPIO_CLK |
                         DRV_MT6816_SPI_CLK,
                         ENABLE);

  gpio_init.GPIO_Speed = GPIO_Speed_50MHz;

  gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
  gpio_init.GPIO_Pin = DRV_MT6816_CS_PIN;
  GPIO_Init(DRV_MT6816_CS_GPIO, &gpio_init);

  gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
  gpio_init.GPIO_Pin = DRV_MT6816_SCK_PIN;
  GPIO_Init(DRV_MT6816_SCK_GPIO, &gpio_init);

  gpio_init.GPIO_Pin = DRV_MT6816_MOSI_PIN;
  GPIO_Init(DRV_MT6816_MOSI_GPIO, &gpio_init);

  gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpio_init.GPIO_Pin = DRV_MT6816_MISO_PIN;
  GPIO_Init(DRV_MT6816_MISO_GPIO, &gpio_init);

  drvMt6816CsHigh();

  SPI_I2S_DeInit(DRV_MT6816_SPI);
  SPI_StructInit(&spi_init);
  spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  spi_init.SPI_Mode = SPI_Mode_Master;
  spi_init.SPI_DataSize = SPI_DataSize_8b;
  spi_init.SPI_CPOL = SPI_CPOL_High;
  spi_init.SPI_CPHA = SPI_CPHA_2Edge;
  spi_init.SPI_NSS = SPI_NSS_Soft;
  spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
  spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
  spi_init.SPI_CRCPolynomial = 7U;
  SPI_Init(DRV_MT6816_SPI, &spi_init);
  SPI_Cmd(DRV_MT6816_SPI, ENABLE);

  g_last_raw_angle = 0U;
  g_mechanical_turn_count = 0;
  g_has_sample = false;
}

void drvMt6816GetSample(drv_mt6816_sample_t *sample) {
  int32_t delta;
  uint8_t angle_hi;
  uint8_t angle_lo;
  uint16_t raw_angle;

  if (sample == NULL) {
    return;
  }

  angle_hi = drvMt6816ReadRegister(DRV_MT6816_REG_ANGLE_HI);
  angle_lo = drvMt6816ReadRegister(DRV_MT6816_REG_ANGLE_LO);
  raw_angle = (uint16_t)((((uint16_t)angle_hi << 8) | (uint16_t)angle_lo) >> 2);

  if (g_has_sample) {
    delta = (int32_t)raw_angle - (int32_t)g_last_raw_angle;

    if (delta > DRV_MT6816_HALF_SCALE) {
      g_mechanical_turn_count--;
    } else if (delta < -DRV_MT6816_HALF_SCALE) {
      g_mechanical_turn_count++;
    }
  } else {
    g_has_sample = true;
  }

  g_last_raw_angle = raw_angle;

  sample->raw_angle = raw_angle;
  sample->mechanical_angle_decideg =
      (uint16_t)(((uint32_t)raw_angle * 3600UL) / DRV_MT6816_FULL_SCALE);
  sample->mechanical_turn_count = g_mechanical_turn_count;
  sample->ready = true;
}

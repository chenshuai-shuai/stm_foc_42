#ifndef DRV_MT6816_H
#define DRV_MT6816_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f10x.h"

/*
 * Hardware SPI1 mapping:
 * CS   -> PA4  (manual chip select)
 * SCK  -> PA5  (SPI1_SCK)
 * MISO -> PA6  (SPI1_MISO)
 * MOSI -> PA7  (SPI1_MOSI)
 */
#define DRV_MT6816_CS_GPIO         GPIOA
#define DRV_MT6816_CS_PIN          GPIO_Pin_4
#define DRV_MT6816_CS_GPIO_CLK     RCC_APB2Periph_GPIOA

#define DRV_MT6816_SCK_GPIO        GPIOA
#define DRV_MT6816_SCK_PIN         GPIO_Pin_5
#define DRV_MT6816_SCK_GPIO_CLK    RCC_APB2Periph_GPIOA

#define DRV_MT6816_MISO_GPIO       GPIOA
#define DRV_MT6816_MISO_PIN        GPIO_Pin_6
#define DRV_MT6816_MISO_GPIO_CLK   RCC_APB2Periph_GPIOA

#define DRV_MT6816_MOSI_GPIO       GPIOA
#define DRV_MT6816_MOSI_PIN        GPIO_Pin_7
#define DRV_MT6816_MOSI_GPIO_CLK   RCC_APB2Periph_GPIOA

#define DRV_MT6816_SPI             SPI1
#define DRV_MT6816_SPI_CLK         RCC_APB2Periph_SPI1

typedef struct {
  uint16_t raw_angle;
  uint16_t mechanical_angle_decideg;
  int32_t mechanical_turn_count;
  bool ready;
} drv_mt6816_sample_t;

void drvMt6816Init(void);
void drvMt6816GetSample(drv_mt6816_sample_t *sample);

#endif /* DRV_MT6816_H */

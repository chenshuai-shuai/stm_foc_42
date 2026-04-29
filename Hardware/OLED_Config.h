#ifndef __OLED_CONFIG_H
#define __OLED_CONFIG_H

#include "stm32f10x.h"

/* Screen configuration */
#define OLED_WIDTH                      64
#define OLED_HEIGHT                     32
#define OLED_PAGE_COUNT                 (OLED_HEIGHT / 8)
#define OLED_MAX_X                      (OLED_WIDTH - 1)
#define OLED_MAX_Y                      (OLED_HEIGHT - 1)
#define OLED_COLUMN_OFFSET              32

/* SSD1315 initialization parameters */
#define OLED_CMD_DISPLAY_CLOCK_DIV      0x80
#define OLED_CMD_MULTIPLEX_RATIO        0x1F
#define OLED_CMD_DISPLAY_OFFSET         0x00
#define OLED_CMD_DISPLAY_START_LINE     0x40
#define OLED_CMD_SEG_REMAP              0xA1
#define OLED_CMD_COM_SCAN_DIR           0xC8
#define OLED_CMD_COM_PINS_CONFIG        0x12
#define OLED_CMD_CONTRAST               0xCF
#define OLED_CMD_PRECHARGE_PERIOD       0xF1
#define OLED_CMD_VCOMH_LEVEL            0x30
#define OLED_CMD_CHARGE_PUMP            0x14

/* Software I2C pin mapping */
#define OLED_I2C_GPIO_CLK               RCC_APB2Periph_GPIOB
#define OLED_I2C_GPIO_PORT              GPIOB
#define OLED_I2C_SCL_PIN                GPIO_Pin_6
#define OLED_I2C_SDA_PIN                GPIO_Pin_7
#define OLED_I2C_ADDRESS                0x78

#endif

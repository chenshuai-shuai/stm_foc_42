#include "drv_oled_u8g2_port.h"

#include "OLED_Config.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

static void drvOledU8g2WritePin(uint16_t pin, uint8_t high_level) {

  if (high_level != 0U) {
    OLED_I2C_GPIO_PORT->BSRR = pin;
  } else {
    OLED_I2C_GPIO_PORT->BRR = pin;
  }
}

static void drvOledU8g2BusyWait(volatile uint32_t cycles) {

  while (cycles-- > 0U) {
    __NOP();
  }
}

static void drvOledU8g2DelayUs(uint32_t microseconds) {

  const uint32_t cycles_per_us = (SystemCoreClock / 3000000U) + 1U;

  while (microseconds-- > 0U) {
    drvOledU8g2BusyWait(cycles_per_us);
  }
}

static void drvOledU8g2PortInit(void) {

  GPIO_InitTypeDef gpio_init;

  RCC_APB2PeriphClockCmd(OLED_I2C_GPIO_CLK, ENABLE);

  gpio_init.GPIO_Pin = OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN;
  gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
  gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(OLED_I2C_GPIO_PORT, &gpio_init);

  drvOledU8g2WritePin(OLED_I2C_SCL_PIN, 1U);
  drvOledU8g2WritePin(OLED_I2C_SDA_PIN, 1U);
}

uint8_t drvOledU8g2GpioAndDelay(u8x8_t *u8x8,
                                uint8_t msg,
                                uint8_t arg_int,
                                void *arg_ptr) {

  (void)u8x8;
  (void)arg_ptr;

  switch (msg) {
  case U8X8_MSG_GPIO_AND_DELAY_INIT:
    drvOledU8g2PortInit();
    break;

  case U8X8_MSG_DELAY_MILLI:
    while (arg_int-- > 0U) {
      drvOledU8g2DelayUs(1000U);
    }
    break;

  case U8X8_MSG_DELAY_10MICRO:
    while (arg_int-- > 0U) {
      drvOledU8g2DelayUs(10U);
    }
    break;

  case U8X8_MSG_DELAY_100NANO:
  case U8X8_MSG_DELAY_NANO:
    drvOledU8g2BusyWait(2U);
    break;

  case U8X8_MSG_DELAY_I2C:
    drvOledU8g2DelayUs(5U);
    break;

  case U8X8_MSG_GPIO_I2C_CLOCK:
    drvOledU8g2WritePin(OLED_I2C_SCL_PIN, arg_int);
    break;

  case U8X8_MSG_GPIO_I2C_DATA:
    drvOledU8g2WritePin(OLED_I2C_SDA_PIN, arg_int);
    break;

  default:
    break;
  }

  return 1U;
}

#include "drv_usart1.h"

#include <stddef.h>

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

static unsigned g_usart1_write_count;
static bool g_usart1_ready;

void drvUsart1Init(void) {

  GPIO_InitTypeDef gpio_init;
  USART_InitTypeDef usart_init;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

  gpio_init.GPIO_Pin = GPIO_Pin_9;
  gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
  gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &gpio_init);

  gpio_init.GPIO_Pin = GPIO_Pin_10;
  gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &gpio_init);

  USART_StructInit(&usart_init);
  usart_init.USART_BaudRate = 115200U;
  usart_init.USART_WordLength = USART_WordLength_8b;
  usart_init.USART_StopBits = USART_StopBits_1;
  usart_init.USART_Parity = USART_Parity_No;
  usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART1, &usart_init);
  USART_Cmd(USART1, ENABLE);

  g_usart1_write_count = 0U;
  g_usart1_ready = true;
}

bool drvUsart1IsReady(void) {

  return g_usart1_ready;
}

void drvUsart1WriteChar(char ch) {

  if (!g_usart1_ready) {
    return;
  }

  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
  }
  USART_SendData(USART1, (uint16_t)(uint8_t)ch);
}

void drvUsart1Write(const char *text) {

  if (text == NULL) {
    return;
  }

  while (*text != '\0') {
    drvUsart1WriteChar(*text);
    text++;
  }

  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
  }

  g_usart1_write_count++;
}

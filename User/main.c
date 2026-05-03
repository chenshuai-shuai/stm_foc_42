#include "ch.h"
#include "app_main.h"
#include "hal_uart_if.h"
#include "stm32f10x_rcc.h"
#include "system_stm32f10x.h"

#include <stdio.h>

int main(void) {
  char tx_buffer[128];
  uint32_t reset_flags;

  SystemCoreClockUpdate();
  halUartInit();
  halUartWrite("\r\n[BOOT] main entered\r\n");
  reset_flags = 0U;
  if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
    reset_flags |= 0x01U;
  }
  if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) {
    reset_flags |= 0x02U;
  }
  if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET) {
    reset_flags |= 0x04U;
  }
  if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
    reset_flags |= 0x08U;
  }
  if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) {
    reset_flags |= 0x10U;
  }
  if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET) {
    reset_flags |= 0x20U;
  }
  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "[BOOT] reset flags=0x%02lX\r\n",
                 (unsigned long)reset_flags);
  halUartWrite(tx_buffer);
  RCC_ClearFlag();
  halUartWrite("[BOOT] system clock updated\r\n");

  appInit();
}

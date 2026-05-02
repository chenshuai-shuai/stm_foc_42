#include "ch.h"
#include "app_main.h"
#include "hal_uart_if.h"
#include "system_stm32f10x.h"

int main(void) {

  SystemCoreClockUpdate();
  halUartInit();
  halUartWrite("\r\n[BOOT] main entered\r\n");
  halUartWrite("[BOOT] system clock updated\r\n");

  appInit();
}

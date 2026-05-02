#include "hal_uart_if.h"

#include <stddef.h>
#include <stdio.h>

#include "drv_usart1.h"

void halUartInit(void) {

  drvUsart1Init();
}

void halUartWrite(const char *text) {

  drvUsart1Write(text);
}

void halUartPublishRuntime(const hal_uart_runtime_frame_t *frame) {

  char tx_buffer[96];

  if (frame == NULL) {
    return;
  }

  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "sec=%lu,state=%u,fault=0x%02lX,duty=%u\r\n",
                 (unsigned long)frame->rtos_seconds,
                 (unsigned)frame->state,
                 (unsigned long)frame->fault_flags,
                 (unsigned)frame->duty_a_permille);

  drvUsart1Write(tx_buffer);
}

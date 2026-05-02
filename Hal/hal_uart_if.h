#ifndef HAL_UART_IF_H
#define HAL_UART_IF_H

#include <stdint.h>

typedef struct {
  uint32_t rtos_seconds;
  uint32_t fault_flags;
  uint16_t duty_a_permille;
  uint8_t state;
} hal_uart_runtime_frame_t;

void halUartInit(void);
void halUartWrite(const char *text);
void halUartPublishRuntime(const hal_uart_runtime_frame_t *frame);

#endif /* HAL_UART_IF_H */

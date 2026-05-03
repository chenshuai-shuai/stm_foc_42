#ifndef HAL_UART_IF_H
#define HAL_UART_IF_H

#include <stdint.h>

typedef struct {
  uint32_t rtos_seconds;
  uint32_t fault_flags;
  uint32_t command_revision;
  uint16_t duty_a_permille;
  int16_t target_current_ma;
  int16_t hold_current_ma;
  int16_t target_speed_rpm;
  int16_t measured_speed_rpm;
  int16_t filtered_speed_rpm;
  int16_t speed_current_ref_ma;
  int32_t target_position_total_decideg;
  uint8_t state;
  uint8_t control_mode;
  uint8_t command_source;
} hal_uart_runtime_frame_t;

void halUartInit(void);
void halUartWrite(const char *text);
void halUartPublishRuntime(const hal_uart_runtime_frame_t *frame);

#endif /* HAL_UART_IF_H */

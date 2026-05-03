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

  char tx_buffer[144];

  if (frame == NULL) {
    return;
  }

  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "sec=%lu,state=%u,mode=%u,fault=0x%02lX,duty=%u,run_i=%d,hold_i=%d,spd_ref=%d,spd_meas=%d,spd_filt=%d,iq_ref=%d,tpos=%ld,cmd_rev=%lu,cmd_src=%u\r\n",
                 (unsigned long)frame->rtos_seconds,
                 (unsigned)frame->state,
                 (unsigned)frame->control_mode,
                 (unsigned long)frame->fault_flags,
                 (unsigned)frame->duty_a_permille,
                 (int)frame->target_current_ma,
                 (int)frame->hold_current_ma,
                 (int)frame->target_speed_rpm,
                 (int)frame->measured_speed_rpm,
                 (int)frame->filtered_speed_rpm,
                 (int)frame->speed_current_ref_ma,
                 (long)frame->target_position_total_decideg,
                 (unsigned long)frame->command_revision,
                 (unsigned)frame->command_source);

  drvUsart1Write(tx_buffer);
}

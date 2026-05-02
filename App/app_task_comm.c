#include "ch.h"

#include "app_main.h"
#include "app_task_comm.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waCommThread, 384);

static THD_FUNCTION(CommThread, arg) {

  hal_uart_runtime_frame_t frame;
  app_runtime_t *runtime;

  (void)arg;
  runtime = (app_runtime_t *)appGetRuntime();
  chRegSetThreadName("comm");

  while (true) {
    frame.rtos_seconds = runtime->rtos_seconds;
    frame.state = runtime->state;
    frame.fault_flags = runtime->fault_flags;
    frame.duty_a_permille = runtime->duty_a_permille;

    halUartPublishRuntime(&frame);
    runtime->comm_ticks++;
    chThdSleepMilliseconds(500);
  }
}

void appTaskCommStart(void) {

  (void)chThdCreateStatic(waCommThread,
                          sizeof(waCommThread),
                          NORMALPRIO - 1U,
                          CommThread,
                          NULL);
}

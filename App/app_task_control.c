#include "ch.h"

#include "app_fastloop.h"
#include "app_main.h"
#include "app_task_control.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waControlThread, 512);

static THD_FUNCTION(ControlThread, arg) {

  static const app_command_t command = {
    true,
    1200,
    600,
  };
  app_runtime_t *runtime;

  (void)arg;
  runtime = (app_runtime_t *)appGetRuntime();
  chRegSetThreadName("ctrl");
  halUartWrite("[CTRL] thread entered\r\n");

  while (true) {
    appFastLoopStep(&command, runtime);
    chThdSleepMilliseconds(100);
    runtime->control_ticks++;
  }
}

void appTaskControlStart(void) {

  (void)chThdCreateStatic(waControlThread,
                          sizeof(waControlThread),
                          NORMALPRIO + 1U,
                          ControlThread,
                          NULL);
}

#include "ch.h"

#include "app_fastloop.h"
#include "app_main.h"
#include "app_task_control.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waControlThread, 512);

static THD_FUNCTION(ControlThread, arg) {
  app_command_snapshot_t command_snapshot;

  (void)arg;
  chRegSetThreadName("ctrl");
  halUartWrite("[CTRL] thread entered\r\n");

  while (true) {
    appCommandGetSnapshot(&command_snapshot);
    appFastLoopStep(&command_snapshot.value);
    chThdSleepMilliseconds(1);
    appRuntimeIncrementControlTicks();
  }
}

void appTaskControlStart(void) {

  (void)chThdCreateStatic(waControlThread,
                          sizeof(waControlThread),
                          NORMALPRIO + 1U,
                          ControlThread,
                          NULL);
}

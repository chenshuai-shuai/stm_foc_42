#include "ch.h"

#include "app_calib.h"
#include "app_task_calib.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waCalibThread, 2048);

static THD_FUNCTION(CalibThread, arg) {

  (void)arg;
  chRegSetThreadName("calib");
  halUartWrite("[CAL] thread entered\r\n");

  while (true) {
    appCalibService();
    chThdSleepMilliseconds(10);
  }
}

void appTaskCalibStart(void) {

  (void)chThdCreateStatic(waCalibThread,
                          sizeof(waCalibThread),
                          LOWPRIO,
                          CalibThread,
                          NULL);
}

#include "hal_board.h"

#include "drv_gpio.h"
#include "drv_irq.h"
#include "hal_gate_if.h"
#include "hal_keys_if.h"
#include "hal_motor_if.h"
#include "hal_sense_if.h"
#include "hal_uart_if.h"

void halBoardInit(void) {

  drvGpioInit();
  drvIrqInit();
  halSenseInit();
  halMotorInit();
  halGateInit();
  halUartInit();
  halKeysInit();
}

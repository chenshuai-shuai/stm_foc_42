#include "hal_gate_if.h"

#include "drv_gpio.h"

void halGateInit(void) {

  drvGatePinInit();
  drvGatePinWrite(false);
}

void halGateSetEnabled(bool enabled) {

  drvGatePinWrite(enabled);
}

bool halGateIsEnabled(void) {

  return drvGatePinRead();
}

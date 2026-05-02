#include "drv_gpio.h"

static bool g_gate_enabled;

void drvGpioInit(void) {

  g_gate_enabled = false;
}

void drvGatePinInit(void) {

  g_gate_enabled = false;
}

void drvGatePinWrite(bool enabled) {

  g_gate_enabled = enabled;
}

bool drvGatePinRead(void) {

  return g_gate_enabled;
}

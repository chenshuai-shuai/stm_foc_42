#include "app_fault_mgr.h"

#include <stddef.h>

void appFaultMgrInit(void) {
}

unsigned appFaultMgrCheck(const motor_feedback_t *feedback) {

  unsigned fault_flags;

  if (feedback == NULL) {
    return APP_FAULT_BUS_VOLTAGE_LOW;
  }

  fault_flags = APP_FAULT_NONE;

  if (feedback->bus_voltage_mv < 10000U) {
    fault_flags |= APP_FAULT_BUS_VOLTAGE_LOW;
  }

  if ((feedback->phase_a_current_ma > 1500) ||
      (feedback->phase_a_current_ma < -1500)) {
    fault_flags |= APP_FAULT_PHASE_CURRENT_HIGH;
  }

  return fault_flags;
}

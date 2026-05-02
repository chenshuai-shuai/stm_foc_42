#include "app_state_machine.h"

static app_state_t g_app_state = APP_STATE_BOOT;

void appStateMachineInit(void) {

  g_app_state = APP_STATE_BOOT;
}

app_state_t appStateMachineStep(uint32_t control_ticks, uint32_t fault_flags) {

  if (fault_flags != 0U) {
    g_app_state = APP_STATE_FAULT;
    return g_app_state;
  }

  switch (g_app_state) {
    case APP_STATE_BOOT:
      g_app_state = APP_STATE_IDLE;
      break;

    case APP_STATE_IDLE:
      if (control_ticks >= 5U) {
        g_app_state = APP_STATE_RUN;
      }
      break;

    case APP_STATE_RUN:
      break;

    case APP_STATE_FAULT:
    default:
      break;
  }

  return g_app_state;
}

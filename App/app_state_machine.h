#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include <stdint.h>

typedef enum {
  APP_STATE_BOOT = 0,
  APP_STATE_IDLE,
  APP_STATE_RUN,
  APP_STATE_FAULT
} app_state_t;

void appStateMachineInit(void);
app_state_t appStateMachineStep(uint32_t control_ticks, uint32_t fault_flags);

#endif /* APP_STATE_MACHINE_H */

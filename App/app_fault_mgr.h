#ifndef APP_FAULT_MGR_H
#define APP_FAULT_MGR_H

#include "algo_motor_types.h"

#define APP_FAULT_NONE                0U
#define APP_FAULT_BUS_VOLTAGE_LOW     (1U << 0)
#define APP_FAULT_PHASE_CURRENT_HIGH  (1U << 1)

void appFaultMgrInit(void);
unsigned appFaultMgrCheck(const motor_feedback_t *feedback);

#endif /* APP_FAULT_MGR_H */

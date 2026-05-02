#ifndef HAL_SENSE_IF_H
#define HAL_SENSE_IF_H

#include "algo_motor_types.h"

void halSenseInit(void);
void halSenseGetFeedbackSnapshot(motor_feedback_t *feedback);

#endif /* HAL_SENSE_IF_H */

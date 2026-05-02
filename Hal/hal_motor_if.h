#ifndef HAL_MOTOR_IF_H
#define HAL_MOTOR_IF_H

#include "algo_motor_types.h"

void halMotorInit(void);
void halMotorApplyOutput(const motor_control_output_t *output);
uint16_t halMotorGetDutyA(void);

#endif /* HAL_MOTOR_IF_H */

#include "hal_motor_if.h"

#include <stddef.h>

#include "drv_tim1_pwm.h"

void halMotorInit(void) {

  drvTim1PwmInit();
}

void halMotorApplyOutput(const motor_control_output_t *output) {

  drv_tim1_pwm_raw_t raw;

  if (output == NULL) {
    return;
  }

  raw.duty_a_permille = output->duty_a_permille;
  raw.duty_b_permille = output->duty_b_permille;
  raw.duty_c_permille = output->duty_c_permille;
  raw.enable = output->pwm_enable;

  drvTim1PwmApply(&raw);
}

uint16_t halMotorGetDutyA(void) {

  drv_tim1_pwm_raw_t raw;

  drvTim1PwmGetLast(&raw);
  return raw.duty_a_permille;
}

#include "drv_tim1_pwm.h"

#include <stddef.h>

static drv_tim1_pwm_raw_t g_pwm_last;

void drvTim1PwmInit(void) {

  g_pwm_last.duty_a_permille = 0U;
  g_pwm_last.duty_b_permille = 0U;
  g_pwm_last.duty_c_permille = 0U;
  g_pwm_last.enable = false;
}

void drvTim1PwmApply(const drv_tim1_pwm_raw_t *raw) {

  if (raw == NULL) {
    return;
  }

  g_pwm_last = *raw;
}

void drvTim1PwmGetLast(drv_tim1_pwm_raw_t *raw) {

  if (raw == NULL) {
    return;
  }

  *raw = g_pwm_last;
}

#ifndef DRV_TIM1_PWM_H
#define DRV_TIM1_PWM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t duty_a_permille;
  uint16_t duty_b_permille;
  bool phase_a_forward;
  bool phase_b_forward;
  bool enable;
} drv_tim1_pwm_raw_t;

void drvTim1PwmInit(void);
void drvTim1PwmApply(const drv_tim1_pwm_raw_t *raw);
void drvTim1PwmGetLast(drv_tim1_pwm_raw_t *raw);

#endif /* DRV_TIM1_PWM_H */

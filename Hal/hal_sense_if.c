#include "hal_sense_if.h"

#include <stddef.h>

#include "drv_adc1.h"

void halSenseInit(void) {

  drvAdc1Init();
}

void halSenseGetFeedbackSnapshot(motor_feedback_t *feedback) {

  drv_adc1_raw_t raw;

  if (feedback == NULL) {
    return;
  }

  drvAdc1GetLatest(&raw);

  feedback->phase_a_current_ma = raw.phase_a_current_ma;
  feedback->phase_b_current_ma = raw.phase_b_current_ma;
  feedback->bus_voltage_mv = raw.bus_voltage_mv;
  feedback->electrical_angle_deg = raw.electrical_angle_deg;
  feedback->mechanical_speed_rpm = raw.mechanical_speed_rpm;
}

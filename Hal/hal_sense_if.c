#include "hal_sense_if.h"

#include <stddef.h>

#include "drv_adc1.h"
#include "hal_encoder_if.h"

void halSenseInit(void) {

  drvAdc1Init();
  halEncoderInit();
}

void halSenseGetFeedbackSnapshot(motor_feedback_t *feedback) {

  drv_adc1_raw_t raw;
  hal_encoder_sample_t encoder;

  if (feedback == NULL) {
    return;
  }

  drvAdc1GetLatest(&raw);
  halEncoderGetSample(&encoder);

  feedback->phase_a_current_ma = raw.phase_a_current_ma;
  feedback->phase_b_current_ma = raw.phase_b_current_ma;
  feedback->bus_voltage_mv = raw.bus_voltage_mv;
  feedback->electrical_angle_deg = 0;
  feedback->electrical_angle_decideg = 0U;
  feedback->mechanical_speed_rpm = encoder.mechanical_speed_rpm;
  feedback->mechanical_angle_decideg = encoder.mechanical_angle_decideg;
  feedback->mechanical_turn_count = encoder.mechanical_turn_count;
  feedback->encoder_raw = encoder.raw_angle;
  feedback->encoder_ready = encoder.ready;
}

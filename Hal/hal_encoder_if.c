#include "hal_encoder_if.h"

#include <stddef.h>

#include "algo_angle.h"
#include "drv_mt6816.h"

static bool g_has_sample;
static uint16_t g_last_mechanical_angle_decideg;
static int32_t g_mechanical_turn_count;

void halEncoderInit(void) {

  drvMt6816Init();
  halEncoderResetTracking();
}

void halEncoderResetTracking(void) {
  g_has_sample = false;
  g_last_mechanical_angle_decideg = 0U;
  g_mechanical_turn_count = 0;
}

void halEncoderGetSample(hal_encoder_sample_t *sample) {
  drv_mt6816_sample_t drv_sample;
  int32_t delta;
  uint16_t corrected_angle_decideg;

  if (sample == NULL) {
    return;
  }

  drvMt6816GetSample(&drv_sample);

  corrected_angle_decideg = algoAngleGetMechanicalAngleDecideg(drv_sample.raw_angle);

  if (g_has_sample) {
    delta = (int32_t)corrected_angle_decideg - (int32_t)g_last_mechanical_angle_decideg;

    if (delta > 1800) {
      g_mechanical_turn_count--;
    } else if (delta < -1800) {
      g_mechanical_turn_count++;
    }
  } else {
    g_has_sample = true;
  }

  g_last_mechanical_angle_decideg = corrected_angle_decideg;

  sample->raw_angle = drv_sample.raw_angle;
  sample->mechanical_angle_decideg = corrected_angle_decideg;
  sample->mechanical_turn_count = g_mechanical_turn_count;
  sample->mechanical_speed_rpm = 0;
  sample->ready = drv_sample.ready;
}

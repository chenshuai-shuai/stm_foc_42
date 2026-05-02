#include "hal_encoder_if.h"

#include <stddef.h>

#include "drv_mt6816.h"

void halEncoderInit(void) {

  drvMt6816Init();
}

void halEncoderGetSample(hal_encoder_sample_t *sample) {
  drv_mt6816_sample_t drv_sample;

  if (sample == NULL) {
    return;
  }

  drvMt6816GetSample(&drv_sample);

  sample->raw_angle = drv_sample.raw_angle;
  sample->mechanical_angle_decideg = drv_sample.mechanical_angle_decideg;
  sample->mechanical_turn_count = drv_sample.mechanical_turn_count;
  sample->mechanical_speed_rpm = 0;
  sample->ready = drv_sample.ready;
}

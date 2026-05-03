#ifndef HAL_ENCODER_IF_H
#define HAL_ENCODER_IF_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t raw_angle;
  uint16_t mechanical_angle_decideg;
  int32_t mechanical_turn_count;
  int16_t mechanical_speed_rpm;
  bool ready;
} hal_encoder_sample_t;

void halEncoderInit(void);
void halEncoderGetSample(hal_encoder_sample_t *sample);
void halEncoderResetTracking(void);

#endif /* HAL_ENCODER_IF_H */

#ifndef ALGO_ANGLE_H
#define ALGO_ANGLE_H

#include <stdint.h>

typedef struct {
  uint16_t encoder_cpr;
  uint8_t pole_pairs;
  int8_t encoder_direction;
  uint16_t encoder_zero_raw;
} algo_angle_config_t;

void algoAngleInit(const algo_angle_config_t *config);
uint16_t algoAngleGetMechanicalAngleDecideg(uint16_t encoder_raw);
uint16_t algoAngleGetElectricalAngleDecideg(uint16_t encoder_raw);

#endif /* ALGO_ANGLE_H */

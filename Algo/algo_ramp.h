#ifndef ALGO_RAMP_H
#define ALGO_RAMP_H

#include <stdint.h>

typedef struct {
  int32_t value;
  int32_t step_up;
  int32_t step_down;
} algo_ramp_t;

void algoRampInit(algo_ramp_t *ramp, int32_t initial, int32_t step_up, int32_t step_down);
int32_t algoRampStep(algo_ramp_t *ramp, int32_t target);

#endif /* ALGO_RAMP_H */

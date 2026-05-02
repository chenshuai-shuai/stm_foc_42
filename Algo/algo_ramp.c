#include "algo_ramp.h"

#include <stddef.h>

void algoRampInit(algo_ramp_t *ramp,
                  int32_t initial,
                  int32_t step_up,
                  int32_t step_down) {

  if (ramp == NULL) {
    return;
  }

  ramp->value = initial;
  ramp->step_up = step_up;
  ramp->step_down = step_down;
}

int32_t algoRampStep(algo_ramp_t *ramp, int32_t target) {

  if (ramp == NULL) {
    return target;
  }

  if (target > ramp->value) {
    ramp->value += ramp->step_up;
    if (ramp->value > target) {
      ramp->value = target;
    }
  } else if (target < ramp->value) {
    ramp->value -= ramp->step_down;
    if (ramp->value < target) {
      ramp->value = target;
    }
  }

  return ramp->value;
}

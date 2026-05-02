#include "algo_pid.h"

#include <stddef.h>

static int32_t algoPidClamp(int32_t value, int32_t low, int32_t high) {

  if (value < low) {
    return low;
  }

  if (value > high) {
    return high;
  }

  return value;
}

void algoPidInit(algo_pid_t *pid,
                 int32_t kp,
                 int32_t ki,
                 int32_t output_min,
                 int32_t output_max) {

  if (pid == NULL) {
    return;
  }

  pid->kp = kp;
  pid->ki = ki;
  pid->integral = 0;
  pid->output_min = output_min;
  pid->output_max = output_max;
}

void algoPidReset(algo_pid_t *pid) {

  if (pid == NULL) {
    return;
  }

  pid->integral = 0;
}

int32_t algoPidStep(algo_pid_t *pid, int32_t error) {

  int32_t output;

  if (pid == NULL) {
    return 0;
  }

  pid->integral += error * pid->ki;
  pid->integral = algoPidClamp(pid->integral,
                               pid->output_min * 16,
                               pid->output_max * 16);

  output = error * pid->kp + pid->integral / 16;
  return algoPidClamp(output, pid->output_min, pid->output_max);
}

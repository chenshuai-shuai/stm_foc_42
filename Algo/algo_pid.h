#ifndef ALGO_PID_H
#define ALGO_PID_H

#include <stdint.h>

typedef struct {
  int32_t kp;
  int32_t ki;
  int32_t integral;
  int32_t output_min;
  int32_t output_max;
} algo_pid_t;

void algoPidInit(algo_pid_t *pid,
                 int32_t kp,
                 int32_t ki,
                 int32_t output_min,
                 int32_t output_max);
void algoPidReset(algo_pid_t *pid);
int32_t algoPidStep(algo_pid_t *pid, int32_t error);

#endif /* ALGO_PID_H */

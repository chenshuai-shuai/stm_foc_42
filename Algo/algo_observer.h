#ifndef ALGO_OBSERVER_H
#define ALGO_OBSERVER_H

#include <stdint.h>

typedef struct {
  int32_t estimated_speed_rpm;
} algo_observer_t;

void algoObserverInit(algo_observer_t *observer);
int32_t algoObserverStep(algo_observer_t *observer, int32_t measured_speed_rpm);

#endif /* ALGO_OBSERVER_H */

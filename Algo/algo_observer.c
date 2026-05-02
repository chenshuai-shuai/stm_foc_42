#include "algo_observer.h"

#include <stddef.h>

void algoObserverInit(algo_observer_t *observer) {

  if (observer == NULL) {
    return;
  }

  observer->estimated_speed_rpm = 0;
}

int32_t algoObserverStep(algo_observer_t *observer, int32_t measured_speed_rpm) {

  if (observer == NULL) {
    return measured_speed_rpm;
  }

  observer->estimated_speed_rpm =
      (observer->estimated_speed_rpm * 3 + measured_speed_rpm) / 4;

  return observer->estimated_speed_rpm;
}

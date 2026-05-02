#ifndef HAL_GATE_IF_H
#define HAL_GATE_IF_H

#include <stdbool.h>

void halGateInit(void);
void halGateSetEnabled(bool enabled);
bool halGateIsEnabled(void);

#endif /* HAL_GATE_IF_H */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "app_types.h"

void appInit(void);
void appStartSystemTick(void);
const app_runtime_t *appGetRuntime(void);

#endif /* APP_MAIN_H */

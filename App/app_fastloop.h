#ifndef APP_FASTLOOP_H
#define APP_FASTLOOP_H

#include "app_types.h"

void appFastLoopInit(void);
void appFastLoopStep(const app_command_t *command, app_runtime_t *runtime);

#endif /* APP_FASTLOOP_H */

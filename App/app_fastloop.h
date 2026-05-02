#ifndef APP_FASTLOOP_H
#define APP_FASTLOOP_H

#include "app_types.h"

void appFastLoopInit(void);
void appFastLoopStep(const app_command_t *command);

#endif /* APP_FASTLOOP_H */

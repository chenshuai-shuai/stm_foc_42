#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "app_types.h"

void appInit(void);
void appStartSystemTick(void);
void appRuntimeGetSnapshot(app_runtime_t *runtime);
void appRuntimeIncrementSeconds(void);
void appRuntimeIncrementControlTicks(void);
void appRuntimeIncrementCommTicks(void);
void appRuntimePublishFastLoop(const app_runtime_fastloop_update_t *update);
void appCommandGetSnapshot(app_command_snapshot_t *command_snapshot);
void appCommandSubmitFromUi(const app_command_t *command);
void appCommandSubmitFromComm(const app_command_t *command);
void appCommandSubmitFromParam(const app_command_t *command);
void appCommandSubmitFromSafety(const app_command_t *command);

#endif /* APP_MAIN_H */

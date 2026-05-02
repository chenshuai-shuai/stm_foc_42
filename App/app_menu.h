#ifndef APP_MENU_H
#define APP_MENU_H

#include <stdint.h>

#include "app_types.h"
#include "hal_oled_if.h"

void appMenuInit(void);
void appMenuMoveNext(void);
void appMenuMovePrev(void);
void appMenuEnter(void);
void appMenuBack(void);
void appMenuDemoStep(void);
uint8_t appMenuActivate(const app_command_t *current_command,
                        app_command_t *next_command);
uint8_t appMenuNeedsPeriodicRefresh(void);
void appMenuBuildView(const app_command_snapshot_t *command_snapshot,
                      const app_runtime_t *runtime,
                      hal_oled_menu_view_t *view);

#endif /* APP_MENU_H */

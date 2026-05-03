#ifndef APP_MENU_H
#define APP_MENU_H

#include <stdint.h>

#include "app_types.h"
#include "hal_oled_if.h"

typedef enum {
  APP_MENU_ENTER_RESULT_NONE = 0,
  APP_MENU_ENTER_RESULT_NAVIGATED,
  APP_MENU_ENTER_RESULT_COMMAND_UPDATED,
  APP_MENU_ENTER_RESULT_LOCAL_ACTION
} app_menu_enter_result_t;

void appMenuInit(void);
void appMenuMoveNext(void);
void appMenuMovePrev(void);
void appMenuEnter(void);
void appMenuBack(void);
void appMenuDemoStep(void);
app_menu_enter_result_t appMenuActivate(const app_command_t *current_command,
                                        const app_runtime_t *runtime,
                                        app_command_t *next_command);
uint8_t appMenuNeedsPeriodicRefresh(void);
void appMenuBuildView(const app_command_snapshot_t *command_snapshot,
                      const app_runtime_t *runtime,
                      hal_oled_menu_view_t *view);
void appMenuReset(void);

#endif /* APP_MENU_H */

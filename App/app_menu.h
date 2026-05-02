#ifndef APP_MENU_H
#define APP_MENU_H

#include "hal_oled_if.h"

void appMenuInit(void);
void appMenuMoveNext(void);
void appMenuMovePrev(void);
void appMenuEnter(void);
void appMenuBack(void);
void appMenuDemoStep(void);
void appMenuBuildView(hal_oled_menu_view_t *view);

#endif /* APP_MENU_H */

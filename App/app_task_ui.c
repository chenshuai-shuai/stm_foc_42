#include "ch.h"

#include "app_menu.h"
#include "app_main.h"
#include "app_task_ui.h"
#include "hal_keys_if.h"
#include "hal_oled_if.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waUiThread, 1024);

static THD_FUNCTION(UiThread, arg) {

  hal_oled_menu_view_t view;
  uint8_t redraw_pending;

  (void)arg;
  chRegSetThreadName("ui");
  halUartWrite("[UI] thread entered\r\n");

  halOledInit();
  appMenuInit();
  halUartWrite("[UI] display ready\r\n");
  redraw_pending = 1U;

  while (true) {
    hal_key_event_t key_event = halKeysPoll();

    switch (key_event) {
    case HAL_KEY_EVENT_PREV:
      appMenuMovePrev();
      redraw_pending = 1U;
      break;

    case HAL_KEY_EVENT_NEXT:
      appMenuMoveNext();
      redraw_pending = 1U;
      break;

    case HAL_KEY_EVENT_ENTER:
      appMenuEnter();
      redraw_pending = 1U;
      break;

    case HAL_KEY_EVENT_BACK:
      appMenuBack();
      redraw_pending = 1U;
      break;

    case HAL_KEY_EVENT_NONE:
    default:
      break;
    }

    if (redraw_pending != 0U) {
      appMenuBuildView(&view);
      halOledShowMenu(&view);
      redraw_pending = 0U;
    }

    chThdSleepMilliseconds(20);
  }
}

void appTaskUiStart(void) {

  (void)chThdCreateStatic(waUiThread,
                          sizeof(waUiThread),
                          NORMALPRIO,
                          UiThread,
                          NULL);
}

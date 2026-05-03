#include "ch.h"

#include "app_calib.h"
#include "app_menu.h"
#include "app_main.h"
#include "app_task_ui.h"
#include "hal_keys_if.h"
#include "hal_oled_if.h"
#include "hal_uart_if.h"

static THD_WORKING_AREA(waUiThread, 1024);

static THD_FUNCTION(UiThread, arg) {

  hal_oled_menu_view_t view;
  app_command_snapshot_t command_snapshot;
  app_runtime_t runtime_snapshot;
  app_command_t next_command;
  systime_t last_refresh_tick;
  uint8_t redraw_pending;

  (void)arg;
  chRegSetThreadName("ui");
  halUartWrite("[UI] thread entered\r\n");

  halOledInit();
  appMenuInit();
  halUartWrite("[UI] display ready\r\n");
  last_refresh_tick = chVTGetSystemTimeX();
  redraw_pending = 1U;

  while (true) {
    app_menu_enter_result_t enter_result;
    hal_key_event_t key_event = halKeysPoll();
    appCommandGetSnapshot(&command_snapshot);

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
      appRuntimeGetSnapshot(&runtime_snapshot);
      enter_result =
          appMenuActivate(&command_snapshot.value, &runtime_snapshot, &next_command);
      if (enter_result == APP_MENU_ENTER_RESULT_COMMAND_UPDATED) {
        appCommandSubmitFromUi(&next_command);
        halUartWrite("[UI] command submit\r\n");
        redraw_pending = 1U;
      } else if (enter_result == APP_MENU_ENTER_RESULT_LOCAL_ACTION) {
        halUartWrite("[UI] local action\r\n");
        redraw_pending = 1U;
      } else {
        appMenuEnter();
        halUartWrite("[UI] menu enter\r\n");
        redraw_pending = 1U;
      }
      break;

    case HAL_KEY_EVENT_BACK:
      appMenuBack();
      redraw_pending = 1U;
      break;

    case HAL_KEY_EVENT_NONE:
    default:
      break;
    }

    if ((redraw_pending == 0U) && (appMenuNeedsPeriodicRefresh() != 0U)) {
      if (chVTTimeElapsedSinceX(last_refresh_tick) >= TIME_MS2I(33)) {
        redraw_pending = 1U;
      }
    }

    if (redraw_pending != 0U) {
      appCommandGetSnapshot(&command_snapshot);
      appRuntimeGetSnapshot(&runtime_snapshot);
      appMenuBuildView(&command_snapshot, &runtime_snapshot, &view);
      halOledShowMenu(&view);
      last_refresh_tick = chVTGetSystemTimeX();
      redraw_pending = 0U;
    }

    chThdSleepMilliseconds(5);
  }
}

void appTaskUiStart(void) {

  (void)chThdCreateStatic(waUiThread,
                          sizeof(waUiThread),
                          NORMALPRIO,
                          UiThread,
                          NULL);
}

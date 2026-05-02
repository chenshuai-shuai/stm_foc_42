#include "hal_keys_if.h"

#include "drv_keys.h"

static uint8_t g_key3_long_latched;

void halKeysInit(void) {

  drvKeysInit();
  g_key3_long_latched = 0U;
}

hal_key_event_t halKeysPoll(void) {
  drv_key_state_t key1;
  drv_key_state_t key2;
  drv_key_state_t key3;

  drvKeysScan();

  key1 = drvKeysGetState(DRV_KEY_ID_1);
  key2 = drvKeysGetState(DRV_KEY_ID_2);
  key3 = drvKeysGetState(DRV_KEY_ID_3);

  if (key3.long_press_event != 0U) {
    g_key3_long_latched = 1U;
    return HAL_KEY_EVENT_BACK;
  }

  if (key1.pressed_event != 0U) {
    return HAL_KEY_EVENT_PREV;
  }

  if (key2.pressed_event != 0U) {
    return HAL_KEY_EVENT_NEXT;
  }

  if (key3.released_event != 0U) {
    if (g_key3_long_latched != 0U) {
      g_key3_long_latched = 0U;
      return HAL_KEY_EVENT_NONE;
    }

    return HAL_KEY_EVENT_ENTER;
  }

  return HAL_KEY_EVENT_NONE;
}

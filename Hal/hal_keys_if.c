#include "ch.h"

#include "hal_keys_if.h"

#include "drv_keys.h"

#define HAL_KEYS_QUEUE_DEPTH 8U

static uint8_t g_key3_long_latched;
static hal_key_event_t g_key_event_queue[HAL_KEYS_QUEUE_DEPTH];
static uint8_t g_key_event_read_index;
static uint8_t g_key_event_write_index;
static uint8_t g_key_event_count;
static THD_WORKING_AREA(waKeysThread, 256);

static void halKeysQueuePush(hal_key_event_t event) {

  if ((event == HAL_KEY_EVENT_NONE) || (g_key_event_count >= HAL_KEYS_QUEUE_DEPTH)) {
    return;
  }

  g_key_event_queue[g_key_event_write_index] = event;
  g_key_event_write_index++;
  if (g_key_event_write_index >= HAL_KEYS_QUEUE_DEPTH) {
    g_key_event_write_index = 0U;
  }
  g_key_event_count++;
}

static hal_key_event_t halKeysTranslateScan(void) {
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

static THD_FUNCTION(KeysThread, arg) {

  (void)arg;
  chRegSetThreadName("keys");

  while (true) {
    hal_key_event_t event = halKeysTranslateScan();

    if (event != HAL_KEY_EVENT_NONE) {
      chSysLock();
      halKeysQueuePush(event);
      chSysUnlock();
    }

    chThdSleepMilliseconds(1);
  }
}

void halKeysInit(void) {

  drvKeysInit();
  g_key3_long_latched = 0U;
  g_key_event_read_index = 0U;
  g_key_event_write_index = 0U;
  g_key_event_count = 0U;
}

void halKeysStart(void) {

  (void)chThdCreateStatic(waKeysThread,
                          sizeof(waKeysThread),
                          NORMALPRIO + 2U,
                          KeysThread,
                          NULL);
}

hal_key_event_t halKeysPoll(void) {
  hal_key_event_t event;

  chSysLock();
  if (g_key_event_count == 0U) {
    event = HAL_KEY_EVENT_NONE;
  } else {
    event = g_key_event_queue[g_key_event_read_index];
    g_key_event_read_index++;
    if (g_key_event_read_index >= HAL_KEYS_QUEUE_DEPTH) {
      g_key_event_read_index = 0U;
    }
    g_key_event_count--;
  }
  chSysUnlock();

  return event;
}

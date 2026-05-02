#ifndef HAL_KEYS_IF_H
#define HAL_KEYS_IF_H

typedef enum {
  HAL_KEY_EVENT_NONE = 0,
  HAL_KEY_EVENT_PREV,
  HAL_KEY_EVENT_NEXT,
  HAL_KEY_EVENT_ENTER,
  HAL_KEY_EVENT_BACK
} hal_key_event_t;

void halKeysInit(void);
void halKeysStart(void);
hal_key_event_t halKeysPoll(void);

#endif /* HAL_KEYS_IF_H */

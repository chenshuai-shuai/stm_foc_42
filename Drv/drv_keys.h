#ifndef DRV_KEYS_H
#define DRV_KEYS_H

#include <stdint.h>

typedef enum {
  DRV_KEY_ID_1 = 0,
  DRV_KEY_ID_2,
  DRV_KEY_ID_3,
  DRV_KEY_ID_COUNT
} drv_key_id_t;

typedef struct {
  uint8_t pressed;
  uint8_t pressed_event;
  uint8_t released_event;
  uint8_t long_press_event;
} drv_key_state_t;

void drvKeysInit(void);
void drvKeysScan(void);
drv_key_state_t drvKeysGetState(drv_key_id_t key_id);

#endif /* DRV_KEYS_H */

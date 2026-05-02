#include "drv_keys.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define DRV_KEYS_DEBOUNCE_TICKS 2U
#define DRV_KEYS_LONGPRESS_TICKS 30U

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
} drv_key_pin_t;

typedef struct {
  uint8_t stable_pressed;
  uint8_t debounce_ticks;
  uint8_t press_ticks;
  uint8_t long_reported;
  drv_key_state_t state;
} drv_key_runtime_t;

static const drv_key_pin_t g_key_pins[DRV_KEY_ID_COUNT] = {
    {GPIOB, GPIO_Pin_0},
    {GPIOB, GPIO_Pin_1},
    {GPIOA, GPIO_Pin_1},
};

static drv_key_runtime_t g_key_runtime[DRV_KEY_ID_COUNT];

static uint8_t drvKeysReadRawPressed(drv_key_id_t key_id) {

  return GPIO_ReadInputDataBit(g_key_pins[key_id].port, g_key_pins[key_id].pin) ==
                 Bit_RESET
             ? 1U
             : 0U;
}

void drvKeysInit(void) {
  GPIO_InitTypeDef gpio_init;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

  gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
  gpio_init.GPIO_Mode = GPIO_Mode_IPU;

  gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_Init(GPIOB, &gpio_init);

  gpio_init.GPIO_Pin = GPIO_Pin_1;
  GPIO_Init(GPIOA, &gpio_init);
}

void drvKeysScan(void) {
  drv_key_id_t key_id;

  for (key_id = DRV_KEY_ID_1; key_id < DRV_KEY_ID_COUNT; key_id++) {
    drv_key_runtime_t *runtime = &g_key_runtime[key_id];
    uint8_t raw_pressed = drvKeysReadRawPressed(key_id);

    runtime->state.pressed_event = 0U;
    runtime->state.released_event = 0U;
    runtime->state.long_press_event = 0U;

    if (raw_pressed == runtime->stable_pressed) {
      runtime->debounce_ticks = 0U;
    } else {
      runtime->debounce_ticks++;
      if (runtime->debounce_ticks >= DRV_KEYS_DEBOUNCE_TICKS) {
        runtime->debounce_ticks = 0U;
        runtime->stable_pressed = raw_pressed;
        runtime->state.pressed = raw_pressed;

        if (raw_pressed != 0U) {
          runtime->press_ticks = 0U;
          runtime->long_reported = 0U;
          runtime->state.pressed_event = 1U;
        } else {
          runtime->state.released_event = 1U;
          runtime->press_ticks = 0U;
          runtime->long_reported = 0U;
        }
      }
    }

    if (runtime->stable_pressed != 0U) {
      if (runtime->press_ticks < 0xFFU) {
        runtime->press_ticks++;
      }

      if ((runtime->long_reported == 0U) &&
          (runtime->press_ticks >= DRV_KEYS_LONGPRESS_TICKS)) {
        runtime->long_reported = 1U;
        runtime->state.long_press_event = 1U;
      }
    }
  }
}

drv_key_state_t drvKeysGetState(drv_key_id_t key_id) {

  return g_key_runtime[key_id].state;
}

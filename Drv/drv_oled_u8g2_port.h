#ifndef DRV_OLED_U8G2_PORT_H
#define DRV_OLED_U8G2_PORT_H

#include "u8g2.h"

uint8_t drvOledU8g2GpioAndDelay(u8x8_t *u8x8,
                                uint8_t msg,
                                uint8_t arg_int,
                                void *arg_ptr);

#endif /* DRV_OLED_U8G2_PORT_H */

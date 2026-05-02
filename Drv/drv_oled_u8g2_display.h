#ifndef DRV_OLED_U8G2_DISPLAY_H
#define DRV_OLED_U8G2_DISPLAY_H

#include "u8x8.h"

uint8_t drvOledU8g2Display(u8x8_t *u8x8,
                           uint8_t msg,
                           uint8_t arg_int,
                           void *arg_ptr);

#endif /* DRV_OLED_U8G2_DISPLAY_H */

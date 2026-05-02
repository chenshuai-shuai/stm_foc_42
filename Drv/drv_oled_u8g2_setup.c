#include "u8g2.h"

#include "drv_oled_u8g2_display.h"

uint8_t *u8g2_m_8_4_1(uint8_t *page_cnt) {

  static uint8_t buf[64];

  *page_cnt = 1U;
  return buf;
}

void u8g2_Setup_ssd1306_i2c_64x32_1f_1(u8g2_t *u8g2,
                                       const u8g2_cb_t *rotation,
                                       u8x8_msg_cb byte_cb,
                                       u8x8_msg_cb gpio_and_delay_cb) {

  uint8_t tile_buf_height;
  uint8_t *buf;

  u8g2_SetupDisplay(u8g2,
                    drvOledU8g2Display,
                    u8x8_cad_ssd13xx_fast_i2c,
                    byte_cb,
                    gpio_and_delay_cb);
  buf = u8g2_m_8_4_1(&tile_buf_height);
  u8g2_SetupBuffer(
      u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}

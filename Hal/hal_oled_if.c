#include "hal_oled_if.h"

#include <stdbool.h>

#include "OLED_Config.h"
#include "drv_oled_u8g2_port.h"
#include "u8g2.h"

extern const uint8_t u8g2_font_5x8_tr[];

static u8g2_t g_oled_u8g2;
static bool g_oled_ready;

static void halOledDrawLevelDots(uint8_t level) {
  uint8_t i;
  uint8_t dot_count = level;

  if (dot_count > 3U) {
    dot_count = 3U;
  }

  for (i = 0U; i < dot_count; i++) {
    u8g2_DrawBox(&g_oled_u8g2, (u8g2_uint_t)(54U + (i * 3U)), 2U, 2U, 2U);
  }
}

static void halOledDrawScrollBar(uint8_t first_visible, uint8_t total_rows) {
  uint8_t knob_y;
  uint8_t knob_h;

  if (total_rows <= HAL_OLED_MENU_ROWS) {
    return;
  }

  u8g2_DrawFrame(&g_oled_u8g2, 61U, 9U, 3U, 23U);

  knob_h = (uint8_t)(23U / total_rows);
  if (knob_h < 6U) {
    knob_h = 6U;
  }

  knob_y = (uint8_t)(10U + ((uint16_t)first_visible * (uint16_t)(21U - knob_h)) /
                                (uint16_t)(total_rows - HAL_OLED_MENU_ROWS));
  u8g2_DrawBox(&g_oled_u8g2, 62U, knob_y, 1U, knob_h);
}

static void halOledDrawChevron(uint8_t x, uint8_t y) {

  u8g2_DrawPixel(&g_oled_u8g2, x, y);
  u8g2_DrawPixel(&g_oled_u8g2, (u8g2_uint_t)(x + 1U), (u8g2_uint_t)(y + 1U));
  u8g2_DrawPixel(&g_oled_u8g2, (u8g2_uint_t)(x + 2U), (u8g2_uint_t)(y + 2U));
  u8g2_DrawPixel(&g_oled_u8g2, (u8g2_uint_t)(x + 1U), (u8g2_uint_t)(y + 3U));
  u8g2_DrawPixel(&g_oled_u8g2, x, (u8g2_uint_t)(y + 4U));
}

static void halOledDrawMenuRow(uint8_t row,
                               const char *label,
                               const char *value,
                               hal_oled_menu_row_kind_t kind,
                               bool selected) {

  const uint8_t y = (uint8_t)(8U + (row * 8U));
  const uint8_t text_x = 2U;

  if (selected) {
    u8g2_DrawBox(&g_oled_u8g2, 0U, y, 60U, 8U);
    u8g2_SetDrawColor(&g_oled_u8g2, 0U);
  } else {
    u8g2_SetDrawColor(&g_oled_u8g2, 1U);
  }

  if (kind != HAL_OLED_MENU_ROW_EMPTY) {
    u8g2_DrawStr(&g_oled_u8g2, text_x, (u8g2_uint_t)(y + 7U), label);
  }

  if ((kind == HAL_OLED_MENU_ROW_VALUE) && (value != NULL) && (value[0] != '\0')) {
    u8g2_DrawStr(&g_oled_u8g2, 34U, (u8g2_uint_t)(y + 7U), value);
  } else if (kind == HAL_OLED_MENU_ROW_SUBMENU) {
    halOledDrawChevron(55U, (uint8_t)(y + 1U));
  }

  u8g2_SetDrawColor(&g_oled_u8g2, 1U);
}

void halOledInit(void) {

  u8g2_Setup_ssd1306_i2c_64x32_1f_1(&g_oled_u8g2,
                                    U8G2_R0,
                                    u8x8_byte_sw_i2c,
                                    drvOledU8g2GpioAndDelay);
  u8x8_SetI2CAddress(&g_oled_u8g2.u8x8, OLED_I2C_ADDRESS);
  u8g2_InitDisplay(&g_oled_u8g2);
  u8g2_SetPowerSave(&g_oled_u8g2, 0U);
  u8g2_SetFont(&g_oled_u8g2, u8g2_font_5x8_tr);
  u8g2_SetFontMode(&g_oled_u8g2, 1U);
  u8g2_ClearDisplay(&g_oled_u8g2);
  g_oled_ready = true;
}

void halOledShowMenu(const hal_oled_menu_view_t *view) {
  uint8_t row;

  if ((!g_oled_ready) || (view == NULL) || (view->title == NULL)) {
    return;
  }

  u8g2_FirstPage(&g_oled_u8g2);
  do {
    u8g2_DrawBox(&g_oled_u8g2, 0U, 0U, 64U, 8U);
    u8g2_SetDrawColor(&g_oled_u8g2, 0U);
    if (view->has_parent != 0U) {
      u8g2_DrawStr(&g_oled_u8g2, 1U, 7U, "<");
      u8g2_DrawStr(&g_oled_u8g2, 7U, 7U, view->title);
    } else {
      u8g2_DrawStr(&g_oled_u8g2, 1U, 7U, view->title);
    }
    halOledDrawLevelDots(view->level);
    u8g2_SetDrawColor(&g_oled_u8g2, 1U);

    for (row = 0U; row < HAL_OLED_MENU_ROWS; row++) {
      const char *label = view->rows[row].label;
      const char *value = view->rows[row].value;

      if ((label == NULL) || (value == NULL)) {
        continue;
      }

      halOledDrawMenuRow(row,
                         label,
                         value,
                         view->rows[row].kind,
                         view->selected_row == row);
    }

    halOledDrawScrollBar(view->first_visible_row, view->total_rows);
  } while (u8g2_NextPage(&g_oled_u8g2) != 0U);
}

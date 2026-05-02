#ifndef HAL_OLED_IF_H
#define HAL_OLED_IF_H

#include <stdint.h>

#define HAL_OLED_MENU_ROWS 3U

typedef enum {
  HAL_OLED_MENU_ROW_EMPTY = 0,
  HAL_OLED_MENU_ROW_VALUE,
  HAL_OLED_MENU_ROW_SUBMENU
} hal_oled_menu_row_kind_t;

typedef struct {
  const char *label;
  const char *value;
  hal_oled_menu_row_kind_t kind;
} hal_oled_menu_row_t;

typedef struct {
  const char *title;
  hal_oled_menu_row_t rows[HAL_OLED_MENU_ROWS];
  uint8_t selected_row;
  uint8_t level;
  uint8_t has_parent;
  uint8_t first_visible_row;
  uint8_t total_rows;
} hal_oled_menu_view_t;

void halOledInit(void);
void halOledShowMenu(const hal_oled_menu_view_t *view);

#endif /* HAL_OLED_IF_H */

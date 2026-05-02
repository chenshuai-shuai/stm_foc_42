#include "app_menu.h"

#include <stddef.h>

typedef struct app_menu_page app_menu_page_t;

typedef struct {
  const char *label;
  const char *value;
  const app_menu_page_t *target;
} app_menu_item_t;

struct app_menu_page {
  const char *title;
  const app_menu_item_t *items;
  uint8_t item_count;
};

#define APP_MENU_STACK_DEPTH 4U

typedef struct {
  const app_menu_page_t *stack[APP_MENU_STACK_DEPTH];
  uint8_t depth;
  uint8_t selected[APP_MENU_STACK_DEPTH];
  uint8_t first_visible[APP_MENU_STACK_DEPTH];
} app_menu_state_t;

static const app_menu_page_t g_page_root;
static const app_menu_page_t g_page_control;
static const app_menu_page_t g_page_monitor;
static const app_menu_page_t g_page_system;
static const app_menu_page_t g_page_mode;
static const app_menu_page_t g_page_limits;
static const app_menu_page_t g_page_display;
static const app_menu_page_t g_page_comm;

static const app_menu_item_t g_items_root[] = {
  {"Control", "", &g_page_control},
  {"Monitor", "", &g_page_monitor},
  {"System", "", &g_page_system},
  {"Debug", "", NULL},
};

static const app_menu_item_t g_items_control[] = {
  {"Enable", "OFF", NULL},
  {"Mode", "", &g_page_mode},
  {"Limits", "", &g_page_limits},
  {"Assist", "Demo", NULL},
};

static const app_menu_item_t g_items_monitor[] = {
  {"Phase", "--", NULL},
  {"Bus", "--", NULL},
  {"Speed", "--", NULL},
  {"Trace", "", NULL},
};

static const app_menu_item_t g_items_system[] = {
  {"Display", "", &g_page_display},
  {"Comm", "", &g_page_comm},
  {"Info", "", NULL},
};

static const app_menu_item_t g_items_mode[] = {
  {"Current", "", NULL},
  {"Speed", "", NULL},
  {"Position", "", NULL},
};

static const app_menu_item_t g_items_limits[] = {
  {"I Max", "--", NULL},
  {"RPM Max", "--", NULL},
  {"Temp", "--", NULL},
};

static const app_menu_item_t g_items_display[] = {
  {"Theme", "Mono", NULL},
  {"Bright", "Mid", NULL},
  {"Layout", "Tree", NULL},
};

static const app_menu_item_t g_items_comm[] = {
  {"UART", "ON", NULL},
  {"Rate", "115K", NULL},
  {"Node", "01", NULL},
};

static const app_menu_page_t g_page_root = {"HOME",
                                            g_items_root,
                                            (uint8_t)(sizeof(g_items_root) /
                                                      sizeof(g_items_root[0]))};
static const app_menu_page_t g_page_control = {
    "CTRL",
    g_items_control,
    (uint8_t)(sizeof(g_items_control) / sizeof(g_items_control[0]))};
static const app_menu_page_t g_page_monitor = {
    "MON",
    g_items_monitor,
    (uint8_t)(sizeof(g_items_monitor) / sizeof(g_items_monitor[0]))};
static const app_menu_page_t g_page_system = {
    "SYS",
    g_items_system,
    (uint8_t)(sizeof(g_items_system) / sizeof(g_items_system[0]))};
static const app_menu_page_t g_page_mode = {"MODE",
                                            g_items_mode,
                                            (uint8_t)(sizeof(g_items_mode) /
                                                      sizeof(g_items_mode[0]))};
static const app_menu_page_t g_page_limits = {
    "LIM",
    g_items_limits,
    (uint8_t)(sizeof(g_items_limits) / sizeof(g_items_limits[0]))};
static const app_menu_page_t g_page_display = {
    "DSP",
    g_items_display,
    (uint8_t)(sizeof(g_items_display) / sizeof(g_items_display[0]))};
static const app_menu_page_t g_page_comm = {"COMM",
                                            g_items_comm,
                                            (uint8_t)(sizeof(g_items_comm) /
                                                      sizeof(g_items_comm[0]))};

static app_menu_state_t g_menu_state;

static void appMenuSyncWindow(void) {
  uint8_t level;
  const app_menu_page_t *page;
  uint8_t selected;
  uint8_t top;

  if (g_menu_state.depth == 0U) {
    return;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];
  selected = g_menu_state.selected[level];
  top = g_menu_state.first_visible[level];

  if (selected < top) {
    top = selected;
  } else if (selected >= (uint8_t)(top + HAL_OLED_MENU_ROWS)) {
    top = (uint8_t)(selected - (HAL_OLED_MENU_ROWS - 1U));
  }

  if (page->item_count <= HAL_OLED_MENU_ROWS) {
    top = 0U;
  }

  g_menu_state.first_visible[level] = top;
}

static const app_menu_item_t *appMenuGetSelectedItem(void) {
  uint8_t level;
  const app_menu_page_t *page;
  uint8_t selected;

  if (g_menu_state.depth == 0U) {
    return NULL;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];
  selected = g_menu_state.selected[level];

  if (selected >= page->item_count) {
    return NULL;
  }

  return &page->items[selected];
}

void appMenuInit(void) {
  uint8_t i;

  g_menu_state.depth = 1U;
  g_menu_state.stack[0] = &g_page_root;

  for (i = 0U; i < APP_MENU_STACK_DEPTH; i++) {
    g_menu_state.selected[i] = 0U;
    g_menu_state.first_visible[i] = 0U;
  }
}

void appMenuMoveNext(void) {
  uint8_t level;
  const app_menu_page_t *page;

  if (g_menu_state.depth == 0U) {
    return;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];

  if ((g_menu_state.selected[level] + 1U) < page->item_count) {
    g_menu_state.selected[level]++;
  } else {
    g_menu_state.selected[level] = 0U;
  }

  appMenuSyncWindow();
}

void appMenuMovePrev(void) {
  uint8_t level;
  const app_menu_page_t *page;

  if (g_menu_state.depth == 0U) {
    return;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];

  if (g_menu_state.selected[level] > 0U) {
    g_menu_state.selected[level]--;
  } else {
    g_menu_state.selected[level] = (uint8_t)(page->item_count - 1U);
  }

  appMenuSyncWindow();
}

void appMenuEnter(void) {
  const app_menu_item_t *item;
  uint8_t next_level;

  item = appMenuGetSelectedItem();
  if ((item == NULL) || (item->target == NULL)) {
    return;
  }

  if (g_menu_state.depth >= APP_MENU_STACK_DEPTH) {
    return;
  }

  next_level = g_menu_state.depth;
  g_menu_state.stack[next_level] = item->target;
  g_menu_state.selected[next_level] = 0U;
  g_menu_state.first_visible[next_level] = 0U;
  g_menu_state.depth++;
}

void appMenuBack(void) {

  if (g_menu_state.depth > 1U) {
    g_menu_state.depth--;
  }
}

void appMenuDemoStep(void) {
  const app_menu_item_t *item = appMenuGetSelectedItem();

  if ((item != NULL) && (item->target != NULL)) {
    appMenuEnter();
    return;
  }

  if (g_menu_state.depth > 1U) {
    appMenuBack();
    appMenuMoveNext();
    return;
  }

  appMenuMoveNext();
}

void appMenuBuildView(hal_oled_menu_view_t *view) {
  uint8_t level;
  const app_menu_page_t *page;
  uint8_t row;
  uint8_t first_visible;
  uint8_t selected;

  if ((view == NULL) || (g_menu_state.depth == 0U)) {
    return;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];
  first_visible = g_menu_state.first_visible[level];
  selected = g_menu_state.selected[level];

  view->title = page->title;
  view->selected_row = (uint8_t)(selected - first_visible);
  view->level = g_menu_state.depth;
  view->has_parent = g_menu_state.depth > 1U ? 1U : 0U;
  view->first_visible_row = first_visible;
  view->total_rows = page->item_count;

  for (row = 0U; row < HAL_OLED_MENU_ROWS; row++) {
    uint8_t item_index = (uint8_t)(first_visible + row);

    view->rows[row].label = "";
    view->rows[row].value = "";
    view->rows[row].kind = HAL_OLED_MENU_ROW_EMPTY;

    if (item_index < page->item_count) {
      const app_menu_item_t *item = &page->items[item_index];

      view->rows[row].label = item->label;
      view->rows[row].value = item->value;
      view->rows[row].kind = item->target != NULL ? HAL_OLED_MENU_ROW_SUBMENU
                                                  : HAL_OLED_MENU_ROW_VALUE;
    }
  }
}

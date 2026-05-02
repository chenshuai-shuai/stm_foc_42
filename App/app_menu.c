#include "app_menu.h"

#include <stddef.h>
#include <stdio.h>

typedef struct app_menu_page app_menu_page_t;
typedef enum {
  APP_MENU_ACTION_NONE = 0,
  APP_MENU_ACTION_TOGGLE_RUN
} app_menu_action_t;
typedef enum {
  APP_MENU_VALUE_STATIC = 0,
  APP_MENU_VALUE_RUN_STATE,
  APP_MENU_VALUE_MON_MECH_ANGLE,
  APP_MENU_VALUE_MON_ELEC_ANGLE,
  APP_MENU_VALUE_MON_VBUS,
  APP_MENU_VALUE_MON_TURN,
  APP_MENU_VALUE_MON_RAW
} app_menu_value_id_t;

typedef struct {
  const char *label;
  const char *value;
  const app_menu_page_t *target;
  app_menu_action_t action;
  app_menu_value_id_t value_id;
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
    {"Control", "", &g_page_control, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Monitor", "", &g_page_monitor, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"System", "", &g_page_system, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Debug", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_control[] = {
    {"Enable", "", NULL, APP_MENU_ACTION_TOGGLE_RUN, APP_MENU_VALUE_RUN_STATE},
    {"Mode", "", &g_page_mode, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Limits", "", &g_page_limits, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Assist", "Demo", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_monitor[] = {
    {"Mech", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_MON_MECH_ANGLE},
    {"Elec", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_MON_ELEC_ANGLE},
    {"Vbus", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_MON_VBUS},
    {"Turn", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_MON_TURN},
    {"Raw", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_MON_RAW},
};

static const app_menu_item_t g_items_system[] = {
    {"Display", "", &g_page_display, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Comm", "", &g_page_comm, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Info", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_mode[] = {
    {"Current", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Speed", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Position", "", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_limits[] = {
    {"I Max", "--", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"RPM Max", "--", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Temp", "--", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_display[] = {
    {"Theme", "Mono", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Bright", "Mid", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Layout", "Tree", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
};

static const app_menu_item_t g_items_comm[] = {
    {"UART", "ON", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Rate", "115K", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
    {"Node", "01", NULL, APP_MENU_ACTION_NONE, APP_MENU_VALUE_STATIC},
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
static char g_value_buffer[HAL_OLED_MENU_ROWS][12];

static const char *appMenuResolveValue(const app_menu_item_t *item,
                                       const app_command_snapshot_t *command_snapshot,
                                       const app_runtime_t *runtime,
                                       uint8_t row_index) {

  if (item == NULL) {
    return "";
  }

  if (row_index >= HAL_OLED_MENU_ROWS) {
    return "";
  }

  switch (item->value_id) {
  case APP_MENU_VALUE_RUN_STATE:
    if ((command_snapshot != NULL) && command_snapshot->value.run_request) {
      return "ON";
    }
    return "OFF";

  case APP_MENU_VALUE_MON_MECH_ANGLE:
    if ((runtime != NULL) && (runtime->encoder_ready != 0U)) {
      (void)snprintf(g_value_buffer[row_index],
                     sizeof(g_value_buffer[row_index]),
                     "%3u.%u",
                     (unsigned)(runtime->mechanical_angle_decideg / 10U),
                     (unsigned)(runtime->mechanical_angle_decideg % 10U));
      return g_value_buffer[row_index];
    }
    return "--.-";

  case APP_MENU_VALUE_MON_ELEC_ANGLE:
    if ((runtime != NULL) && (runtime->encoder_ready != 0U)) {
      (void)snprintf(g_value_buffer[row_index],
                     sizeof(g_value_buffer[row_index]),
                     "%3u.%u",
                     (unsigned)(runtime->electrical_angle_decideg / 10U),
                     (unsigned)(runtime->electrical_angle_decideg % 10U));
      return g_value_buffer[row_index];
    }
    return "--.-";

  case APP_MENU_VALUE_MON_VBUS:
    if (runtime != NULL) {
      (void)snprintf(g_value_buffer[row_index],
                     sizeof(g_value_buffer[row_index]),
                     "%2u.%1uV",
                     (unsigned)(runtime->bus_voltage_mv / 1000U),
                     (unsigned)((runtime->bus_voltage_mv % 1000U) / 100U));
      return g_value_buffer[row_index];
    }
    return "--.-V";

  case APP_MENU_VALUE_MON_TURN:
    if ((runtime != NULL) && (runtime->encoder_ready != 0U)) {
      (void)snprintf(g_value_buffer[row_index],
                     sizeof(g_value_buffer[row_index]),
                     "%ld",
                     (long)runtime->mechanical_turn_count);
      return g_value_buffer[row_index];
    }
    return "--";

  case APP_MENU_VALUE_MON_RAW:
    if ((runtime != NULL) && (runtime->encoder_ready != 0U)) {
      (void)snprintf(g_value_buffer[row_index],
                     sizeof(g_value_buffer[row_index]),
                     "%05u",
                     (unsigned)runtime->encoder_raw);
      return g_value_buffer[row_index];
    }
    return "-----";

  case APP_MENU_VALUE_STATIC:
  default:
    if (item->value != NULL) {
      return item->value;
    }

    return "";
  }
}

uint8_t appMenuNeedsPeriodicRefresh(void) {
  uint8_t level;
  const app_menu_page_t *page;
  uint8_t row;

  if (g_menu_state.depth == 0U) {
    return 0U;
  }

  level = (uint8_t)(g_menu_state.depth - 1U);
  page = g_menu_state.stack[level];
  if (page == NULL) {
    return 0U;
  }

  for (row = 0U; row < page->item_count; row++) {
    switch (page->items[row].value_id) {
    case APP_MENU_VALUE_MON_MECH_ANGLE:
    case APP_MENU_VALUE_MON_ELEC_ANGLE:
    case APP_MENU_VALUE_MON_VBUS:
    case APP_MENU_VALUE_MON_TURN:
    case APP_MENU_VALUE_MON_RAW:
      return 1U;

    case APP_MENU_VALUE_STATIC:
    case APP_MENU_VALUE_RUN_STATE:
    default:
      break;
    }
  }

  return 0U;
}

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

app_menu_enter_result_t appMenuActivate(const app_command_t *current_command,
                                        app_command_t *next_command) {
  const app_menu_item_t *item;

  item = appMenuGetSelectedItem();
  if ((item == NULL) || (current_command == NULL) || (next_command == NULL)) {
    return APP_MENU_ENTER_RESULT_NONE;
  }

  *next_command = *current_command;

  switch (item->action) {
  case APP_MENU_ACTION_TOGGLE_RUN:
    next_command->run_request = !current_command->run_request;
    return APP_MENU_ENTER_RESULT_COMMAND_UPDATED;

  case APP_MENU_ACTION_NONE:
  default:
    return APP_MENU_ENTER_RESULT_NONE;
  }
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

void appMenuBuildView(const app_command_snapshot_t *command_snapshot,
                      const app_runtime_t *runtime,
                      hal_oled_menu_view_t *view) {
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
      view->rows[row].value =
          appMenuResolveValue(item, command_snapshot, runtime, row);
      view->rows[row].kind = item->target != NULL ? HAL_OLED_MENU_ROW_SUBMENU
                                                  : HAL_OLED_MENU_ROW_VALUE;
    }
  }
}

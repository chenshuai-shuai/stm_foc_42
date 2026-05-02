#include "drv_oled_u8g2_display.h"

#include "OLED_Config.h"

static const uint8_t drvOledPowersaveOnSeq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x0af),
    U8X8_END_TRANSFER(),
    U8X8_END()};

static const uint8_t drvOledPowersaveOffSeq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x0ae),
    U8X8_END_TRANSFER(),
    U8X8_END()};

static const uint8_t drvOledFlip0Seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(OLED_CMD_SEG_REMAP),
    U8X8_C(OLED_CMD_COM_SCAN_DIR),
    U8X8_END_TRANSFER(),
    U8X8_END()};

static const uint8_t drvOledFlip1Seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x0a0),
    U8X8_C(0x0c0),
    U8X8_END_TRANSFER(),
    U8X8_END()};

static const u8x8_display_info_t drvOledDisplayInfo = {
    0U,
    1U,
    20U,
    10U,
    100U,
    100U,
    50U,
    50U,
    8000000UL,
    0U,
    4U,
    40U,
    150U,
    8U,
    4U,
    OLED_COLUMN_OFFSET,
    OLED_COLUMN_OFFSET,
    OLED_WIDTH,
    OLED_HEIGHT};

static const uint8_t drvOledInitSeq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x0ae),
    U8X8_CA(0x0d5, OLED_CMD_DISPLAY_CLOCK_DIV),
    U8X8_CA(0x0a8, OLED_CMD_MULTIPLEX_RATIO),
    U8X8_CA(0x0d3, OLED_CMD_DISPLAY_OFFSET),
    U8X8_C(OLED_CMD_DISPLAY_START_LINE),
    U8X8_C(OLED_CMD_SEG_REMAP),
    U8X8_C(OLED_CMD_COM_SCAN_DIR),
    U8X8_CA(0x0da, OLED_CMD_COM_PINS_CONFIG),
    U8X8_CA(0x081, OLED_CMD_CONTRAST),
    U8X8_CA(0x0d9, OLED_CMD_PRECHARGE_PERIOD),
    U8X8_CA(0x0db, OLED_CMD_VCOMH_LEVEL),
    U8X8_C(0x0a4),
    U8X8_C(0x0a6),
    U8X8_CA(0x08d, OLED_CMD_CHARGE_PUMP),
    U8X8_END_TRANSFER(),
    U8X8_END()};

static uint8_t drvOledDrawTiles(u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr) {

  uint8_t x;
  uint8_t count;
  uint8_t *ptr;

  u8x8_cad_StartTransfer(u8x8);
  x = ((u8x8_tile_t *)arg_ptr)->x_pos;
  x = (uint8_t)(x * 8U);
  x = (uint8_t)(x + u8x8->x_offset);

  u8x8_cad_SendCmd(u8x8, (uint8_t)(0x010U | (x >> 4)));
  u8x8_cad_SendCmd(u8x8, (uint8_t)(0x000U | (x & 0x0fU)));
  u8x8_cad_SendCmd(u8x8, (uint8_t)(0x0b0U | ((u8x8_tile_t *)arg_ptr)->y_pos));

  do {
    count = ((u8x8_tile_t *)arg_ptr)->cnt;
    ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
    u8x8_cad_SendData(u8x8, (uint8_t)(count * 8U), ptr);
    arg_int--;
  } while (arg_int > 0U);

  u8x8_cad_EndTransfer(u8x8);
  return 1U;
}

uint8_t drvOledU8g2Display(u8x8_t *u8x8,
                           uint8_t msg,
                           uint8_t arg_int,
                           void *arg_ptr) {

  switch (msg) {
  case U8X8_MSG_DISPLAY_SETUP_MEMORY:
    u8x8_d_helper_display_setup_memory(u8x8, &drvOledDisplayInfo);
    return 1U;

  case U8X8_MSG_DISPLAY_INIT:
    u8x8_d_helper_display_init(u8x8);
    u8x8_cad_SendSequence(u8x8, drvOledInitSeq);
    return 1U;

  case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
    u8x8_cad_SendSequence(u8x8,
                          (arg_int == 0U) ? drvOledPowersaveOnSeq
                                          : drvOledPowersaveOffSeq);
    return 1U;

  case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
    if (arg_int == 0U) {
      u8x8_cad_SendSequence(u8x8, drvOledFlip0Seq);
      u8x8->x_offset = u8x8->display_info->default_x_offset;
    } else {
      u8x8_cad_SendSequence(u8x8, drvOledFlip1Seq);
      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
    }
    return 1U;

  case U8X8_MSG_DISPLAY_SET_CONTRAST:
    u8x8_cad_StartTransfer(u8x8);
    u8x8_cad_SendCmd(u8x8, 0x081U);
    u8x8_cad_SendArg(u8x8, arg_int);
    u8x8_cad_EndTransfer(u8x8);
    return 1U;

  case U8X8_MSG_DISPLAY_DRAW_TILE:
    return drvOledDrawTiles(u8x8, arg_int, arg_ptr);

  default:
    return 0U;
  }
}

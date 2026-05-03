#ifndef DRV_FLASH_CFG_H
#define DRV_FLASH_CFG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t valid;
  int8_t encoder_direction;
  int8_t phase_order_sign;
  uint8_t pole_pairs;
  uint16_t encoder_zero_raw;
  uint16_t align_current_ma;
  uint16_t run_current_limit_ma;
  uint16_t reserved1;
  uint16_t step_lut_raw[200];
} drv_flash_cfg_payload_t;

typedef struct {
  uint32_t magic;
  uint16_t version;
  uint16_t length;
  uint32_t sequence;
  uint32_t crc32;
  drv_flash_cfg_payload_t payload;
} drv_flash_cfg_record_t;

bool drvFlashCfgLoad(drv_flash_cfg_record_t *record);
bool drvFlashCfgSave(const drv_flash_cfg_record_t *record);
void drvFlashCfgClear(void);

#endif /* DRV_FLASH_CFG_H */

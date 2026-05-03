#include "drv_flash_cfg.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "stm32f10x_flash.h"
#include "drv_usart1.h"

#define DRV_FLASH_CFG_MAGIC             0x434C4231UL
#define DRV_FLASH_CFG_VERSION           0x0001U
#define DRV_FLASH_CFG_PAGE0_BASE        0x0800F000UL
#define DRV_FLASH_CFG_PAGE1_BASE        0x0800F800UL
#define DRV_FLASH_CFG_PAGE_SIZE         0x0800UL

static void drvFlashCfgLog(const char *text) {
  if (drvUsart1IsReady()) {
    drvUsart1Write(text);
  }
}

static void drvFlashCfgLogStatus(const char *prefix,
                                 uint32_t address,
                                 uint32_t value) {
  char tx_buffer[128];

  if (!drvUsart1IsReady()) {
    return;
  }

  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "%s0x%08lX val=%lu\r\n",
                 prefix,
                 (unsigned long)address,
                 (unsigned long)value);
  drvUsart1Write(tx_buffer);
}

static uint32_t drvFlashCfgCrc32(const uint8_t *data, uint32_t length) {
  uint32_t crc;
  uint32_t index;
  uint8_t bit_index;

  crc = 0xFFFFFFFFUL;
  for (index = 0U; index < length; index++) {
    crc ^= (uint32_t)data[index];
    for (bit_index = 0U; bit_index < 8U; bit_index++) {
      if ((crc & 1UL) != 0UL) {
        crc = (crc >> 1U) ^ 0xEDB88320UL;
      } else {
        crc >>= 1U;
      }
    }
  }

  return ~crc;
}

static uint32_t drvFlashCfgGetRecordCrc(const drv_flash_cfg_record_t *record) {
  drv_flash_cfg_record_t temp;

  temp = *record;
  temp.crc32 = 0UL;
  return drvFlashCfgCrc32((const uint8_t *)&temp, sizeof(temp));
}

static bool drvFlashCfgIsRecordValidAt(uint32_t address,
                                       drv_flash_cfg_record_t *record) {
  const drv_flash_cfg_record_t *flash_record;

  flash_record = (const drv_flash_cfg_record_t *)address;
  if ((flash_record->magic != DRV_FLASH_CFG_MAGIC) ||
      (flash_record->version != DRV_FLASH_CFG_VERSION) ||
      (flash_record->length != (uint16_t)sizeof(drv_flash_cfg_record_t))) {
    return false;
  }

  if (drvFlashCfgGetRecordCrc(flash_record) != flash_record->crc32) {
    return false;
  }

  if (record != NULL) {
    *record = *flash_record;
  }

  return true;
}

static bool drvFlashCfgProgramRecord(uint32_t base_address,
                                     const drv_flash_cfg_record_t *record) {
  uint32_t offset;
  const uint16_t *data16;
  FLASH_Status status;

  drvFlashCfgLogStatus("[FLASH] erase begin page=", base_address, 0U);
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
  status = FLASH_ErasePage(base_address);
  if (status != FLASH_COMPLETE) {
    drvFlashCfgLogStatus("[FLASH] erase fail page=", base_address, (uint32_t)status);
    FLASH_Lock();
    return false;
  }
  drvFlashCfgLogStatus("[FLASH] erase ok page=", base_address, 0U);

  data16 = (const uint16_t *)record;
  for (offset = 0U; offset < sizeof(*record); offset += 2U) {
    status = FLASH_ProgramHalfWord(base_address + offset, data16[offset / 2U]);
    if (status != FLASH_COMPLETE) {
      drvFlashCfgLogStatus("[FLASH] prog fail addr=", base_address + offset, (uint32_t)status);
      FLASH_Lock();
      return false;
    }
  }

  FLASH_Lock();
  drvFlashCfgLogStatus("[FLASH] prog ok page=", base_address, (uint32_t)record->sequence);
  return true;
}

bool drvFlashCfgLoad(drv_flash_cfg_record_t *record) {
  drv_flash_cfg_record_t record0;
  drv_flash_cfg_record_t record1;
  bool valid0;
  bool valid1;

  valid0 = drvFlashCfgIsRecordValidAt(DRV_FLASH_CFG_PAGE0_BASE, &record0);
  valid1 = drvFlashCfgIsRecordValidAt(DRV_FLASH_CFG_PAGE1_BASE, &record1);

  if (!valid0 && !valid1) {
    drvFlashCfgLog("[FLASH] load none\r\n");
    return false;
  }

  if (!valid1 || (valid0 && (record0.sequence >= record1.sequence))) {
    if (record != NULL) {
      *record = record0;
    }
  } else {
    if (record != NULL) {
      *record = record1;
    }
  }

  return true;
}

bool drvFlashCfgSave(const drv_flash_cfg_record_t *record) {
  drv_flash_cfg_record_t next;
  uint32_t target_base;

  if (record == NULL) {
    return false;
  }

  next = *record;
  next.magic = DRV_FLASH_CFG_MAGIC;
  next.version = DRV_FLASH_CFG_VERSION;
  next.length = (uint16_t)sizeof(drv_flash_cfg_record_t);
  next.sequence = 1UL;

  if (drvFlashCfgLoad(NULL)) {
    const drv_flash_cfg_record_t *page0_record;
    const drv_flash_cfg_record_t *page1_record;
    bool page0_valid;
    bool page1_valid;

    page0_record = (const drv_flash_cfg_record_t *)DRV_FLASH_CFG_PAGE0_BASE;
    page1_record = (const drv_flash_cfg_record_t *)DRV_FLASH_CFG_PAGE1_BASE;
    page0_valid = drvFlashCfgIsRecordValidAt(DRV_FLASH_CFG_PAGE0_BASE, NULL);
    page1_valid = drvFlashCfgIsRecordValidAt(DRV_FLASH_CFG_PAGE1_BASE, NULL);

    if (page0_valid && (!page1_valid || (page0_record->sequence >= page1_record->sequence))) {
      next.sequence = page0_record->sequence + 1UL;
      target_base = DRV_FLASH_CFG_PAGE1_BASE;
    } else if (page1_valid) {
      next.sequence = page1_record->sequence + 1UL;
      target_base = DRV_FLASH_CFG_PAGE0_BASE;
    } else {
      next.sequence = 1UL;
      target_base = DRV_FLASH_CFG_PAGE0_BASE;
    }
  } else {
    target_base = DRV_FLASH_CFG_PAGE0_BASE;
  }

  next.crc32 = drvFlashCfgGetRecordCrc(&next);
  drvFlashCfgLogStatus("[FLASH] save target=", target_base, next.sequence);
  return drvFlashCfgProgramRecord(target_base, &next);
}

void drvFlashCfgClear(void) {
  drvFlashCfgLog("[FLASH] clear begin\r\n");
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
  (void)FLASH_ErasePage(DRV_FLASH_CFG_PAGE0_BASE);
  (void)FLASH_ErasePage(DRV_FLASH_CFG_PAGE1_BASE);
  FLASH_Lock();
  drvFlashCfgLog("[FLASH] clear end\r\n");
}

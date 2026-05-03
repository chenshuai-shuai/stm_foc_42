#include "app_calib.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "algo_foc.h"
#include "algo_limit.h"
#include "ch.h"
#include "drv_flash_cfg.h"
#include "hal_encoder_if.h"
#include "hal_uart_if.h"
#define APP_CALIB_DEFAULT_POLE_PAIRS                 50U
#define APP_CALIB_DEFAULT_ALIGN_CURRENT_MA           500U
#define APP_CALIB_DEFAULT_RUN_CURRENT_LIMIT_MA       900U
#define APP_CALIB_FULL_STEPS_PER_REV                 200U
#define APP_CALIB_MICROSTEPS_PER_FULL_STEP           256U
#define APP_CALIB_MICROSTEPS_PER_ELECTRICAL_CYCLE    1024U
#define APP_CALIB_ALIGN_SETTLE_MS                    300U
#define APP_CALIB_FORWARD_REV_MS                     4000U
#define APP_CALIB_REVERSE_REV_MS                     4000U

typedef enum {
  APP_CALIB_STAGE_IDLE = 0,
  APP_CALIB_STAGE_ALIGN_SETTLE,
  APP_CALIB_STAGE_FORWARD_REV,
  APP_CALIB_STAGE_REVERSE_REV
} app_calib_stage_t;

static app_calib_status_t g_calib_status;
static app_calib_stage_t g_calib_stage;
static uint32_t g_calib_stage_tick;
static int32_t g_calib_start_mech_decideg;
static uint16_t g_calib_last_sample_index;
static uint16_t g_calib_step_lut_raw[200];
static bool g_calib_start_requested;
static bool g_calib_load_requested;
static bool g_calib_clear_requested;
static bool g_calib_autosave_pending;

static void appCalibLogU32(const char *prefix, uint32_t value) {
  char tx_buffer[96];

  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "%s%lu\r\n",
                 prefix,
                 (unsigned long)value);
  halUartWrite(tx_buffer);
}

static void appCalibLogS32(const char *prefix, int32_t value) {
  char tx_buffer[96];

  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "%s%ld\r\n",
                 prefix,
                 (long)value);
  halUartWrite(tx_buffer);
}

static int32_t appCalibGetTotalMechanicalDecideg(const motor_feedback_t *feedback) {
  if (feedback == NULL) {
    return 0;
  }

  return (int32_t)(((int64_t)feedback->mechanical_turn_count * 3600LL) +
                   (int64_t)feedback->mechanical_angle_decideg);
}

static void appCalibApplyToAlgo(void) {
  algo_foc_calibration_t calib;

  calib.valid = g_calib_status.params.valid;
  calib.encoder_direction = g_calib_status.params.encoder_direction;
  calib.phase_order_sign = g_calib_status.params.phase_order_sign;
  calib.pole_pairs = g_calib_status.params.pole_pairs;
  calib.encoder_zero_raw = g_calib_status.params.encoder_zero_raw;
  algoFocApplyCalibration(&calib);
  halEncoderResetTracking();
}

static void appCalibLoadDefaults(void) {
  memset(&g_calib_status, 0, sizeof(g_calib_status));
  g_calib_status.state = APP_CALIB_STATE_IDLE;
  g_calib_status.params.valid = 0U;
  g_calib_status.params.encoder_direction = 1;
  g_calib_status.params.phase_order_sign = 1;
  g_calib_status.params.pole_pairs = APP_CALIB_DEFAULT_POLE_PAIRS;
  g_calib_status.params.encoder_zero_raw = 0U;
  g_calib_status.params.align_current_ma = APP_CALIB_DEFAULT_ALIGN_CURRENT_MA;
  g_calib_status.params.run_current_limit_ma = APP_CALIB_DEFAULT_RUN_CURRENT_LIMIT_MA;
  memset(g_calib_step_lut_raw, 0xFF, sizeof(g_calib_step_lut_raw));
}

static void appCalibComposeOutput(int32_t electrical_angle_decideg,
                                  uint16_t current_ma,
                                  motor_control_output_t *output) {
  float theta_rad;
  float sin_theta;
  float cos_theta;
  int32_t phase_a_current_ma;
  int32_t phase_b_current_ma;
  uint32_t phase_a_magnitude_ma;
  uint32_t phase_b_magnitude_ma;

  if (output == NULL) {
    return;
  }

  electrical_angle_decideg %= 3600L;
  if (electrical_angle_decideg < 0) {
    electrical_angle_decideg += 3600L;
  }

  theta_rad = ((float)electrical_angle_decideg * 3.1415926f) / 1800.0f;
  sin_theta = sinf(theta_rad);
  cos_theta = cosf(theta_rad);

  phase_a_current_ma = (int32_t)((float)current_ma * sin_theta);
  phase_b_current_ma = (int32_t)((float)current_ma * cos_theta);
  phase_a_magnitude_ma =
      (uint32_t)((phase_a_current_ma >= 0) ? phase_a_current_ma : -phase_a_current_ma);
  phase_b_magnitude_ma =
      (uint32_t)((phase_b_current_ma >= 0) ? phase_b_current_ma : -phase_b_current_ma);

  output->duty_a_permille =
      algoLimitU16((uint16_t)((phase_a_magnitude_ma * 1000U) / 3300U), 0U, 950U);
  output->duty_b_permille =
      algoLimitU16((uint16_t)((phase_b_magnitude_ma * 1000U) / 3300U), 0U, 950U);
  output->phase_a_forward = (bool)(phase_a_current_ma >= 0);
  output->phase_b_forward = (bool)(phase_b_current_ma >= 0);
  output->enable = true;
}

static void appCalibComposeFromMechanicalMicrostep(uint32_t microstep_index,
                                                   uint16_t current_ma,
                                                   motor_control_output_t *output) {
  uint32_t electrical_microstep_index;
  int32_t electrical_angle_decideg;

  electrical_microstep_index =
      microstep_index % APP_CALIB_MICROSTEPS_PER_ELECTRICAL_CYCLE;
  electrical_angle_decideg =
      (int32_t)((electrical_microstep_index * 3600UL) /
                APP_CALIB_MICROSTEPS_PER_ELECTRICAL_CYCLE);
  appCalibComposeOutput(electrical_angle_decideg, current_ma, output);
}

static bool appCalibAutoSave(void) {
  drv_flash_cfg_record_t record;

  memset(&record, 0, sizeof(record));
  record.payload.valid = g_calib_status.params.valid;
  record.payload.encoder_direction = g_calib_status.params.encoder_direction;
  record.payload.phase_order_sign = g_calib_status.params.phase_order_sign;
  record.payload.pole_pairs = g_calib_status.params.pole_pairs;
  record.payload.encoder_zero_raw = g_calib_status.params.encoder_zero_raw;
  record.payload.align_current_ma = g_calib_status.params.align_current_ma;
  record.payload.run_current_limit_ma = g_calib_status.params.run_current_limit_ma;
  memcpy(record.payload.step_lut_raw,
         g_calib_step_lut_raw,
         sizeof(record.payload.step_lut_raw));

  if (!drvFlashCfgSave(&record)) {
    return false;
  }

  if (drvFlashCfgLoad(&record)) {
    g_calib_status.sequence = record.sequence;
  }

  return true;
}

static void appCalibFinish(app_calib_state_t state) {
  char tx_buffer[64];

  g_calib_status.state = state;
  g_calib_stage = APP_CALIB_STAGE_IDLE;
  g_calib_stage_tick = 0U;
  g_calib_last_sample_index = 0U;
  (void)snprintf(tx_buffer,
                 sizeof(tx_buffer),
                 "[CAL] finish state=%u\r\n",
                 (unsigned)state);
  halUartWrite(tx_buffer);
}

static void appCalibStartInternal(const motor_feedback_t *feedback) {
  if ((feedback == NULL) || !feedback->encoder_ready) {
    appCalibFinish(APP_CALIB_STATE_FAILED);
    halUartWrite("[CAL] start failed: encoder not ready\r\n");
    return;
  }

  g_calib_status.state = APP_CALIB_STATE_RUNNING;
  g_calib_stage = APP_CALIB_STAGE_ALIGN_SETTLE;
  g_calib_stage_tick = 0U;
  g_calib_start_mech_decideg = appCalibGetTotalMechanicalDecideg(feedback);
  g_calib_last_sample_index = 0U;
  memset(g_calib_step_lut_raw, 0xFF, sizeof(g_calib_step_lut_raw));
  g_calib_step_lut_raw[0] = feedback->encoder_raw;
  halUartWrite("[CAL] revolution calibration started\r\n");
  appCalibLogU32("[CAL] start raw0=", feedback->encoder_raw);
}

static void appCalibHandleLoad(void) {
  drv_flash_cfg_record_t record;

  if (!drvFlashCfgLoad(&record)) {
    g_calib_status.state = APP_CALIB_STATE_FAILED;
    halUartWrite("[CAL] load failed\r\n");
    return;
  }

  g_calib_status.params.valid = record.payload.valid;
  g_calib_status.params.encoder_direction = record.payload.encoder_direction;
  g_calib_status.params.phase_order_sign = record.payload.phase_order_sign;
  g_calib_status.params.pole_pairs = record.payload.pole_pairs;
  g_calib_status.params.encoder_zero_raw = record.payload.encoder_zero_raw;
  g_calib_status.params.align_current_ma = record.payload.align_current_ma;
  g_calib_status.params.run_current_limit_ma = record.payload.run_current_limit_ma;
  memcpy(g_calib_step_lut_raw,
         record.payload.step_lut_raw,
         sizeof(g_calib_step_lut_raw));
  g_calib_status.sequence = record.sequence;
  g_calib_status.state = APP_CALIB_STATE_IDLE;
  appCalibApplyToAlgo();
  halUartWrite("[CAL] params loaded\r\n");
  appCalibLogU32("[CAL] load seq=", g_calib_status.sequence);
}

static void appCalibHandleClear(void) {
  drvFlashCfgClear();
  appCalibLoadDefaults();
  appCalibApplyToAlgo();
  halUartWrite("[CAL] params cleared\r\n");
}

void appCalibInit(void) {
  drv_flash_cfg_record_t record;

  appCalibLoadDefaults();
  g_calib_stage = APP_CALIB_STAGE_IDLE;
  g_calib_stage_tick = 0U;
  g_calib_start_requested = false;
  g_calib_load_requested = false;
  g_calib_clear_requested = false;
  g_calib_autosave_pending = false;

  if (drvFlashCfgLoad(&record)) {
    g_calib_status.params.valid = record.payload.valid;
    g_calib_status.params.encoder_direction = record.payload.encoder_direction;
    g_calib_status.params.phase_order_sign = record.payload.phase_order_sign;
    g_calib_status.params.pole_pairs = record.payload.pole_pairs;
    g_calib_status.params.encoder_zero_raw = record.payload.encoder_zero_raw;
    g_calib_status.params.align_current_ma = record.payload.align_current_ma;
    g_calib_status.params.run_current_limit_ma = record.payload.run_current_limit_ma;
    memcpy(g_calib_step_lut_raw,
           record.payload.step_lut_raw,
           sizeof(g_calib_step_lut_raw));
    g_calib_status.sequence = record.sequence;
    halUartWrite("[CAL] params restored from flash\r\n");
    appCalibLogU32("[CAL] restored seq=", g_calib_status.sequence);
    appCalibLogU32("[CAL] restored zero=", g_calib_status.params.encoder_zero_raw);
    appCalibLogS32("[CAL] restored dir=",
                   (int32_t)g_calib_status.params.encoder_direction);
    appCalibLogS32("[CAL] restored phase=",
                   (int32_t)g_calib_status.params.phase_order_sign);
  } else {
    halUartWrite("[CAL] using default calibration params\r\n");
  }

  appCalibApplyToAlgo();
}

void appCalibService(void) {
  bool do_load;
  bool do_clear;
  bool do_autosave;

  chSysLock();
  do_load = g_calib_load_requested;
  do_clear = g_calib_clear_requested;
  do_autosave = g_calib_autosave_pending;
  g_calib_load_requested = false;
  g_calib_clear_requested = false;
  g_calib_autosave_pending = false;
  chSysUnlock();

  if (do_clear) {
    halUartWrite("[CAL] service clear begin\r\n");
    appCalibHandleClear();
    halUartWrite("[CAL] service clear end\r\n");
  }

  if (do_load) {
    halUartWrite("[CAL] service load begin\r\n");
    appCalibHandleLoad();
    halUartWrite("[CAL] service load end\r\n");
  }

  if (do_autosave) {
    halUartWrite("[CAL] service autosave begin\r\n");
    g_calib_status.state = APP_CALIB_STATE_SAVING;
    __disable_irq();
    if (!appCalibAutoSave()) {
      __enable_irq();
      g_calib_status.state = APP_CALIB_STATE_FAILED;
      halUartWrite("[CAL] auto save failed\r\n");
    } else {
      __enable_irq();
      g_calib_status.state = APP_CALIB_STATE_SUCCESS;
      halUartWrite("[CAL] auto saved\r\n");
      appCalibLogU32("[CAL] autosave seq=", g_calib_status.sequence);
    }
    halUartWrite("[CAL] service autosave end\r\n");
  }
}

bool appCalibProcess(const motor_feedback_t *feedback,
                     motor_control_output_t *output) {
  bool start_requested;
  int32_t delta_mech_decideg;

  chSysLock();
  start_requested = g_calib_start_requested;
  g_calib_start_requested = false;
  chSysUnlock();

  if ((g_calib_status.state != APP_CALIB_STATE_RUNNING) && start_requested) {
    appCalibStartInternal(feedback);
  }

  if (g_calib_status.state != APP_CALIB_STATE_RUNNING) {
    return false;
  }

  if ((feedback == NULL) || !feedback->encoder_ready || (output == NULL)) {
    appCalibFinish(APP_CALIB_STATE_FAILED);
    halUartWrite("[CAL] failed during run\r\n");
    return false;
  }

  switch (g_calib_stage) {
  case APP_CALIB_STAGE_ALIGN_SETTLE:
    appCalibComposeOutput(0L, g_calib_status.params.align_current_ma, output);
    g_calib_stage_tick++;
    if (g_calib_stage_tick >= APP_CALIB_ALIGN_SETTLE_MS) {
      g_calib_stage = APP_CALIB_STAGE_FORWARD_REV;
      g_calib_stage_tick = 0U;
      g_calib_start_mech_decideg = appCalibGetTotalMechanicalDecideg(feedback);
      halUartWrite("[CAL] stage forward rev\r\n");
    }
    return true;

  case APP_CALIB_STAGE_FORWARD_REV: {
    uint32_t commanded_microsteps;
    uint16_t sample_index;

    commanded_microsteps =
        (uint32_t)(((uint64_t)APP_CALIB_FULL_STEPS_PER_REV *
                    APP_CALIB_MICROSTEPS_PER_FULL_STEP *
                    (uint64_t)g_calib_stage_tick) /
                   (uint64_t)APP_CALIB_FORWARD_REV_MS);
    appCalibComposeFromMechanicalMicrostep(commanded_microsteps,
                                           g_calib_status.params.align_current_ma,
                                           output);
    sample_index =
        (uint16_t)(commanded_microsteps / APP_CALIB_MICROSTEPS_PER_FULL_STEP);
    while ((g_calib_last_sample_index < sample_index) &&
           (g_calib_last_sample_index < (APP_CALIB_FULL_STEPS_PER_REV - 1U))) {
      g_calib_last_sample_index++;
      g_calib_step_lut_raw[g_calib_last_sample_index] =
          feedback->encoder_raw;
    }
    g_calib_stage_tick++;
    if (g_calib_stage_tick >= APP_CALIB_FORWARD_REV_MS) {
      delta_mech_decideg =
          appCalibGetTotalMechanicalDecideg(feedback) - g_calib_start_mech_decideg;
      g_calib_status.params.encoder_direction = (delta_mech_decideg >= 0) ? 1 : -1;
      g_calib_status.params.phase_order_sign = (delta_mech_decideg >= 0) ? 1 : -1;
      appCalibLogU32("[CAL] delta mech=", (uint32_t)delta_mech_decideg);
      appCalibLogS32("[CAL] enc dir=", (int32_t)g_calib_status.params.encoder_direction);
      appCalibLogS32("[CAL] phase sign=", (int32_t)g_calib_status.params.phase_order_sign);
      g_calib_stage = APP_CALIB_STAGE_REVERSE_REV;
      g_calib_stage_tick = 0U;
      halUartWrite("[CAL] stage reverse rev\r\n");
    }
    return true;
  }

  case APP_CALIB_STAGE_REVERSE_REV: {
    uint32_t commanded_microsteps;

    commanded_microsteps =
        (uint32_t)(((uint64_t)APP_CALIB_FULL_STEPS_PER_REV *
                    APP_CALIB_MICROSTEPS_PER_FULL_STEP *
                    (uint64_t)(APP_CALIB_REVERSE_REV_MS - g_calib_stage_tick)) /
                   (uint64_t)APP_CALIB_REVERSE_REV_MS);
    appCalibComposeFromMechanicalMicrostep(commanded_microsteps,
                                           g_calib_status.params.align_current_ma,
                                           output);
    g_calib_stage_tick++;
    if (g_calib_stage_tick >= APP_CALIB_REVERSE_REV_MS) {
      g_calib_status.params.encoder_zero_raw = g_calib_step_lut_raw[0];
      g_calib_status.params.valid = 1U;
      appCalibApplyToAlgo();
      appCalibFinish(APP_CALIB_STATE_SUCCESS);
      chSysLock();
      g_calib_autosave_pending = true;
      chSysUnlock();
      halUartWrite("[CAL] revolution calibration success\r\n");
      appCalibLogU32("[CAL] zero raw=", g_calib_status.params.encoder_zero_raw);
      halUartWrite("[CAL] autosave queued\r\n");
      output->enable = false;
      return true;
    }
    return true;
  }

  case APP_CALIB_STAGE_IDLE:
  default:
    break;
  }

  return false;
}

void appCalibRequestStart(void) {
  chSysLock();
  g_calib_start_requested = true;
  chSysUnlock();
  halUartWrite("[CAL] request start\r\n");
}

void appCalibRequestLoad(void) {
  chSysLock();
  g_calib_load_requested = true;
  chSysUnlock();
  halUartWrite("[CAL] request load\r\n");
}

void appCalibRequestClear(void) {
  chSysLock();
  g_calib_clear_requested = true;
  chSysUnlock();
  halUartWrite("[CAL] request clear\r\n");
}

void appCalibGetStatus(app_calib_status_t *status) {
  if (status == NULL) {
    return;
  }

  chSysLock();
  *status = g_calib_status;
  chSysUnlock();
}

int16_t appCalibClampRunCurrent(int16_t requested_current_ma) {
  int16_t limit_ma;

  limit_ma = (int16_t)g_calib_status.params.run_current_limit_ma;
  if (limit_ma <= 0) {
    return requested_current_ma;
  }

  if (requested_current_ma > limit_ma) {
    return limit_ma;
  }

  if (requested_current_ma < 0) {
    return 0;
  }

  return requested_current_ma;
}

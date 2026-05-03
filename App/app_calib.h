#ifndef APP_CALIB_H
#define APP_CALIB_H

#include <stdbool.h>
#include <stdint.h>

#include "algo_motor_types.h"

typedef enum {
  APP_CALIB_STATE_IDLE = 0,
  APP_CALIB_STATE_RUNNING,
  APP_CALIB_STATE_SAVING,
  APP_CALIB_STATE_SUCCESS,
  APP_CALIB_STATE_FAILED
} app_calib_state_t;

typedef struct {
  uint8_t valid;
  int8_t encoder_direction;
  int8_t phase_order_sign;
  uint8_t pole_pairs;
  uint16_t encoder_zero_raw;
  uint16_t align_current_ma;
  uint16_t run_current_limit_ma;
} app_calib_params_t;

typedef struct {
  app_calib_state_t state;
  app_calib_params_t params;
  uint32_t sequence;
} app_calib_status_t;

void appCalibInit(void);
void appCalibService(void);
bool appCalibProcess(const motor_feedback_t *feedback,
                     motor_control_output_t *output);
void appCalibRequestStart(void);
void appCalibRequestLoad(void);
void appCalibRequestClear(void);
void appCalibGetStatus(app_calib_status_t *status);
int16_t appCalibClampRunCurrent(int16_t requested_current_ma);

#endif /* APP_CALIB_H */

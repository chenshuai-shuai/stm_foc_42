#ifndef ALGO_FOC_H
#define ALGO_FOC_H

#include "algo_motor_types.h"

typedef struct {
  uint8_t valid;
  int8_t encoder_direction;
  int8_t phase_order_sign;
  uint8_t pole_pairs;
  uint16_t encoder_zero_raw;
} algo_foc_calibration_t;

void algoFocInit(void);
void algoFocApplyCalibration(const algo_foc_calibration_t *calibration);
void algoFocStep(const motor_feedback_t *feedback,
                 const motor_command_t *command,
                 motor_control_output_t *output);
void algoFocGetDiagnostics(algo_foc_diag_t *diag);

#endif /* ALGO_FOC_H */

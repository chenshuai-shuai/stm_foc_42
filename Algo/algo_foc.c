#include "algo_foc.h"

#include <math.h>
#include <stddef.h>

#include "algo_angle.h"
#include "algo_limit.h"
#include "algo_ramp.h"

static const algo_angle_config_t g_default_angle_config = {
    16384U,
    50U,
    1,
    0U,
};
#define ALGO_STEPPER_FULL_STEPS_PER_REV             200L
#define ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP       256L
#define ALGO_STEPPER_MICROSTEPS_PER_ELECTRICAL_CYCLE 1024L
#define ALGO_CONTROL_PERIOD_MS                      1L
#define ALGO_MAX_PHASE_ADVANCE_DECIDEG              600L
#define ALGO_POSITION_KP_NUM                        1L
#define ALGO_POSITION_KP_DEN                        3L
#define ALGO_SPEED_DAMP_DECIDEG_PER_RPM             2L
#define ALGO_SPEED_RAMP_UPDATE_DIVIDER              5U

static algo_ramp_t g_current_ramp;
static algo_ramp_t g_speed_ramp;
static algo_foc_diag_t g_foc_diag;
static int64_t g_target_position_microstep_q16;
static int64_t g_last_actual_position_microstep_q16;
static uint16_t g_drive_electrical_angle_decideg;
static bool g_closed_loop_synced;
static uint8_t g_speed_ramp_divider;

static int32_t algoFocWrapAngle(int32_t angle_decideg) {

  while (angle_decideg < 0) {
    angle_decideg += 3600;
  }

  while (angle_decideg >= 3600) {
    angle_decideg -= 3600;
  }

  return angle_decideg;
}

static int64_t algoFocGetMechanicalPositionMicrostepQ16(
    const motor_feedback_t *feedback) {
  int64_t total_mechanical_decideg;

  if (feedback == NULL) {
    return 0;
  }

  total_mechanical_decideg =
      ((int64_t)feedback->mechanical_turn_count * 3600LL) +
      (int64_t)feedback->mechanical_angle_decideg;

  return (total_mechanical_decideg *
          ALGO_STEPPER_FULL_STEPS_PER_REV *
          ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
          65536LL) /
         3600LL;
}

static int32_t algoFocGetSpeedRpmFromPositionDelta(int64_t delta_microstep_q16) {

  return (int32_t)((delta_microstep_q16 * 60000LL) /
                   ((int64_t)ALGO_STEPPER_FULL_STEPS_PER_REV *
                    ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
                    65536LL *
                    ALGO_CONTROL_PERIOD_MS));
}

static int32_t algoFocGetElectricalAngleFromPositionError(int64_t error_microstep_q16) {

  return (int32_t)((error_microstep_q16 * 3600LL) /
                   ((int64_t)ALGO_STEPPER_MICROSTEPS_PER_ELECTRICAL_CYCLE *
                    65536LL));
}

void algoFocInit(void) {

  algoAngleInit(&g_default_angle_config);
  algoRampInit(&g_current_ramp, 0, 4, 6);
  algoRampInit(&g_speed_ramp, 0, 1, 2);
  g_foc_diag.electrical_angle_decideg = 0U;
  g_foc_diag.pole_pairs = g_default_angle_config.pole_pairs;
  g_foc_diag.encoder_zero_raw = g_default_angle_config.encoder_zero_raw;
  g_foc_diag.encoder_direction = g_default_angle_config.encoder_direction;
  g_target_position_microstep_q16 = 0;
  g_last_actual_position_microstep_q16 = 0;
  g_drive_electrical_angle_decideg = 0U;
  g_closed_loop_synced = false;
  g_speed_ramp_divider = 0U;
}

void algoFocStep(const motor_feedback_t *feedback,
                 const motor_command_t *command,
                 motor_control_output_t *output) {

  int32_t ramp_current_ma;
  int32_t target_speed_rpm;
  int64_t delta_microstep_q16;
  int64_t actual_position_microstep_q16;
  int64_t position_error_microstep_q16;
  int64_t actual_position_delta_microstep_q16;
  int32_t measured_speed_rpm;
  int32_t position_error_electrical_decideg;
  int32_t phase_advance_decideg;
  int32_t drive_electrical_angle_decideg;
  float theta_rad;
  float sin_theta;
  float cos_theta;
  int32_t phase_a_current_ma;
  int32_t phase_b_current_ma;
  uint32_t phase_a_magnitude_ma;
  uint32_t phase_b_magnitude_ma;

  if ((command == NULL) || (output == NULL) || (command->run == false)) {
    algoRampInit(&g_current_ramp, 0, 4, 6);
    algoRampInit(&g_speed_ramp, 0, 1, 2);
    g_foc_diag.electrical_angle_decideg = 0U;
    g_target_position_microstep_q16 = 0;
    g_last_actual_position_microstep_q16 = 0;
    g_drive_electrical_angle_decideg = 0U;
    g_closed_loop_synced = false;
    g_speed_ramp_divider = 0U;
    if (output != NULL) {
      output->duty_a_permille = 0U;
      output->duty_b_permille = 0U;
      output->phase_a_forward = true;
      output->phase_b_forward = true;
      output->enable = false;
    }
    return;
  }

  if (g_speed_ramp_divider == 0U) {
    target_speed_rpm =
        algoRampStep(&g_speed_ramp, algoLimitS32(command->target_speed_rpm, -60, 60));
  } else {
    target_speed_rpm = g_speed_ramp.value;
  }
  g_speed_ramp_divider++;
  if (g_speed_ramp_divider >= ALGO_SPEED_RAMP_UPDATE_DIVIDER) {
    g_speed_ramp_divider = 0U;
  }
  delta_microstep_q16 =
      ((int64_t)target_speed_rpm *
       ALGO_STEPPER_FULL_STEPS_PER_REV *
       ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
       65536LL *
       ALGO_CONTROL_PERIOD_MS) /
      60000LL;

  if ((feedback != NULL) && feedback->encoder_ready) {
    actual_position_microstep_q16 = algoFocGetMechanicalPositionMicrostepQ16(feedback);

    if (!g_closed_loop_synced) {
      g_target_position_microstep_q16 = actual_position_microstep_q16;
      g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
      g_closed_loop_synced = true;
    }

    g_target_position_microstep_q16 += delta_microstep_q16;
    actual_position_delta_microstep_q16 =
        actual_position_microstep_q16 - g_last_actual_position_microstep_q16;
    g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
    measured_speed_rpm =
        algoFocGetSpeedRpmFromPositionDelta(actual_position_delta_microstep_q16);
    position_error_microstep_q16 =
        g_target_position_microstep_q16 - actual_position_microstep_q16;
    position_error_electrical_decideg =
        algoFocGetElectricalAngleFromPositionError(position_error_microstep_q16);
    phase_advance_decideg =
        (position_error_electrical_decideg * ALGO_POSITION_KP_NUM) /
            ALGO_POSITION_KP_DEN -
        (measured_speed_rpm * ALGO_SPEED_DAMP_DECIDEG_PER_RPM);
    phase_advance_decideg =
        algoLimitS32(phase_advance_decideg,
                     -ALGO_MAX_PHASE_ADVANCE_DECIDEG,
                     ALGO_MAX_PHASE_ADVANCE_DECIDEG);
    g_foc_diag.electrical_angle_decideg =
        algoAngleGetElectricalAngleDecideg(feedback->encoder_raw);
    drive_electrical_angle_decideg =
        algoFocWrapAngle((int32_t)g_foc_diag.electrical_angle_decideg +
                         phase_advance_decideg);
    g_drive_electrical_angle_decideg =
        (uint16_t)drive_electrical_angle_decideg;
  } else {
    g_closed_loop_synced = false;
    g_target_position_microstep_q16 += delta_microstep_q16;
    g_foc_diag.electrical_angle_decideg = 0U;
    drive_electrical_angle_decideg =
        algoFocGetElectricalAngleFromPositionError(g_target_position_microstep_q16);
    g_drive_electrical_angle_decideg =
        (uint16_t)algoFocWrapAngle(drive_electrical_angle_decideg);
  }

  ramp_current_ma =
      algoRampStep(&g_current_ramp,
                   algoLimitS32(command->target_current_ma, 0, 1200));
  theta_rad = ((float)g_drive_electrical_angle_decideg * 3.1415926f) / 1800.0f;
  sin_theta = sinf(theta_rad);
  cos_theta = cosf(theta_rad);

  phase_a_current_ma = (int32_t)((float)ramp_current_ma * sin_theta);
  phase_b_current_ma = (int32_t)((float)ramp_current_ma * cos_theta);
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

void algoFocGetDiagnostics(algo_foc_diag_t *diag) {

  if (diag == NULL) {
    return;
  }

  *diag = g_foc_diag;
}

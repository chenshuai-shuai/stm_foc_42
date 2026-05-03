#include "algo_foc.h"

#include <math.h>
#include <stddef.h>

#include "algo_angle.h"
#include "algo_limit.h"
#include "algo_pid.h"
#include "algo_ramp.h"

static const algo_angle_config_t g_default_angle_config = {
    16384U,
    50U,
    1,
    0U,
};

#define ALGO_STEPPER_FULL_STEPS_PER_REV              200L
#define ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP        256L
#define ALGO_STEPPER_MICROSTEPS_PER_ELECTRICAL_CYCLE 1024L
#define ALGO_CONTROL_PERIOD_MS                       1L
#define ALGO_MAX_PHASE_ADVANCE_DECIDEG               600L
#define ALGO_POSITION_KP_NUM                         1L
#define ALGO_POSITION_KP_DEN                         3L
#define ALGO_SPEED_DAMP_DECIDEG_PER_RPM              2L
#define ALGO_HOLD_POSITION_KP_NUM                    1L
#define ALGO_HOLD_POSITION_KP_DEN                    6L
#define ALGO_HOLD_SPEED_DAMP_DECIDEG_PER_RPM         6L
#define ALGO_HOLD_MAX_PHASE_ADVANCE_DECIDEG          260L
#define ALGO_HOME_HOLD_SPEED_KP_NUM                  1L
#define ALGO_HOME_HOLD_SPEED_KP_DEN                  80L
#define ALGO_HOME_HOLD_MAX_SPEED_RPM                 3L
#define ALGO_HOME_HOLD_POSITION_KP_NUM               1L
#define ALGO_HOME_HOLD_POSITION_KP_DEN               18L
#define ALGO_HOME_HOLD_SPEED_DAMP_DECIDEG_PER_RPM    16L
#define ALGO_HOME_HOLD_MAX_PHASE_ADVANCE_DECIDEG     90L
#define ALGO_HOME_HOLD_SETTLE_WINDOW_DECIDEG         20L
#define ALGO_HOME_HOLD_SETTLE_SPEED_RPM              1L
#define ALGO_HOME_HOLD_ALIGN_CURRENT_MA              450L
#define ALGO_HOME_HOLD_RETURN_CURRENT_MA             650L
#define ALGO_SPEED_RAMP_UPDATE_DIVIDER               5U
#define ALGO_POSITION_MODE_KP_NUM                    1L
#define ALGO_POSITION_MODE_KP_DEN                    18L
#define ALGO_POSITION_HOLD_WINDOW_DECIDEG            30L
#define ALGO_SPEED_PID_KP                            3L
#define ALGO_SPEED_PID_KI                            1L
#define ALGO_SPEED_FILTER_SHIFT                      3L
#define ALGO_SPEED_MEAS_WINDOW_MS                    20L
#define ALGO_SPEED_MIN_CURRENT_MA                    180L

static algo_ramp_t g_current_ramp;
static algo_ramp_t g_speed_ramp;
static algo_ramp_t g_speed_current_ramp;
static algo_pid_t g_speed_pid;
static algo_foc_diag_t g_foc_diag;
static algo_angle_config_t g_active_angle_config;
static int8_t g_phase_order_sign;
static int64_t g_target_position_microstep_q16;
static int64_t g_last_actual_position_microstep_q16;
static int64_t g_home_hold_reference_microstep_q16;
static uint16_t g_drive_electrical_angle_decideg;
static bool g_closed_loop_synced;
static uint8_t g_speed_ramp_divider;
static int32_t g_filtered_speed_rpm;
static int32_t g_last_measured_speed_rpm;
static int64_t g_speed_meas_accum_delta_microstep_q16;
static uint8_t g_speed_meas_tick_count;

static int32_t algoFocWrapAngle(int32_t angle_decideg) {

  while (angle_decideg < 0) {
    angle_decideg += 3600;
  }

  while (angle_decideg >= 3600) {
    angle_decideg -= 3600;
  }

  return angle_decideg;
}

static int32_t algoFocWrapMechanicalError(int32_t error_decideg) {

  while (error_decideg > 1800L) {
    error_decideg -= 3600L;
  }

  while (error_decideg < -1800L) {
    error_decideg += 3600L;
  }

  return error_decideg;
}

static int64_t algoFocWrapSingleTurnMicrostepQ16(int64_t position_microstep_q16) {
  const int64_t one_rev_microstep_q16 =
      (int64_t)ALGO_STEPPER_FULL_STEPS_PER_REV *
      ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
      65536LL;

  while (position_microstep_q16 < 0) {
    position_microstep_q16 += one_rev_microstep_q16;
  }

  while (position_microstep_q16 >= one_rev_microstep_q16) {
    position_microstep_q16 -= one_rev_microstep_q16;
  }

  return position_microstep_q16;
}

static uint16_t algoFocGetMechanicalDecidegFromMicrostepQ16(
    int64_t position_microstep_q16) {
  const int64_t one_rev_microstep_q16 =
      (int64_t)ALGO_STEPPER_FULL_STEPS_PER_REV *
      ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
      65536LL;

  position_microstep_q16 =
      algoFocWrapSingleTurnMicrostepQ16(position_microstep_q16);

  return (uint16_t)((position_microstep_q16 * 3600LL) / one_rev_microstep_q16);
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

static int64_t algoFocGetMicrostepQ16FromMechanicalDecideg(
    int32_t mechanical_angle_decideg) {

  return ((int64_t)mechanical_angle_decideg *
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

static int32_t algoFocUpdateMeasuredSpeedRpm(int64_t delta_microstep_q16) {

  g_speed_meas_accum_delta_microstep_q16 += delta_microstep_q16;
  g_speed_meas_tick_count++;

  if (g_speed_meas_tick_count >= (uint8_t)ALGO_SPEED_MEAS_WINDOW_MS) {
    g_last_measured_speed_rpm =
        (int32_t)((g_speed_meas_accum_delta_microstep_q16 * 60000LL) /
                  ((int64_t)ALGO_STEPPER_FULL_STEPS_PER_REV *
                   ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
                   65536LL *
                   ALGO_SPEED_MEAS_WINDOW_MS));
    g_speed_meas_accum_delta_microstep_q16 = 0;
    g_speed_meas_tick_count = 0U;
  }

  return g_last_measured_speed_rpm;
}

static int32_t algoFocGetElectricalAngleFromPositionError(
    int64_t error_microstep_q16) {

  return (int32_t)((error_microstep_q16 * 3600LL) /
                   ((int64_t)ALGO_STEPPER_MICROSTEPS_PER_ELECTRICAL_CYCLE *
                    65536LL));
}

static int32_t algoFocGetSpeedModeCurrentRequest(int32_t target_speed_rpm,
                                                 int32_t filtered_speed_rpm,
                                                 int32_t run_current_ma,
                                                 int32_t hold_current_ma) {
  int32_t speed_error_rpm;
  int32_t speed_pid_output_ma;
  int32_t base_current_ma;

  speed_error_rpm = target_speed_rpm - filtered_speed_rpm;
  base_current_ma = algoLimitS32(hold_current_ma, 0, run_current_ma);

  if (speed_error_rpm <= 0) {
    algoPidReset(&g_speed_pid);
    return algoLimitS32(base_current_ma,
                        ALGO_SPEED_MIN_CURRENT_MA,
                        run_current_ma);
  }

  speed_pid_output_ma = algoPidStep(&g_speed_pid, speed_error_rpm);
  return algoLimitS32(base_current_ma + speed_pid_output_ma,
                      ALGO_SPEED_MIN_CURRENT_MA,
                      run_current_ma);
}

void algoFocInit(void) {

  g_active_angle_config = g_default_angle_config;
  algoAngleInit(&g_active_angle_config);
  algoRampInit(&g_current_ramp, 0, 4, 6);
  algoRampInit(&g_speed_ramp, 0, 1, 2);
  algoRampInit(&g_speed_current_ramp, 0, 2, 3);
  algoPidInit(&g_speed_pid, ALGO_SPEED_PID_KP, ALGO_SPEED_PID_KI, -1200, 1200);
  g_foc_diag.electrical_angle_decideg = 0U;
  g_foc_diag.pole_pairs = g_active_angle_config.pole_pairs;
  g_foc_diag.encoder_zero_raw = g_active_angle_config.encoder_zero_raw;
  g_foc_diag.encoder_direction = g_active_angle_config.encoder_direction;
  g_foc_diag.phase_order_sign = 1;
  g_foc_diag.speed_target_rpm = 0;
  g_foc_diag.speed_measured_rpm = 0;
  g_foc_diag.speed_filtered_rpm = 0;
  g_foc_diag.speed_current_ref_ma = 0;
  g_target_position_microstep_q16 = 0;
  g_last_actual_position_microstep_q16 = 0;
  g_home_hold_reference_microstep_q16 = 0;
  g_drive_electrical_angle_decideg = 0U;
  g_closed_loop_synced = false;
  g_speed_ramp_divider = 0U;
  g_filtered_speed_rpm = 0;
  g_last_measured_speed_rpm = 0;
  g_speed_meas_accum_delta_microstep_q16 = 0;
  g_speed_meas_tick_count = 0U;
  g_phase_order_sign = 1;
}

void algoFocApplyCalibration(const algo_foc_calibration_t *calibration) {

  g_active_angle_config = g_default_angle_config;

  if (calibration != NULL) {
    if (calibration->pole_pairs != 0U) {
      g_active_angle_config.pole_pairs = calibration->pole_pairs;
    }
    if (calibration->encoder_direction != 0) {
      g_active_angle_config.encoder_direction = calibration->encoder_direction;
    }
    if (calibration->phase_order_sign != 0) {
      g_phase_order_sign = calibration->phase_order_sign;
    } else {
      g_phase_order_sign = 1;
    }
    if (calibration->valid != 0U) {
      g_active_angle_config.encoder_zero_raw = calibration->encoder_zero_raw;
    }
  }

  algoAngleInit(&g_active_angle_config);
  g_foc_diag.pole_pairs = g_active_angle_config.pole_pairs;
  g_foc_diag.encoder_zero_raw = g_active_angle_config.encoder_zero_raw;
  g_foc_diag.encoder_direction = g_active_angle_config.encoder_direction;
  g_foc_diag.phase_order_sign = g_phase_order_sign;
}

void algoFocStep(const motor_feedback_t *feedback,
                 const motor_command_t *command,
                 motor_control_output_t *output) {
  int32_t ramp_current_ma;
  int32_t requested_current_ma;
  int32_t target_speed_rpm;
  int64_t delta_microstep_q16;
  int64_t actual_position_microstep_q16;
  int64_t position_error_microstep_q16;
  int64_t actual_position_delta_microstep_q16;
  int32_t measured_speed_rpm;
  int32_t position_error_electrical_decideg;
  int32_t position_error_mechanical_decideg;
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
    algoRampInit(&g_speed_current_ramp, 0, 2, 3);
    algoPidReset(&g_speed_pid);
    g_foc_diag.electrical_angle_decideg = 0U;
    g_foc_diag.speed_target_rpm = 0;
    g_foc_diag.speed_measured_rpm = 0;
    g_foc_diag.speed_filtered_rpm = 0;
    g_foc_diag.speed_current_ref_ma = 0;
    g_target_position_microstep_q16 = 0;
    g_last_actual_position_microstep_q16 = 0;
    g_home_hold_reference_microstep_q16 = 0;
    g_drive_electrical_angle_decideg = 0U;
    g_closed_loop_synced = false;
    g_speed_ramp_divider = 0U;
    g_filtered_speed_rpm = 0;
    g_last_measured_speed_rpm = 0;
    g_speed_meas_accum_delta_microstep_q16 = 0;
    g_speed_meas_tick_count = 0U;
    output->duty_a_permille = 0U;
    output->duty_b_permille = 0U;
    output->phase_a_forward = true;
    output->phase_b_forward = true;
    output->enable = false;
    return;
  }

  target_speed_rpm = 0;
  requested_current_ma = command->target_current_ma;
  measured_speed_rpm = 0;

  switch (command->mode) {
  case MOTOR_CONTROL_MODE_OPEN_LOOP:
    target_speed_rpm = algoLimitS32(command->target_speed_rpm, -60, 60);
    requested_current_ma = command->target_current_ma;
    g_closed_loop_synced = false;
    break;

  case MOTOR_CONTROL_MODE_CURRENT_TEST:
    requested_current_ma = command->target_current_ma;
    break;

  case MOTOR_CONTROL_MODE_HOME_HOLD:
    requested_current_ma = command->target_current_ma;
    break;

  case MOTOR_CONTROL_MODE_POSITION:
    if ((feedback != NULL) && feedback->encoder_ready) {
      position_error_mechanical_decideg =
          (int32_t)(command->target_position_total_decideg -
                    (((int32_t)feedback->mechanical_turn_count * 3600L) +
                     (int32_t)feedback->mechanical_angle_decideg));
      target_speed_rpm =
          (position_error_mechanical_decideg * ALGO_POSITION_MODE_KP_NUM) /
          ALGO_POSITION_MODE_KP_DEN;
      target_speed_rpm =
          algoLimitS32(target_speed_rpm,
                       -algoLimitS32(command->target_speed_rpm, 1, 60),
                       algoLimitS32(command->target_speed_rpm, 1, 60));
      if ((position_error_mechanical_decideg <= ALGO_POSITION_HOLD_WINDOW_DECIDEG) &&
          (position_error_mechanical_decideg >= -ALGO_POSITION_HOLD_WINDOW_DECIDEG)) {
        requested_current_ma = command->hold_current_ma;
      } else {
        requested_current_ma = command->target_current_ma;
      }
    }
    break;

  case MOTOR_CONTROL_MODE_SPEED:
  default:
    target_speed_rpm = algoLimitS32(command->target_speed_rpm, -60, 60);
    requested_current_ma = command->target_current_ma;
    break;
  }

  if (g_speed_ramp_divider == 0U) {
    target_speed_rpm = algoRampStep(&g_speed_ramp, target_speed_rpm);
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

  if ((command->mode == MOTOR_CONTROL_MODE_CURRENT_TEST) &&
      (feedback != NULL) && feedback->encoder_ready) {
    int32_t target_single_turn_decideg;
    int32_t single_turn_error_decideg;
    int32_t hold_measured_speed_rpm;
    int32_t hold_phase_advance_decideg;

    actual_position_microstep_q16 = algoFocGetMechanicalPositionMicrostepQ16(feedback);
    actual_position_delta_microstep_q16 =
        actual_position_microstep_q16 - g_last_actual_position_microstep_q16;
    g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
    hold_measured_speed_rpm =
        algoFocUpdateMeasuredSpeedRpm(actual_position_delta_microstep_q16);
    measured_speed_rpm = hold_measured_speed_rpm;
    target_single_turn_decideg = command->target_position_total_decideg % 3600L;
    if (target_single_turn_decideg < 0) {
      target_single_turn_decideg += 3600L;
    }
    single_turn_error_decideg =
        algoFocWrapMechanicalError(target_single_turn_decideg -
                                   (int32_t)feedback->mechanical_angle_decideg);
    position_error_microstep_q16 =
        algoFocGetMicrostepQ16FromMechanicalDecideg(single_turn_error_decideg);
    position_error_electrical_decideg =
        algoFocGetElectricalAngleFromPositionError(position_error_microstep_q16);
    hold_phase_advance_decideg =
        (((position_error_electrical_decideg * ALGO_HOLD_POSITION_KP_NUM) /
              ALGO_HOLD_POSITION_KP_DEN) -
         (hold_measured_speed_rpm * ALGO_HOLD_SPEED_DAMP_DECIDEG_PER_RPM)) *
        (int32_t)g_phase_order_sign;
    phase_advance_decideg =
        algoLimitS32(hold_phase_advance_decideg,
                     -ALGO_HOLD_MAX_PHASE_ADVANCE_DECIDEG,
                     ALGO_HOLD_MAX_PHASE_ADVANCE_DECIDEG);
    g_foc_diag.electrical_angle_decideg =
        algoAngleGetElectricalAngleDecideg(feedback->encoder_raw);
    drive_electrical_angle_decideg =
        algoFocWrapAngle((int32_t)g_foc_diag.electrical_angle_decideg +
                         phase_advance_decideg);
    g_drive_electrical_angle_decideg =
        (uint16_t)drive_electrical_angle_decideg;
    g_closed_loop_synced = true;
    algoPidReset(&g_speed_pid);
  } else if ((command->mode == MOTOR_CONTROL_MODE_HOME_HOLD) &&
             (feedback != NULL) && feedback->encoder_ready) {
    int32_t target_single_turn_decideg;
    int32_t single_turn_error_decideg;
    int32_t reference_decideg;
    int32_t reference_error_decideg;
    int32_t home_hold_measured_speed_rpm;
    int32_t home_target_speed_rpm;
    int32_t home_hold_phase_advance_decideg;
    int64_t home_ref_delta_microstep_q16;
    int64_t actual_single_turn_microstep_q16;

    actual_position_microstep_q16 = algoFocGetMechanicalPositionMicrostepQ16(feedback);
    actual_single_turn_microstep_q16 =
        algoFocGetMicrostepQ16FromMechanicalDecideg(
            (int32_t)feedback->mechanical_angle_decideg);
    actual_position_delta_microstep_q16 =
        actual_position_microstep_q16 - g_last_actual_position_microstep_q16;
    g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
    home_hold_measured_speed_rpm =
        algoFocUpdateMeasuredSpeedRpm(actual_position_delta_microstep_q16);
    measured_speed_rpm = home_hold_measured_speed_rpm;

    if (!g_closed_loop_synced) {
      g_home_hold_reference_microstep_q16 = actual_single_turn_microstep_q16;
      g_closed_loop_synced = true;
    }

    target_single_turn_decideg = command->target_position_total_decideg % 3600L;
    if (target_single_turn_decideg < 0) {
      target_single_turn_decideg += 3600L;
    }

    reference_decideg =
        (int32_t)algoFocGetMechanicalDecidegFromMicrostepQ16(
            g_home_hold_reference_microstep_q16);
    reference_error_decideg =
        algoFocWrapMechanicalError(target_single_turn_decideg - reference_decideg);

    home_target_speed_rpm =
        (reference_error_decideg * ALGO_HOME_HOLD_SPEED_KP_NUM) /
        ALGO_HOME_HOLD_SPEED_KP_DEN;
    home_target_speed_rpm =
        algoLimitS32(home_target_speed_rpm,
                     -ALGO_HOME_HOLD_MAX_SPEED_RPM,
                     ALGO_HOME_HOLD_MAX_SPEED_RPM);

    home_ref_delta_microstep_q16 =
        ((int64_t)home_target_speed_rpm *
         ALGO_STEPPER_FULL_STEPS_PER_REV *
         ALGO_STEPPER_MICROSTEPS_PER_FULL_STEP *
         65536LL *
         ALGO_CONTROL_PERIOD_MS) /
        60000LL;

    g_home_hold_reference_microstep_q16 =
        algoFocWrapSingleTurnMicrostepQ16(
            g_home_hold_reference_microstep_q16 + home_ref_delta_microstep_q16);

    if ((reference_error_decideg <= ALGO_HOME_HOLD_SETTLE_WINDOW_DECIDEG) &&
        (reference_error_decideg >= -ALGO_HOME_HOLD_SETTLE_WINDOW_DECIDEG)) {
      g_home_hold_reference_microstep_q16 =
          algoFocGetMicrostepQ16FromMechanicalDecideg(target_single_turn_decideg);
    }

    reference_decideg =
        (int32_t)algoFocGetMechanicalDecidegFromMicrostepQ16(
            g_home_hold_reference_microstep_q16);
    single_turn_error_decideg =
        algoFocWrapMechanicalError(reference_decideg -
                                   (int32_t)feedback->mechanical_angle_decideg);

    if (((single_turn_error_decideg <= ALGO_HOME_HOLD_SETTLE_WINDOW_DECIDEG) &&
         (single_turn_error_decideg >= -ALGO_HOME_HOLD_SETTLE_WINDOW_DECIDEG)) &&
        ((home_hold_measured_speed_rpm <= ALGO_HOME_HOLD_SETTLE_SPEED_RPM) &&
         (home_hold_measured_speed_rpm >= -ALGO_HOME_HOLD_SETTLE_SPEED_RPM))) {
      requested_current_ma = algoLimitS32(command->hold_current_ma, 0, ALGO_HOME_HOLD_ALIGN_CURRENT_MA);
    } else {
      requested_current_ma = algoLimitS32(command->target_current_ma,
                                          ALGO_HOME_HOLD_ALIGN_CURRENT_MA,
                                          ALGO_HOME_HOLD_RETURN_CURRENT_MA);
    }

    position_error_microstep_q16 =
        algoFocGetMicrostepQ16FromMechanicalDecideg(single_turn_error_decideg);
    position_error_electrical_decideg =
        algoFocGetElectricalAngleFromPositionError(position_error_microstep_q16);
    home_hold_phase_advance_decideg =
        ((((position_error_electrical_decideg * ALGO_HOME_HOLD_POSITION_KP_NUM) /
              ALGO_HOME_HOLD_POSITION_KP_DEN) -
         (home_hold_measured_speed_rpm *
          ALGO_HOME_HOLD_SPEED_DAMP_DECIDEG_PER_RPM)) +
         (home_target_speed_rpm * 12L)) *
        (int32_t)g_phase_order_sign;
    phase_advance_decideg =
        algoLimitS32(home_hold_phase_advance_decideg,
                     -ALGO_HOME_HOLD_MAX_PHASE_ADVANCE_DECIDEG,
                     ALGO_HOME_HOLD_MAX_PHASE_ADVANCE_DECIDEG);

    g_foc_diag.electrical_angle_decideg =
        algoAngleGetElectricalAngleDecideg(feedback->encoder_raw);
    drive_electrical_angle_decideg =
        algoFocWrapAngle((int32_t)g_foc_diag.electrical_angle_decideg +
                         phase_advance_decideg);
    g_drive_electrical_angle_decideg =
        (uint16_t)drive_electrical_angle_decideg;
    g_closed_loop_synced = true;
    algoPidReset(&g_speed_pid);
  } else if ((command->mode != MOTOR_CONTROL_MODE_OPEN_LOOP) &&
             (feedback != NULL) && feedback->encoder_ready) {
    actual_position_microstep_q16 = algoFocGetMechanicalPositionMicrostepQ16(feedback);

    if (!g_closed_loop_synced) {
      g_target_position_microstep_q16 = actual_position_microstep_q16;
      g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
      g_closed_loop_synced = true;
    }

    if (command->mode == MOTOR_CONTROL_MODE_POSITION) {
      g_target_position_microstep_q16 =
          algoFocGetMicrostepQ16FromMechanicalDecideg(
              command->target_position_total_decideg);
    } else {
      g_target_position_microstep_q16 += delta_microstep_q16;
    }
    actual_position_delta_microstep_q16 =
        actual_position_microstep_q16 - g_last_actual_position_microstep_q16;
    g_last_actual_position_microstep_q16 = actual_position_microstep_q16;
    measured_speed_rpm =
        algoFocUpdateMeasuredSpeedRpm(actual_position_delta_microstep_q16);
    g_filtered_speed_rpm +=
        (measured_speed_rpm - g_filtered_speed_rpm) >> ALGO_SPEED_FILTER_SHIFT;

    if (command->mode == MOTOR_CONTROL_MODE_SPEED) {
      requested_current_ma =
          algoFocGetSpeedModeCurrentRequest(target_speed_rpm,
                                            g_filtered_speed_rpm,
                                            command->target_current_ma,
                                            command->hold_current_ma);
    } else {
      algoPidReset(&g_speed_pid);
    }
    position_error_microstep_q16 =
        g_target_position_microstep_q16 - actual_position_microstep_q16;
    position_error_electrical_decideg =
        algoFocGetElectricalAngleFromPositionError(position_error_microstep_q16);
    phase_advance_decideg =
        (((position_error_electrical_decideg * ALGO_POSITION_KP_NUM) /
              ALGO_POSITION_KP_DEN) -
         (g_filtered_speed_rpm * ALGO_SPEED_DAMP_DECIDEG_PER_RPM)) *
        (int32_t)g_phase_order_sign;
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
    if (command->mode == MOTOR_CONTROL_MODE_CURRENT_TEST) {
      g_target_position_microstep_q16 =
          algoFocGetMicrostepQ16FromMechanicalDecideg(
              command->target_position_total_decideg);
    } else {
      g_target_position_microstep_q16 += delta_microstep_q16;
    }
    g_foc_diag.electrical_angle_decideg = 0U;
    drive_electrical_angle_decideg =
        algoFocGetElectricalAngleFromPositionError(g_target_position_microstep_q16);
    g_drive_electrical_angle_decideg =
        (uint16_t)algoFocWrapAngle(drive_electrical_angle_decideg);
  }

  if (command->mode == MOTOR_CONTROL_MODE_SPEED) {
    ramp_current_ma =
        algoRampStep(&g_speed_current_ramp,
                     algoLimitS32(requested_current_ma, 0, 1200));
  } else {
    ramp_current_ma =
        algoRampStep(&g_current_ramp,
                     algoLimitS32(requested_current_ma, 0, 1200));
  }
  g_foc_diag.speed_target_rpm = (int16_t)target_speed_rpm;
  g_foc_diag.speed_measured_rpm = (int16_t)measured_speed_rpm;
  g_foc_diag.speed_filtered_rpm = (int16_t)g_filtered_speed_rpm;
  g_foc_diag.speed_current_ref_ma = (int16_t)ramp_current_ma;
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

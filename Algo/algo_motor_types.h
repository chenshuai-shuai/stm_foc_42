#ifndef ALGO_MOTOR_TYPES_H
#define ALGO_MOTOR_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t phase_a_current_ma;
  int16_t phase_b_current_ma;
  uint16_t bus_voltage_mv;
  int16_t electrical_angle_deg;
  uint16_t electrical_angle_decideg;
  int16_t mechanical_speed_rpm;
  uint16_t mechanical_angle_decideg;
  int32_t mechanical_turn_count;
  uint16_t encoder_raw;
  bool encoder_ready;
} motor_feedback_t;

typedef enum {
  MOTOR_CONTROL_MODE_OPEN_LOOP = 0,
  MOTOR_CONTROL_MODE_CURRENT_TEST,
  MOTOR_CONTROL_MODE_HOME_HOLD,
  MOTOR_CONTROL_MODE_SPEED,
  MOTOR_CONTROL_MODE_POSITION
} motor_control_mode_t;

typedef struct {
  bool run;
  bool auto_stop_on_target;
  motor_control_mode_t mode;
  int16_t target_current_ma;
  int16_t hold_current_ma;
  int16_t target_speed_rpm;
  int32_t target_position_total_decideg;
} motor_command_t;

typedef struct {
  uint16_t duty_a_permille;
  uint16_t duty_b_permille;
  bool phase_a_forward;
  bool phase_b_forward;
  bool enable;
} motor_control_output_t;

typedef struct {
  uint16_t electrical_angle_decideg;
  uint8_t pole_pairs;
  uint16_t encoder_zero_raw;
  int8_t encoder_direction;
  int8_t phase_order_sign;
  int16_t speed_target_rpm;
  int16_t speed_measured_rpm;
  int16_t speed_filtered_rpm;
  int16_t speed_current_ref_ma;
} algo_foc_diag_t;

#endif /* ALGO_MOTOR_TYPES_H */

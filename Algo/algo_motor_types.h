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

typedef struct {
  bool run;
  int16_t target_current_ma;
  int16_t target_speed_rpm;
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
} algo_foc_diag_t;

#endif /* ALGO_MOTOR_TYPES_H */

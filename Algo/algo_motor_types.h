#ifndef ALGO_MOTOR_TYPES_H
#define ALGO_MOTOR_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t phase_a_current_ma;
  int16_t phase_b_current_ma;
  uint16_t bus_voltage_mv;
  int16_t electrical_angle_deg;
  int16_t mechanical_speed_rpm;
} motor_feedback_t;

typedef struct {
  bool run;
  int16_t target_current_ma;
  int16_t target_speed_rpm;
} motor_command_t;

typedef struct {
  uint16_t duty_a_permille;
  uint16_t duty_b_permille;
  uint16_t duty_c_permille;
  bool pwm_enable;
} motor_control_output_t;

#endif /* ALGO_MOTOR_TYPES_H */

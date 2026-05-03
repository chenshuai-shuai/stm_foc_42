#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  APP_CONTROL_MODE_OPEN_LOOP = 0,
  APP_CONTROL_MODE_CURRENT_TEST,
  APP_CONTROL_MODE_HOME_HOLD,
  APP_CONTROL_MODE_SPEED,
  APP_CONTROL_MODE_POSITION
} app_control_mode_t;

typedef struct {
  bool enable_request;
  bool run_request;
  bool auto_stop_on_target;
  app_control_mode_t control_mode;
  int16_t target_current_ma;
  int16_t hold_current_ma;
  int16_t target_speed_rpm;
  int32_t target_position_total_decideg;
} app_command_t;

typedef enum {
  APP_CMD_SRC_BOOT = 0,
  APP_CMD_SRC_UI,
  APP_CMD_SRC_COMM,
  APP_CMD_SRC_PARAM,
  APP_CMD_SRC_SAFETY
} app_command_source_t;

typedef struct {
  app_command_t value;
  uint32_t revision;
  app_command_source_t source;
} app_command_snapshot_t;

typedef struct {
  uint32_t fault_flags;
  uint8_t state;
  bool gate_enabled;
} app_fault_snapshot_t;

typedef struct {
  uint16_t bus_voltage_mv;
  int16_t phase_a_current_ma;
  uint16_t duty_a_permille;
  uint32_t fault_flags;
  uint8_t state;
  uint8_t control_mode;
  int16_t target_current_ma;
  int16_t hold_current_ma;
  int16_t target_speed_rpm;
  int32_t target_position_total_decideg;
  uint16_t mechanical_angle_decideg;
  uint16_t electrical_angle_decideg;
  uint16_t encoder_raw;
  int32_t mechanical_turn_count;
  uint8_t encoder_ready;
  int16_t measured_speed_rpm;
  int16_t filtered_speed_rpm;
  int16_t speed_current_ref_ma;
} app_runtime_fastloop_update_t;

struct app_runtime {
  uint32_t rtos_seconds;
  uint32_t control_ticks;
  uint32_t comm_ticks;
  uint16_t bus_voltage_mv;
  int16_t phase_a_current_ma;
  uint16_t duty_a_permille;
  uint32_t fault_flags;
  uint8_t state;
  uint8_t control_mode;
  int16_t target_current_ma;
  int16_t hold_current_ma;
  int16_t target_speed_rpm;
  int32_t target_position_total_decideg;
  uint16_t mechanical_angle_decideg;
  uint16_t electrical_angle_decideg;
  uint16_t encoder_raw;
  int32_t mechanical_turn_count;
  uint8_t encoder_ready;
  int16_t measured_speed_rpm;
  int16_t filtered_speed_rpm;
  int16_t speed_current_ref_ma;
  uint32_t command_revision;
  app_command_source_t command_source;
};

typedef struct app_runtime app_runtime_t;

#endif /* APP_TYPES_H */

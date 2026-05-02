#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool run_request;
  int16_t target_current_ma;
  int16_t target_speed_rpm;
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
  int16_t target_current_ma;
  int16_t target_speed_rpm;
  uint16_t mechanical_angle_decideg;
  uint16_t encoder_raw;
  int32_t mechanical_turn_count;
  uint8_t encoder_ready;
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
  int16_t target_current_ma;
  int16_t target_speed_rpm;
  uint16_t mechanical_angle_decideg;
  uint16_t encoder_raw;
  int32_t mechanical_turn_count;
  uint8_t encoder_ready;
  uint32_t command_revision;
  app_command_source_t command_source;
};

typedef struct app_runtime app_runtime_t;

#endif /* APP_TYPES_H */

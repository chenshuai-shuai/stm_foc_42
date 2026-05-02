#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool run_request;
  int16_t target_current_ma;
  int16_t target_speed_rpm;
} app_command_t;

typedef struct {
  uint32_t fault_flags;
  uint8_t state;
  bool gate_enabled;
} app_fault_snapshot_t;

struct app_runtime {
  volatile uint32_t rtos_seconds;
  volatile uint32_t control_ticks;
  volatile uint32_t comm_ticks;
  volatile uint16_t bus_voltage_mv;
  volatile int16_t phase_a_current_ma;
  volatile uint16_t duty_a_permille;
  volatile uint32_t fault_flags;
  volatile uint8_t state;
  volatile int16_t target_current_ma;
  volatile int16_t target_speed_rpm;
};

typedef struct app_runtime app_runtime_t;

#endif /* APP_TYPES_H */

#include "algo_foc.h"

#include <stddef.h>

#include "algo_limit.h"
#include "algo_observer.h"
#include "algo_pid.h"
#include "algo_ramp.h"

static algo_pid_t g_current_pid;
static algo_ramp_t g_current_ramp;
static algo_observer_t g_speed_observer;

void algoFocInit(void) {

  algoPidInit(&g_current_pid, 1, 1, 0, 950);
  algoRampInit(&g_current_ramp, 0, 80, 120);
  algoObserverInit(&g_speed_observer);
}

void algoFocStep(const motor_feedback_t *feedback,
                 const motor_command_t *command,
                 motor_control_output_t *output) {

  int32_t closed_loop_duty;
  int32_t current_error;
  int32_t estimated_speed_rpm;
  int32_t ramp_current_ma;
  uint16_t base_duty;

  if ((command == NULL) || (output == NULL) || (command->run == false)) {
    algoPidReset(&g_current_pid);
    algoRampInit(&g_current_ramp, 0, 80, 120);
    if (output != NULL) {
      output->duty_a_permille = 0U;
      output->duty_b_permille = 0U;
      output->duty_c_permille = 0U;
      output->pwm_enable = false;
    }
    return;
  }

  estimated_speed_rpm =
      algoObserverStep(&g_speed_observer, feedback->mechanical_speed_rpm);
  ramp_current_ma = algoRampStep(&g_current_ramp, command->target_current_ma);
  current_error = ramp_current_ma - feedback->phase_a_current_ma;
  closed_loop_duty = algoPidStep(&g_current_pid, current_error);
  closed_loop_duty = algoLimitS32(closed_loop_duty, 0, 950);
  base_duty = algoLimitU16((uint16_t)closed_loop_duty, 0U, 950U);

  (void)estimated_speed_rpm;

  output->duty_a_permille = base_duty;
  output->duty_b_permille = base_duty;
  output->duty_c_permille = base_duty;
  output->pwm_enable = true;
}

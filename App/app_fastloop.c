#include "app_fastloop.h"

#include <stddef.h>

#include "app_fault_mgr.h"
#include "app_state_machine.h"
#include "algo_foc.h"
#include "algo_motor_types.h"
#include "hal_gate_if.h"
#include "hal_motor_if.h"
#include "hal_sense_if.h"

void appFastLoopInit(void) {
}

void appFastLoopStep(const app_command_t *command, app_runtime_t *runtime) {

  app_fault_snapshot_t fault;
  motor_command_t motor_command;
  motor_feedback_t feedback;
  motor_control_output_t output;

  if ((command == NULL) || (runtime == NULL)) {
    return;
  }

  halSenseGetFeedbackSnapshot(&feedback);

  fault.fault_flags = appFaultMgrCheck(&feedback);
  fault.state = (uint8_t)appStateMachineStep(runtime->control_ticks,
                                             fault.fault_flags);
  fault.gate_enabled = (bool)(fault.fault_flags == APP_FAULT_NONE);
  halGateSetEnabled(fault.gate_enabled);

  motor_command.run = command->run_request;
  motor_command.target_current_ma = command->target_current_ma;
  motor_command.target_speed_rpm = command->target_speed_rpm;

  algoFocStep(&feedback, &motor_command, &output);
  output.pwm_enable = (bool)(output.pwm_enable &&
                             fault.gate_enabled &&
                             halGateIsEnabled());
  halMotorApplyOutput(&output);

  runtime->bus_voltage_mv = feedback.bus_voltage_mv;
  runtime->phase_a_current_ma = feedback.phase_a_current_ma;
  runtime->duty_a_permille = output.duty_a_permille;
  runtime->fault_flags = fault.fault_flags;
  runtime->state = fault.state;
  runtime->target_current_ma = command->target_current_ma;
  runtime->target_speed_rpm = command->target_speed_rpm;
}

#include "app_fastloop.h"

#include <stddef.h>

#include "app_fault_mgr.h"
#include "app_main.h"
#include "app_state_machine.h"
#include "algo_foc.h"
#include "algo_motor_types.h"
#include "hal_gate_if.h"
#include "hal_motor_if.h"
#include "hal_sense_if.h"

void appFastLoopInit(void) {
}

void appFastLoopStep(const app_command_t *command) {
  algo_foc_diag_t foc_diag;
  app_runtime_fastloop_update_t runtime_update;
  app_fault_snapshot_t fault;
  motor_command_t motor_command;
  motor_feedback_t feedback;
  motor_control_output_t output;
  app_runtime_t runtime_snapshot;

  if (command == NULL) {
    return;
  }

  appRuntimeGetSnapshot(&runtime_snapshot);
  halSenseGetFeedbackSnapshot(&feedback);

  fault.fault_flags = appFaultMgrCheck(&feedback);
  fault.state = (uint8_t)appStateMachineStep(runtime_snapshot.control_ticks,
                                             fault.fault_flags);
  fault.gate_enabled = (bool)(fault.fault_flags == APP_FAULT_NONE);
  halGateSetEnabled(fault.gate_enabled);

  motor_command.run = command->run_request;
  motor_command.target_current_ma = command->target_current_ma;
  motor_command.target_speed_rpm = command->target_speed_rpm;

  algoFocStep(&feedback, &motor_command, &output);
  algoFocGetDiagnostics(&foc_diag);
  output.enable = (bool)(output.enable &&
                         fault.gate_enabled &&
                         halGateIsEnabled());
  halMotorApplyOutput(&output);

  runtime_update.bus_voltage_mv = feedback.bus_voltage_mv;
  runtime_update.phase_a_current_ma = feedback.phase_a_current_ma;
  runtime_update.duty_a_permille = output.duty_a_permille;
  runtime_update.fault_flags = fault.fault_flags;
  runtime_update.state = fault.state;
  runtime_update.target_current_ma = command->target_current_ma;
  runtime_update.target_speed_rpm = command->target_speed_rpm;
  runtime_update.mechanical_angle_decideg = feedback.mechanical_angle_decideg;
  runtime_update.electrical_angle_decideg = foc_diag.electrical_angle_decideg;
  runtime_update.encoder_raw = feedback.encoder_raw;
  runtime_update.mechanical_turn_count = feedback.mechanical_turn_count;
  runtime_update.encoder_ready = feedback.encoder_ready ? 1U : 0U;
  appRuntimePublishFastLoop(&runtime_update);
}

#include "app_fastloop.h"

#include <stddef.h>

#include "app_fault_mgr.h"
#include "app_calib.h"
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

  if (!command->enable_request) {
    motor_command.run = false;
    motor_command.auto_stop_on_target = false;
    motor_command.mode = (motor_control_mode_t)command->control_mode;
    motor_command.target_current_ma =
        appCalibClampRunCurrent(command->target_current_ma);
    motor_command.hold_current_ma =
        appCalibClampRunCurrent(command->hold_current_ma);
    motor_command.target_speed_rpm = command->target_speed_rpm;
    motor_command.target_position_total_decideg =
        command->target_position_total_decideg;
  } else if (!command->run_request) {
    motor_command.run = false;
    motor_command.auto_stop_on_target = false;
    motor_command.mode = (motor_control_mode_t)command->control_mode;
    motor_command.target_current_ma =
        appCalibClampRunCurrent(command->target_current_ma);
    motor_command.hold_current_ma =
        appCalibClampRunCurrent(command->hold_current_ma);
    motor_command.target_speed_rpm = command->target_speed_rpm;
    motor_command.target_position_total_decideg =
        command->target_position_total_decideg;
  } else {
    motor_command.run = true;
    motor_command.auto_stop_on_target = command->auto_stop_on_target;
    motor_command.mode = (motor_control_mode_t)command->control_mode;
    motor_command.target_current_ma =
        appCalibClampRunCurrent(command->target_current_ma);
    motor_command.hold_current_ma =
        appCalibClampRunCurrent(command->hold_current_ma);
    motor_command.target_speed_rpm = command->target_speed_rpm;
    motor_command.target_position_total_decideg =
        command->target_position_total_decideg;
  }

  if (!appCalibProcess(&feedback, &output)) {
    algoFocStep(&feedback, &motor_command, &output);
  }
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
  runtime_update.control_mode = (uint8_t)motor_command.mode;
  runtime_update.target_current_ma = motor_command.target_current_ma;
  runtime_update.hold_current_ma = motor_command.hold_current_ma;
  runtime_update.target_speed_rpm = motor_command.target_speed_rpm;
  runtime_update.target_position_total_decideg =
      motor_command.target_position_total_decideg;
  runtime_update.mechanical_angle_decideg = feedback.mechanical_angle_decideg;
  runtime_update.electrical_angle_decideg = foc_diag.electrical_angle_decideg;
  runtime_update.encoder_raw = feedback.encoder_raw;
  runtime_update.mechanical_turn_count = feedback.mechanical_turn_count;
  runtime_update.encoder_ready = feedback.encoder_ready ? 1U : 0U;
  runtime_update.measured_speed_rpm = foc_diag.speed_measured_rpm;
  runtime_update.filtered_speed_rpm = foc_diag.speed_filtered_rpm;
  runtime_update.speed_current_ref_ma = foc_diag.speed_current_ref_ma;
  appRuntimePublishFastLoop(&runtime_update);

  if (command->enable_request &&
      command->run_request &&
      command->auto_stop_on_target &&
      command->control_mode == APP_CONTROL_MODE_POSITION &&
      feedback.encoder_ready) {
    int32_t position_error_decideg =
        (int32_t)(command->target_position_total_decideg -
                  (((int32_t)feedback.mechanical_turn_count * 3600L) +
                   (int32_t)feedback.mechanical_angle_decideg));

    if ((position_error_decideg <= 20) && (position_error_decideg >= -20)) {
      app_command_t stop_command = *command;
      stop_command.run_request = false;
      stop_command.auto_stop_on_target = false;
      appCommandSubmitFromSafety(&stop_command);
    }
  }
}

#include "ch.h"

#include "app_fastloop.h"
#include "app_main.h"
#include "app_fault_mgr.h"
#include "app_state_machine.h"
#include "app_task_comm.h"
#include "app_task_control.h"
#include "app_task_ui.h"
#include "algo_foc.h"
#include "hal_board.h"
#include "hal_uart_if.h"

static app_runtime_t g_app_runtime;
static app_command_snapshot_t g_app_command = {
    {
        true,
        1200,
        600,
    },
    1U,
    APP_CMD_SRC_BOOT,
};

static void appCommandSubmitInternal(app_command_source_t source,
                                     const app_command_t *command) {

  if (command == NULL) {
    return;
  }

  chSysLock();
  g_app_command.value = *command;
  g_app_command.source = source;
  g_app_command.revision++;
  g_app_runtime.target_current_ma = command->target_current_ma;
  g_app_runtime.target_speed_rpm = command->target_speed_rpm;
  g_app_runtime.command_revision = g_app_command.revision;
  g_app_runtime.command_source = g_app_command.source;
  chSysUnlock();
}

void appRuntimeGetSnapshot(app_runtime_t *runtime) {

  if (runtime == NULL) {
    return;
  }

  chSysLock();
  *runtime = g_app_runtime;
  chSysUnlock();
}

void appRuntimeIncrementSeconds(void) {

  chSysLock();
  g_app_runtime.rtos_seconds++;
  chSysUnlock();
}

void appRuntimeIncrementControlTicks(void) {

  chSysLock();
  g_app_runtime.control_ticks++;
  chSysUnlock();
}

void appRuntimeIncrementCommTicks(void) {

  chSysLock();
  g_app_runtime.comm_ticks++;
  chSysUnlock();
}

void appRuntimePublishFastLoop(const app_runtime_fastloop_update_t *update) {

  if (update == NULL) {
    return;
  }

  chSysLock();
  g_app_runtime.bus_voltage_mv = update->bus_voltage_mv;
  g_app_runtime.phase_a_current_ma = update->phase_a_current_ma;
  g_app_runtime.duty_a_permille = update->duty_a_permille;
  g_app_runtime.fault_flags = update->fault_flags;
  g_app_runtime.state = update->state;
  g_app_runtime.target_current_ma = update->target_current_ma;
  g_app_runtime.target_speed_rpm = update->target_speed_rpm;
  g_app_runtime.mechanical_angle_decideg = update->mechanical_angle_decideg;
  g_app_runtime.encoder_raw = update->encoder_raw;
  g_app_runtime.mechanical_turn_count = update->mechanical_turn_count;
  g_app_runtime.encoder_ready = update->encoder_ready;
  g_app_runtime.command_revision = g_app_command.revision;
  g_app_runtime.command_source = g_app_command.source;
  chSysUnlock();
}

void appCommandGetSnapshot(app_command_snapshot_t *command_snapshot) {

  if (command_snapshot == NULL) {
    return;
  }

  chSysLock();
  *command_snapshot = g_app_command;
  chSysUnlock();
}

void appCommandSubmitFromUi(const app_command_t *command) {

  appCommandSubmitInternal(APP_CMD_SRC_UI, command);
}

void appCommandSubmitFromComm(const app_command_t *command) {

  appCommandSubmitInternal(APP_CMD_SRC_COMM, command);
}

void appCommandSubmitFromParam(const app_command_t *command) {

  appCommandSubmitInternal(APP_CMD_SRC_PARAM, command);
}

void appCommandSubmitFromSafety(const app_command_t *command) {

  appCommandSubmitInternal(APP_CMD_SRC_SAFETY, command);
}

void appStartSystemTick(void) {

  halUartWrite("[BOOT] systick cfg begin\r\n");
  SysTick->CTRL = 0U;
  SysTick->LOAD = SystemCoreClock / CH_CFG_ST_FREQUENCY - (systime_t)1;
  SysTick->VAL = 0U;
  NVIC_SetPriority(SysTick_IRQn, 8U);
  halUartWrite("[BOOT] systick cfg done\r\n");
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;
  halUartWrite("[BOOT] systick started\r\n");
}

void appInit(void) {

  halUartWrite("[BOOT] app init begin\r\n");
  appStateMachineInit();
  halUartWrite("[BOOT] state machine ok\r\n");
  appFaultMgrInit();
  halUartWrite("[BOOT] fault manager ok\r\n");
  halBoardInit();
  halUartWrite("[BOOT] board init ok\r\n");
  algoFocInit();
  halUartWrite("[BOOT] algo init ok\r\n");
  appFastLoopInit();
  halUartWrite("[BOOT] fastloop init ok\r\n");
  chSysInit();
  halUartWrite("[BOOT] chibios init ok\r\n");
  halKeysStart();
  appStartSystemTick();

  appTaskControlStart();
  halUartWrite("[BOOT] control thread start requested\r\n");
  appTaskUiStart();
  halUartWrite("[BOOT] ui thread start requested\r\n");

  while (true) {
    chThdSleepSeconds(1);
    appRuntimeIncrementSeconds();
  }
}

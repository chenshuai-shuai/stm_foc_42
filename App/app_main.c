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
  appStartSystemTick();

  appTaskControlStart();
  halUartWrite("[BOOT] control thread start requested\r\n");
  appTaskUiStart();
  halUartWrite("[BOOT] ui thread start requested\r\n");

  while (true) {
    chThdSleepSeconds(1);
    g_app_runtime.rtos_seconds++;
  }
}

const app_runtime_t *appGetRuntime(void) {

  return &g_app_runtime;
}

#include "drv_tim1_pwm.h"

#include <stddef.h>

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define DRV_PWM_TIM                         TIM1
#define DRV_PWM_TIM_CLK                     RCC_APB2Periph_TIM1
#define DRV_PWM_AFIO_CLK                    RCC_APB2Periph_AFIO
#define DRV_PWM_GPIOA_CLK                   RCC_APB2Periph_GPIOA
#define DRV_PWM_GPIOB_CLK                   RCC_APB2Periph_GPIOB

#define DRV_PWM_A_GPIO                      GPIOA
#define DRV_PWM_A_PIN                       GPIO_Pin_8
#define DRV_PWM_B_GPIO                      GPIOA
#define DRV_PWM_B_PIN                       GPIO_Pin_11

#define DRV_PHASE_A_NEG_GPIO                GPIOB
#define DRV_PHASE_A_NEG_PIN                 GPIO_Pin_10
#define DRV_PHASE_A_POS_GPIO                GPIOB
#define DRV_PHASE_A_POS_PIN                 GPIO_Pin_11
#define DRV_PHASE_B_NEG_GPIO                GPIOB
#define DRV_PHASE_B_NEG_PIN                 GPIO_Pin_13
#define DRV_PHASE_B_POS_GPIO                GPIOB
#define DRV_PHASE_B_POS_PIN                 GPIO_Pin_12

#define DRV_PWM_PERIOD_COUNTS               2399U

static drv_tim1_pwm_raw_t g_pwm_last;

static uint16_t drvTim1PwmToCcr(uint16_t duty_permille) {

  if (duty_permille >= 1000U) {
    return (uint16_t)(DRV_PWM_PERIOD_COUNTS + 1U);
  }

  return (uint16_t)(((uint32_t)(DRV_PWM_PERIOD_COUNTS + 1U) * duty_permille) /
                    1000U);
}

static void drvTim1SetPhaseState(bool enable,
                                 bool phase_a_forward,
                                 bool phase_b_forward) {

  if (enable == false) {
    GPIO_ResetBits(DRV_PHASE_A_NEG_GPIO, DRV_PHASE_A_NEG_PIN);
    GPIO_ResetBits(DRV_PHASE_A_POS_GPIO, DRV_PHASE_A_POS_PIN);
    GPIO_ResetBits(DRV_PHASE_B_NEG_GPIO, DRV_PHASE_B_NEG_PIN);
    GPIO_ResetBits(DRV_PHASE_B_POS_GPIO, DRV_PHASE_B_POS_PIN);
    return;
  }

  if (phase_a_forward) {
    GPIO_ResetBits(DRV_PHASE_A_NEG_GPIO, DRV_PHASE_A_NEG_PIN);
    GPIO_SetBits(DRV_PHASE_A_POS_GPIO, DRV_PHASE_A_POS_PIN);
  } else {
    GPIO_SetBits(DRV_PHASE_A_NEG_GPIO, DRV_PHASE_A_NEG_PIN);
    GPIO_ResetBits(DRV_PHASE_A_POS_GPIO, DRV_PHASE_A_POS_PIN);
  }

  if (phase_b_forward) {
    GPIO_ResetBits(DRV_PHASE_B_NEG_GPIO, DRV_PHASE_B_NEG_PIN);
    GPIO_SetBits(DRV_PHASE_B_POS_GPIO, DRV_PHASE_B_POS_PIN);
  } else {
    GPIO_SetBits(DRV_PHASE_B_NEG_GPIO, DRV_PHASE_B_NEG_PIN);
    GPIO_ResetBits(DRV_PHASE_B_POS_GPIO, DRV_PHASE_B_POS_PIN);
  }
}

void drvTim1PwmInit(void) {
  GPIO_InitTypeDef gpio_init;
  TIM_TimeBaseInitTypeDef tim_base;
  TIM_OCInitTypeDef tim_oc;

  g_pwm_last.duty_a_permille = 0U;
  g_pwm_last.duty_b_permille = 0U;
  g_pwm_last.phase_a_forward = true;
  g_pwm_last.phase_b_forward = true;
  g_pwm_last.enable = false;

  RCC_APB2PeriphClockCmd(DRV_PWM_AFIO_CLK |
                             DRV_PWM_GPIOA_CLK |
                             DRV_PWM_GPIOB_CLK |
                             DRV_PWM_TIM_CLK,
                         ENABLE);

  gpio_init.GPIO_Speed = GPIO_Speed_50MHz;

  gpio_init.GPIO_Pin = DRV_PWM_A_PIN | DRV_PWM_B_PIN;
  gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &gpio_init);

  gpio_init.GPIO_Pin = DRV_PHASE_A_NEG_PIN | DRV_PHASE_A_POS_PIN |
                       DRV_PHASE_B_NEG_PIN | DRV_PHASE_B_POS_PIN;
  gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio_init);
  drvTim1SetPhaseState(false, true, true);

  TIM_TimeBaseStructInit(&tim_base);
  tim_base.TIM_Prescaler = 0U;
  tim_base.TIM_CounterMode = TIM_CounterMode_Up;
  tim_base.TIM_Period = DRV_PWM_PERIOD_COUNTS;
  tim_base.TIM_ClockDivision = TIM_CKD_DIV1;
  tim_base.TIM_RepetitionCounter = 0U;
  TIM_TimeBaseInit(DRV_PWM_TIM, &tim_base);

  TIM_OCStructInit(&tim_oc);
  tim_oc.TIM_OCMode = TIM_OCMode_PWM1;
  tim_oc.TIM_OutputState = TIM_OutputState_Enable;
  tim_oc.TIM_Pulse = 0U;
  tim_oc.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(DRV_PWM_TIM, &tim_oc);
  TIM_OC4Init(DRV_PWM_TIM, &tim_oc);

  TIM_OC1PreloadConfig(DRV_PWM_TIM, TIM_OCPreload_Enable);
  TIM_OC4PreloadConfig(DRV_PWM_TIM, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig(DRV_PWM_TIM, ENABLE);
  TIM_CtrlPWMOutputs(DRV_PWM_TIM, ENABLE);
  TIM_Cmd(DRV_PWM_TIM, ENABLE);
}

void drvTim1PwmApply(const drv_tim1_pwm_raw_t *raw) {

  if (raw == NULL) {
    return;
  }

  g_pwm_last = *raw;
  TIM_SetCompare1(DRV_PWM_TIM,
                  raw->enable ? drvTim1PwmToCcr(raw->duty_a_permille) : 0U);
  TIM_SetCompare4(DRV_PWM_TIM,
                  raw->enable ? drvTim1PwmToCcr(raw->duty_b_permille) : 0U);
  drvTim1SetPhaseState(raw->enable,
                       raw->phase_a_forward,
                       raw->phase_b_forward);
}

void drvTim1PwmGetLast(drv_tim1_pwm_raw_t *raw) {

  if (raw == NULL) {
    return;
  }

  *raw = g_pwm_last;
}

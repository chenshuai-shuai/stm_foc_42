#include "drv_adc1.h"

#include <stddef.h>

#include "stm32f10x_adc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define DRV_ADC1_BUS_CHANNEL            ADC_Channel_0
#define DRV_ADC1_BUS_GPIO               GPIOA
#define DRV_ADC1_BUS_PIN                GPIO_Pin_0
#define DRV_ADC1_BUS_GPIO_CLK           RCC_APB2Periph_GPIOA
#define DRV_ADC1_ADC_CLK                RCC_APB2Periph_ADC1
#define DRV_ADC1_VREF_MV                3300UL
#define DRV_ADC1_FULL_SCALE             4095UL
#define DRV_ADC1_DIVIDER_NUMERATOR      17UL
#define DRV_ADC1_DIVIDER_DENOMINATOR    2UL

static drv_adc1_raw_t g_adc1_raw;

void drvAdc1Init(void) {
  ADC_InitTypeDef adc_init;
  GPIO_InitTypeDef gpio_init;

  RCC_APB2PeriphClockCmd(DRV_ADC1_BUS_GPIO_CLK | DRV_ADC1_ADC_CLK, ENABLE);
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  gpio_init.GPIO_Pin = DRV_ADC1_BUS_PIN;
  gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
  gpio_init.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(DRV_ADC1_BUS_GPIO, &gpio_init);

  ADC_DeInit(ADC1);
  adc_init.ADC_Mode = ADC_Mode_Independent;
  adc_init.ADC_ScanConvMode = DISABLE;
  adc_init.ADC_ContinuousConvMode = DISABLE;
  adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  adc_init.ADC_DataAlign = ADC_DataAlign_Right;
  adc_init.ADC_NbrOfChannel = 1U;
  ADC_Init(ADC1, &adc_init);

  ADC_RegularChannelConfig(ADC1,
                           DRV_ADC1_BUS_CHANNEL,
                           1U,
                           ADC_SampleTime_239Cycles5);
  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1) != RESET) {
  }

  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1) != RESET) {
  }

  g_adc1_raw.phase_a_current_ma = 0;
  g_adc1_raw.phase_b_current_ma = 0;
  g_adc1_raw.bus_voltage_mv = 0U;
}

void drvAdc1GetLatest(drv_adc1_raw_t *raw) {
  uint16_t adc_code;
  uint32_t adc_mv;

  if (raw == NULL) {
    return;
  }

  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
  }
  adc_code = ADC_GetConversionValue(ADC1);
  ADC_ClearFlag(ADC1, ADC_FLAG_EOC);

  adc_mv = ((uint32_t)adc_code * DRV_ADC1_VREF_MV) / DRV_ADC1_FULL_SCALE;
  g_adc1_raw.bus_voltage_mv =
      (uint16_t)((adc_mv * DRV_ADC1_DIVIDER_NUMERATOR) /
                 DRV_ADC1_DIVIDER_DENOMINATOR);

  *raw = g_adc1_raw;
}

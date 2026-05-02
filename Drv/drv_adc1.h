#ifndef DRV_ADC1_H
#define DRV_ADC1_H

#include <stdint.h>

typedef struct {
  int16_t phase_a_current_ma;
  int16_t phase_b_current_ma;
  uint16_t bus_voltage_mv;
} drv_adc1_raw_t;

void drvAdc1Init(void);
void drvAdc1GetLatest(drv_adc1_raw_t *raw);

#endif /* DRV_ADC1_H */

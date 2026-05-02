#include "drv_adc1.h"

#include <stddef.h>

static drv_adc1_raw_t g_adc1_raw;
static int16_t g_phase_a_step;
static int16_t g_phase_b_step;

void drvAdc1Init(void) {

  g_adc1_raw.phase_a_current_ma = 200;
  g_adc1_raw.phase_b_current_ma = -150;
  g_adc1_raw.bus_voltage_mv = 24000U;
  g_adc1_raw.electrical_angle_deg = 0;
  g_adc1_raw.mechanical_speed_rpm = 300;
  g_phase_a_step = 25;
  g_phase_b_step = -20;
}

void drvAdc1GetLatest(drv_adc1_raw_t *raw) {

  if (raw == NULL) {
    return;
  }

  g_adc1_raw.phase_a_current_ma += g_phase_a_step;
  g_adc1_raw.phase_b_current_ma += g_phase_b_step;
  g_adc1_raw.electrical_angle_deg += 6;
  if (g_adc1_raw.electrical_angle_deg >= 360) {
    g_adc1_raw.electrical_angle_deg -= 360;
  }

  if ((g_adc1_raw.phase_a_current_ma > 800) ||
      (g_adc1_raw.phase_a_current_ma < -800)) {
    g_phase_a_step = (int16_t)-g_phase_a_step;
  }

  if ((g_adc1_raw.phase_b_current_ma > 700) ||
      (g_adc1_raw.phase_b_current_ma < -700)) {
    g_phase_b_step = (int16_t)-g_phase_b_step;
  }

  *raw = g_adc1_raw;
}

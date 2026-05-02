#include "algo_angle.h"

#define ALGO_ANGLE_DECIDEG_PER_TURN 3600UL

static algo_angle_config_t g_angle_config = {
    16384U,
    50U,
    1,
    0U,
};

static uint16_t algoAngleApplyEncoderTransform(uint16_t encoder_raw) {
  int32_t corrected_raw;
  int32_t cpr;

  cpr = (int32_t)g_angle_config.encoder_cpr;
  corrected_raw = (int32_t)encoder_raw - (int32_t)g_angle_config.encoder_zero_raw;

  if (g_angle_config.encoder_direction < 0) {
    corrected_raw = -corrected_raw;
  }

  while (corrected_raw < 0) {
    corrected_raw += cpr;
  }

  while (corrected_raw >= cpr) {
    corrected_raw -= cpr;
  }

  return (uint16_t)corrected_raw;
}

void algoAngleInit(const algo_angle_config_t *config) {

  if (config == 0) {
    return;
  }

  g_angle_config = *config;

  if (g_angle_config.encoder_cpr == 0U) {
    g_angle_config.encoder_cpr = 16384U;
  }

  if (g_angle_config.pole_pairs == 0U) {
    g_angle_config.pole_pairs = 1U;
  }

  if (g_angle_config.encoder_direction == 0) {
    g_angle_config.encoder_direction = 1;
  }

  g_angle_config.encoder_zero_raw %= g_angle_config.encoder_cpr;
}

uint16_t algoAngleGetMechanicalAngleDecideg(uint16_t encoder_raw) {
  uint16_t corrected_raw;

  corrected_raw = algoAngleApplyEncoderTransform(encoder_raw);
  return (uint16_t)(((uint32_t)corrected_raw * ALGO_ANGLE_DECIDEG_PER_TURN) /
                    (uint32_t)g_angle_config.encoder_cpr);
}

uint16_t algoAngleGetElectricalAngleDecideg(uint16_t encoder_raw) {
  uint32_t electrical_angle_decideg;
  uint16_t mechanical_angle_decideg;

  mechanical_angle_decideg = algoAngleGetMechanicalAngleDecideg(encoder_raw);
  electrical_angle_decideg =
      (uint32_t)mechanical_angle_decideg * (uint32_t)g_angle_config.pole_pairs;

  return (uint16_t)(electrical_angle_decideg % ALGO_ANGLE_DECIDEG_PER_TURN);
}

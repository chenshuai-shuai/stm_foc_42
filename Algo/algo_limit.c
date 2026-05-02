#include "algo_limit.h"

int32_t algoLimitS32(int32_t value, int32_t low, int32_t high) {

  if (value < low) {
    return low;
  }

  if (value > high) {
    return high;
  }

  return value;
}

uint16_t algoLimitU16(uint16_t value, uint16_t low, uint16_t high) {

  if (value < low) {
    return low;
  }

  if (value > high) {
    return high;
  }

  return value;
}

#include <limits.h>
#include <stdint.h>

__attribute__((always_inline)) static inline int16_t fmult16signed(int16_t val,
                                                                   float mul) {
  int32_t intermediate = (int32_t)(val * mul);
  if (intermediate > SHRT_MAX) {
    return SHRT_MAX;
  } else if (intermediate < SHRT_MIN) {
    return SHRT_MIN;
  }
  return (int16_t)intermediate;
}

__attribute__((always_inline)) static inline int16_t add16signed(int16_t a,
                                                                 int16_t b) {
  int32_t intermediate = (int32_t)a + b;
  if (intermediate > SHRT_MAX) {
    return SHRT_MAX;
  } else if (intermediate < SHRT_MIN) {
    return SHRT_MIN;
  }

  return (int16_t)intermediate;
}

extern "C" {
#include "modcanopy.h"
}

#define NUMSTRIPS 2
#include "I2SClocklessDriver.h"
#include <cstdint>
#define FASTLED_NO_MCU
#include <fastled.h>

//#ifndef NO_QSTR
#include <canopy.h>
//#endif

// #ifndef COUNT
// #define COUNT(x) (sizeof(x) / sizeof(x[0]))
// #endif

// const int LEDS_PER_STRIP = 300;
// int pins[] = {8, 17};
// uint8_t leds[3 * COUNT(pins) * LEDS_PER_STRIP];

typedef struct _canopy_pattern_obj_t {
  mp_obj_base_t base;
  std::unique_ptr<Pattern> pattern;
} canopy_pattern_obj_t;

I2SClocklessLedDriveresp32S3 leddriver;
CRGB *leds = NULL;
size_t nChannels = 0;
size_t nLedsPerChannel = 0;

extern "C" const mp_obj_type_t canopy_pattern_type;

extern "C" mp_obj_t canopy_pattern_make_new(const mp_obj_type_t *type,
                                            size_t n_args, size_t n_kw,
                                            const mp_obj_t *args) {

  // verify we have at least one arg
  if (n_args < 1) {
    mp_raise_TypeError("expected at least 1 argument");
  }

  // assume first arg is a string with the pattern config
  const char *pattern_config = mp_obj_str_get_str(args[0]);

  canopy_pattern_obj_t *self = m_new_obj_with_finaliser(canopy_pattern_obj_t);
  self->base.type = &canopy_pattern_type;
  self->pattern = std::make_unique<Pattern>(Pattern::load(pattern_config));

  ESP_LOGI("canopy", "Pattern loaded");

  return MP_OBJ_FROM_PTR(self);
}

extern "C" mp_obj_t canopy_pattern_deinit(mp_obj_t self_in) {
  canopy_pattern_obj_t *self = (canopy_pattern_obj_t *)self_in;
  ESP_LOGI("canopy", "Deinit pattern");
  self->pattern.reset();
  return mp_const_none;
}

extern "C" mp_obj_t canopy_init(mp_obj_t pins, mp_obj_t ledsPerChannel) {
  ESP_LOGI("canopy", "Init");

  // pins should be iterable, ledsPerChannel a number
  // make sure pins is iterable

  // get a c-style array from the pins iterable
  mp_obj_t *pinObjs;
  mp_obj_get_array(pins, &nChannels, &pinObjs);
  int pinArray[nChannels];
  for (int i = 0; i < nChannels; i++) {
    pinArray[i] = mp_obj_get_int(pinObjs[i]);
  }

  // get the number of leds per channel
  nLedsPerChannel = mp_obj_get_int(ledsPerChannel);

  // allocate backbuffer
  printf("Allocating leds %d LEDs %d bytes\n", nChannels * nLedsPerChannel,
         3 * nChannels * nLedsPerChannel);
  leds = (CRGB *)malloc(3 * nChannels * nLedsPerChannel);

  // init
  leddriver.initled((uint8_t *)leds, pinArray, nChannels, nLedsPerChannel);
  return mp_const_none;
}

extern "C" mp_obj_t canopy_clear() {
  if (leds == NULL) {
    return mp_const_none;
  }
  memset(leds, 0, 3 * nChannels * nLedsPerChannel);
  return mp_const_none;
}

extern "C" mp_obj_t canopy_render() {
  if (leds == NULL) {
    return mp_const_none;
  }
  leddriver.show();
  return mp_const_none;
}

extern "C" mp_obj_t canopy_draw(mp_obj_t segment, mp_obj_t pattern,
                                mp_obj_t params) {
  if (leds == NULL) {
    return mp_const_none;
  }

  // segment is tuple of (channel, start, length)

  mp_obj_t *segmentItems;
  size_t segmentElemCount;
  mp_obj_get_array(segment, &segmentElemCount, &segmentItems);

  // verify number of elements in tuple
  if (3 != segmentElemCount) {
    mp_raise_ValueError("segment should be (channel, start, length)");
  }

  int channel = mp_obj_get_int(segmentItems[0]);
  int start = mp_obj_get_int(segmentItems[1]);
  int length = mp_obj_get_int(segmentItems[2]);

  // make sure segment is valid
  if (channel < 0 || channel >= nChannels) {
    mp_raise_ValueError("channel out of range");
  }

  if (start < 0 || start >= nLedsPerChannel) {
    mp_raise_ValueError("start out of range");
  }

  if (length < 0 || start + length > nLedsPerChannel) {
    mp_raise_ValueError("length out of range");
  }

  // pattern is a Pattern object
  canopy_pattern_obj_t *self = (canopy_pattern_obj_t *)pattern;

  // load params from params dict
  Params p;
  if (params != mp_const_none) {
    // mp_map_t *paramsMap = mp_obj_get_map(params);
    // for (int i = 0; i < paramsMap->used; i++) {
    //   mp_map_elem_t *elem = &paramsMap->table[i];
    //   const char *key = mp_obj_str_get_str(elem->key);
    //   // value can be a float or a string
    //   if (mp_obj_is_float(elem->value)) {
    //     p[key] = mp_obj_get_float(elem->value);
    //   } else if (mp_obj_is_str(elem->value)) {
    //     p[key] = mp_obj_str_get_str(elem->value);
    //   }
    // }
  }

  // alpha is a float
  float alphaValue = 1.0f;
  Segment seg(&leds[channel * nLedsPerChannel + start], length, PixelMapping::linearMap(length));
  self->pattern->render(seg, p, alphaValue);

  return mp_const_none;
}

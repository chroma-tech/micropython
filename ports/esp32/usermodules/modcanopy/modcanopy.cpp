extern "C" {
#include "modcanopy.h"
}

// #define NUMSTRIPS 2
// #include "I2SClocklessDriver.h"
// #include <cstdint>
#define FASTLED_NO_MCU
#include "driver.h"
#include <fastled.h>

// #ifndef NO_QSTR
#include <canopy.h>
// #endif

// #ifndef COUNT
// #define COUNT(x) (sizeof(x) / sizeof(x[0]))
// #endif

// const int LEDS_PER_STRIP = 300;
// int pins[] = {8, 17};
// uint8_t leds[3 * COUNT(pins) * LEDS_PER_STRIP];

typedef struct _canopy_pattern_obj_t {
  mp_obj_base_t base;
  mp_obj_t params;
  std::unique_ptr<Pattern> pattern;
} canopy_pattern_obj_t;

typedef struct _canopy_segment_obj_t {
  mp_obj_base_t base;
  std::unique_ptr<Segment> segment;
} canopy_segment_obj_t;

S3ClocklessDriver leddriver;
CRGBOut out;
CRGB *leds = NULL;
size_t nChannels = 0;
size_t nLedsPerChannel = 0;

extern "C" const mp_obj_type_t canopy_pattern_type;
extern "C" const mp_obj_type_t canopy_segment_type;

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

  // printf("Pattern %s has %d layers, %d scalars, %d palettes\n",
  //        self->pattern->name.c_str(), self->pattern->layers.size(),
  //        self->pattern->params.scalars.size(),
  //        self->pattern->params.palettes.size());

  // create a params dict and load it with the params from the pattern
  self->params = mp_obj_new_dict(0);
  for (auto &scalar : self->pattern->params.scalars) {
    if (scalar.second->isConstant()) {
      mp_obj_dict_store(
          self->params,
          mp_obj_new_str(scalar.first.c_str(), scalar.first.size()),
          mp_obj_new_float(scalar.second->value()));
    }
  }

  return MP_OBJ_FROM_PTR(self);
}

extern "C" mp_obj_t canopy_pattern_deinit(mp_obj_t self_in) {
  canopy_pattern_obj_t *self = (canopy_pattern_obj_t *)self_in;
  self->pattern.reset();
  return mp_const_none;
}

extern "C" void canopy_pattern_attr(mp_obj_t self_in, qstr attr,
                                    mp_obj_t *dest) {
  canopy_pattern_obj_t *self = (canopy_pattern_obj_t *)self_in;
  if (attr == MP_QSTR_params) {
    dest[0] = self->params;
  } else {
    dest[0] = MP_OBJ_NULL;
  }
}

extern "C" mp_obj_t canopy_segment_make_new(const mp_obj_type_t *type,
                                            size_t n_args, size_t n_kw,
                                            const mp_obj_t *args) {

  if (n_args < 3) {
    mp_raise_TypeError(
        "expected at least 3 arguments (channel, start, length)");
  }

  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }

  int channel = mp_obj_get_int(args[0]);
  int start = mp_obj_get_int(args[1]);
  int length = mp_obj_get_int(args[2]);

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

  canopy_segment_obj_t *self = m_new_obj_with_finaliser(canopy_segment_obj_t);
  self->base.type = &canopy_segment_type;
  self->segment =
      std::make_unique<Segment>(&leds[channel * nLedsPerChannel + start],
                                length, PixelMapping::linearMap(length));

  return MP_OBJ_FROM_PTR(self);
}

extern "C" mp_obj_t canopy_segment_deinit(mp_obj_t self_in) {
  canopy_segment_obj_t *self = (canopy_segment_obj_t *)self_in;
  self->segment.reset();
  return mp_const_none;
}

extern "C" mp_obj_t canopy_init(mp_obj_t pins, mp_obj_t ledsPerChannel) {
  ESP_LOGI("canopy", "Init");

  static bool initialized = false;
  if (initialized) {
    if (leds != NULL) {
      free(leds);
      leds = NULL;
    }
    leddriver.end();
    initialized = false;
  }

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
  leds = (CRGB *)malloc(3 * nChannels * nLedsPerChannel);

  // init
  leddriver.begin(pinArray, nChannels, nLedsPerChannel);
  initialized = true;

  return mp_const_none;
}

extern "C" mp_obj_t canopy_clear() {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }
  memset(leds, 0, 3 * nChannels * nLedsPerChannel);
  return mp_const_none;
}

extern "C" mp_obj_t canopy_render() {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }
  leddriver.show((uint8_t *)leds, out);
  return mp_const_none;
}

extern "C" mp_obj_t canopy_draw(mp_obj_t segment, mp_obj_t pattern, mp_obj_t alpha) {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }

  // segment is a Segment object
  canopy_segment_obj_t *segment_obj = (canopy_segment_obj_t *)segment;

  // pattern is a Pattern object
  canopy_pattern_obj_t *pattern_obj = (canopy_pattern_obj_t *)pattern;

  // alpha is float with default of 1.0
  float alphaValue = mp_obj_get_float(alpha);

  // load params from params dict
  Params p;
  mp_map_t *params = mp_obj_dict_get_map(pattern_obj->params);

  for (size_t i = 0; i < params->alloc; i++) {
    if (mp_map_slot_is_filled(params, i)) {
      mp_map_elem_t *elem = &(params->table[i]);
      const char *key = mp_obj_str_get_str(elem->key);
      float val = mp_obj_get_float(elem->value);
      p.scalar(key)->value(mp_obj_get_float(elem->value));
    }
  }

  if (alphaValue > 0.0) {
    pattern_obj->pattern->render(*(segment_obj->segment), p, alphaValue);
  }

  return mp_const_none;
}

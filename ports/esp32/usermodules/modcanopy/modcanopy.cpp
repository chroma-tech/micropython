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

typedef struct _canopy_params_obj_t {
  mp_obj_base_t base;
  std::unique_ptr<Params> params;
} canopy_params_obj_t;

S3ClocklessDriver leddriver;
CRGBOut out;
CRGB *leds = NULL;
size_t nChannels = 0;
size_t nLedsPerChannel = 0;

extern "C" const mp_obj_type_t canopy_pattern_type;
extern "C" const mp_obj_type_t canopy_segment_type;
extern "C" const mp_obj_type_t canopy_params_type;

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
                                length, PixelMapping16::linearMap(length));

  return MP_OBJ_FROM_PTR(self);
}

extern "C" mp_obj_t canopy_segment_deinit(mp_obj_t self_in) {
  canopy_segment_obj_t *self = (canopy_segment_obj_t *)self_in;
  self->segment.reset();
  return mp_const_none;
}

extern "C" mp_obj_t canopy_params_make_new(const mp_obj_type_t *type,
                                           size_t n_args, size_t n_kw,
                                           const mp_obj_t *args) {
  canopy_params_obj_t *self = m_new_obj(canopy_params_obj_t);
  self->base.type = type;

  self->params = std::make_unique<Params>();

  // if we have a dict, load it into the params
  if (n_args == 1) {
    mp_map_t *params = mp_obj_dict_get_map(args[0]);
    if (params != NULL) {
      for (size_t i = 0; i < params->alloc; i++) {
        if (mp_map_slot_is_filled(params, i)) {
          mp_map_elem_t *elem = &(params->table[i]);
          const char *key = mp_obj_str_get_str(elem->key);
          float val = mp_obj_get_float(elem->value);
          self->params->scalar(key)->value(val);
        }
      }
    }
  }

  return MP_OBJ_FROM_PTR(self);
}

extern "C" mp_obj_t canopy_params_deinit(mp_obj_t self_in) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)self_in;
  self->params.reset();
  return mp_const_none;
}

// keys() method
extern "C" mp_obj_t canopy_params_keys(mp_obj_t self_in) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)MP_OBJ_TO_PTR(self_in);
  mp_obj_list_t *keys =
      (mp_obj_list_t *)MP_OBJ_TO_PTR(mp_obj_new_list(0, NULL));
  for (const auto &pair : self->params->scalars) {
    mp_obj_list_append(MP_OBJ_FROM_PTR(keys),
                       mp_obj_new_str(pair.first.c_str(), pair.first.size()));
  }
  return MP_OBJ_FROM_PTR(keys);
}

// values() method
extern "C" mp_obj_t canopy_params_values(mp_obj_t self_in) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)MP_OBJ_TO_PTR(self_in);
  mp_obj_list_t *values =
      (mp_obj_list_t *)MP_OBJ_TO_PTR(mp_obj_new_list(0, NULL));
  for (const auto &pair : self->params->scalars) {
    mp_obj_list_append(MP_OBJ_FROM_PTR(values),
                       mp_obj_new_float(pair.second->value()));
  }
  return MP_OBJ_FROM_PTR(values);
}

// items() method
extern "C" mp_obj_t canopy_params_items(mp_obj_t self_in) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)MP_OBJ_TO_PTR(self_in);
  mp_obj_list_t *items =
      (mp_obj_list_t *)MP_OBJ_TO_PTR(mp_obj_new_list(0, NULL));
  for (const auto &pair : self->params->scalars) {
    mp_obj_tuple_t *tuple =
        (mp_obj_tuple_t *)MP_OBJ_TO_PTR(mp_obj_new_tuple(2, NULL));
    tuple->items[0] = mp_obj_new_str(pair.first.c_str(), pair.first.size());
    tuple->items[1] = mp_obj_new_float(pair.second->value());
    mp_obj_list_append(MP_OBJ_FROM_PTR(items), MP_OBJ_FROM_PTR(tuple));
  }
  return MP_OBJ_FROM_PTR(items);
}

// contains() method
extern "C" mp_obj_t canopy_params_contains(mp_obj_t self_in, mp_obj_t key) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)MP_OBJ_TO_PTR(self_in);
  const char *scalarkey = mp_obj_str_get_str(key);
  if (self->params->scalars.find(scalarkey) != self->params->scalars.end()) {
    return mp_const_true;
  }
  return mp_const_false;
}

extern "C" mp_obj_t canopy_params_subscr(mp_obj_t self_in, mp_obj_t index,
                                         mp_obj_t value) {
  canopy_params_obj_t *self = (canopy_params_obj_t *)MP_OBJ_TO_PTR(self_in);
  const char *key = mp_obj_str_get_str(index);

  if (value == MP_OBJ_NULL) {
    // Delete item
    auto it = self->params->scalars.find(key);
    if (it != self->params->scalars.end()) {
      self->params->scalars.erase(it);
    }
    return mp_const_none;
  } else if (value == MP_OBJ_SENTINEL) {
    // Get item
    auto val = (*self->params)[std::string(key)];
    if (val == nullptr) {
      mp_raise_msg(&mp_type_KeyError, "key not found");
    } else {
      return mp_obj_new_float(val->value());
    }
  } else {
    // Set item
    self->params->scalar(key)->value(mp_obj_get_float(value));
    return mp_const_none;
  }
}

extern "C" mp_obj_t canopy_init(mp_obj_t pins, mp_obj_t ledsPerChannel) {
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

extern "C" mp_obj_t canopy_draw(size_t n_args, const mp_obj_t *args,
                                mp_map_t *kw_args) {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }

  // segment is a Segment object. raise exception if it's the wrong type
  if (!MP_OBJ_IS_TYPE(args[0], &canopy_segment_type)) {
    mp_raise_TypeError("expected Segment object");
  }
  canopy_segment_obj_t *segment_obj = (canopy_segment_obj_t *)args[0];

  // pattern is a Pattern object. raise exception if it's the wrong type
  if (!MP_OBJ_IS_TYPE(args[1], &canopy_pattern_type)) {
    mp_raise_TypeError("expected Pattern object");
  }
  canopy_pattern_obj_t *pattern_obj = (canopy_pattern_obj_t *)args[1];

  // alpha is float with default of 1.0
  float alphaValue = 1.0f;

  // get opacity from kwargs
  mp_obj_t alpha =
      mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_alpha), MP_MAP_LOOKUP);
  if (alpha != MP_OBJ_NULL) {
    alphaValue = mp_obj_get_float(alpha);
  }

  static Params noparams;
  Params *p = &noparams;

  if (n_args > 2) {
    if (MP_OBJ_IS_TYPE(args[2], &canopy_params_type)) {
      canopy_params_obj_t *params_obj = (canopy_params_obj_t *)args[2];
      p = params_obj->params.get();
    }

    // TODO: convert dict to Params
    // load params from params dict
    // Params p;
    // mp_map_t *params = mp_obj_dict_get_map(pattern_obj->params);

    // for (size_t i = 0; i < params->alloc; i++) {
    //   if (mp_map_slot_is_filled(params, i)) {
    //     mp_map_elem_t *elem = &(params->table[i]);
    //     const char *key = mp_obj_str_get_str(elem->key);
    //     float val = mp_obj_get_float(elem->value);
    //     p.scalar(key)->value(mp_obj_get_float(elem->value));
    //   }
    // }
  }

  if (alphaValue > 0.0) {
    pattern_obj->pattern->render(*(segment_obj->segment), *p, alphaValue);
  }

  return mp_const_none;
}

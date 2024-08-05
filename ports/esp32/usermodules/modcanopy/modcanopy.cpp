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

typedef S3ClocklessDriver<8, 3> CanopyClocklessDriver;
typedef NewS3ClockedDriver<ColorDepth::C8Bit, SpiWidth::W8Bit,
                           ElementsPerPixel::E4>
    CanopyClockedDriver;

std::unique_ptr<CanopyClocklessDriver> driver_clockless = nullptr;
std::unique_ptr<CanopyClockedDriver> driver_clocked = nullptr;

CRGBOut out;
CRGB *leds = NULL;
size_t nChannels = 0;
size_t nLedsPerChannel = 0;

static bool initialized = false;

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

  bool reverse = false;
  if (n_args > 3) {
    reverse = mp_obj_is_true(args[3]);
  }

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
  self->segment = std::make_unique<Segment>(
      &leds[channel * nLedsPerChannel + start], length,
      PixelMapping16::linearMap(length, reverse));

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

extern "C" mp_obj_t canopy_init(size_t n_args, const mp_obj_t *pos_args,
                                mp_map_t *kw_args) {

  enum { ARG_pins, ARG_ledsPerChannel, ARG_clkPin, ARG_order };
  static const mp_arg_t allowed_args[] = {
      {MP_QSTR_pins, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_NONE}},
      {MP_QSTR_ledsPerChannel,
       MP_ARG_OBJ | MP_ARG_REQUIRED,
       {.u_rom_obj = mp_obj_new_int(100)}},
      {MP_QSTR_clk, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
      {MP_QSTR_order, MP_ARG_OBJ, {.u_rom_obj = mp_obj_new_int(BGR)}},
  };

  // parse args
  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
                   allowed_args, args);

  if (initialized) {
    if (leds != NULL) {
      free(leds);
      leds = NULL;
    }
    if (driver_clockless != nullptr) {
      driver_clockless->end();
      driver_clockless = nullptr;
    }
    if (driver_clocked != nullptr) {
      driver_clocked->end();
      driver_clocked = nullptr;
    }
    initialized = false;
  }

  // get a c-style array from the pins iterable
  mp_obj_t pins = args[ARG_pins].u_obj;
  mp_obj_t *pinObjs;
  mp_obj_get_array(pins, &nChannels, &pinObjs);
  int pinArray[nChannels];
  for (int i = 0; i < nChannels; i++) {
    pinArray[i] = mp_obj_get_int(pinObjs[i]);
  }

  // get the number of leds per channel
  nLedsPerChannel = mp_obj_get_int(args[ARG_ledsPerChannel].u_obj);

  // allocate backbuffer
  leds = (CRGB *)malloc(3 * nChannels * nLedsPerChannel);

  // get the color order
  EOrder order = (EOrder)mp_obj_get_int(args[ARG_order].u_obj);
  out.order = order;

  // init clocked if we have a valid clkpin, otherwise clockless
  if (args[ARG_clkPin].u_obj != MP_ROM_NONE) {
    int clkPin = mp_obj_get_int(args[ARG_clkPin].u_obj);
    driver_clocked = std::make_unique<CanopyClockedDriver>();
    driver_clocked->begin(pinArray, nChannels, nLedsPerChannel, clkPin);
  } else {
    driver_clockless = std::make_unique<CanopyClocklessDriver>();
    driver_clockless->begin(pinArray, nChannels, nLedsPerChannel);
  }

  initialized = true;
  return mp_const_none;
}

extern "C" mp_obj_t canopy_end() {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }

  if (driver_clockless != nullptr) {
    driver_clockless->end();
    driver_clockless = nullptr;
  }
  if (driver_clocked != nullptr) {
    driver_clocked->end();
    driver_clocked = nullptr;
  }
  if (leds != NULL) {
    free(leds);
    leds = NULL;
  }
  initialized = false;

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
  if (driver_clockless != nullptr) {
    driver_clockless->show((uint8_t *)leds, out);
  }
  if (driver_clocked != nullptr) {
    driver_clocked->show((uint8_t *)leds, out);
  }
  return mp_const_none;
}

extern "C" mp_obj_t canopy_draw(size_t n_args, const mp_obj_t *pos_args,
                                mp_map_t *kw_args) {
  if (leds == NULL) {
    mp_raise_msg(&mp_type_RuntimeError, "canopy hasn't been initialized");
  }

  enum { ARG_segment, ARG_pattern, ARG_alpha, ARG_params };
  static const mp_arg_t allowed_args[] = {
      {MP_QSTR_segment,
       MP_ARG_OBJ | MP_ARG_REQUIRED,
       {.u_rom_obj = MP_ROM_NONE}},
      {MP_QSTR_pattern,
       MP_ARG_OBJ | MP_ARG_REQUIRED,
       {.u_rom_obj = MP_ROM_NONE}},
      {MP_QSTR_alpha, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
      {MP_QSTR_params, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_rom_obj = MP_ROM_NONE}},
  };
  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
                   allowed_args, args);

  // segment is a Segment object. raise exception if it's the wrong type
  if (!MP_OBJ_IS_TYPE(args[ARG_segment].u_obj, &canopy_segment_type)) {
    mp_raise_TypeError("expected Segment object");
  }
  canopy_segment_obj_t *segment_obj =
      (canopy_segment_obj_t *)args[ARG_segment].u_obj;

  // pattern is a Pattern object. raise exception if it's the wrong type
  if (!MP_OBJ_IS_TYPE(args[ARG_pattern].u_obj, &canopy_pattern_type)) {
    mp_raise_TypeError("expected Pattern object");
  }
  canopy_pattern_obj_t *pattern_obj =
      (canopy_pattern_obj_t *)args[ARG_pattern].u_obj;

  // alpha is float with default of 1.0
  float alphaValue = 1.0f;
  if (mp_obj_is_float(args[ARG_alpha].u_obj)) {
    alphaValue = mp_obj_get_float(args[ARG_alpha].u_obj);
  }

  static Params noparams;
  Params *p = &noparams;

  if (MP_OBJ_IS_TYPE(args[ARG_params].u_obj, &canopy_params_type)) {
    canopy_params_obj_t *params_obj =
        (canopy_params_obj_t *)args[ARG_params].u_obj;
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

  if (alphaValue > 0.0) {
    pattern_obj->pattern->render(*(segment_obj->segment), *p, alphaValue);
  }

  return mp_const_none;
}

// extern "C" void canopy_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
//   // get
//   if (dest[0] == MP_OBJ_NULL) {
//     if (attr == MP_QSTR_brightness) {
//       dest[0] = mp_obj_new_float(out.brightness / 255.0);
//     } else {
//       dest[1] = MP_OBJ_SENTINEL;
//     }
//   } else {
//     if (attr == MP_QSTR_brightness) {
//       float brightness = mp_obj_get_float(dest[1]);
//       if (brightness < 0.0 || brightness > 1.0) {
//         mp_raise_ValueError("brightness must be between 0.0 and 1.0");
//       }
//       out.brightness = brightness * 255;
//     }
//     dest[0] = MP_OBJ_NULL;
//   }
// }

// extern "C" mp_obj_t canopy_brightness_getter() {
//   return mp_obj_new_float(out.brightness / 255.0);
// }

// extern "C" mp_obj_t canopy_brightness_setter(mp_obj_t value) {
//   float brightness = mp_obj_get_float(value);
//   if (brightness < 0.0 || brightness > 1.0) {
//     mp_raise_ValueError("brightness must be between 0.0 and 1.0");
//   }
//   out.brightness = brightness * 255;
//   return mp_const_none;
// }

extern "C" mp_obj_t canopy_brightness(size_t n_args, const mp_obj_t *args,
                                      mp_map_t *kw_args) {
  if (n_args == 0) {
    return mp_obj_new_float(out.brightness / 255.0);
  } else {
    float brightness = mp_obj_get_float(args[0]);
    if (brightness < 0.0 || brightness > 1.0) {
      mp_raise_ValueError("brightness must be between 0.0 and 1.0");
    }
    out.brightness = brightness * 255.0;
    return mp_const_none;
  }
}

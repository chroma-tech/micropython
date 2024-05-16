#pragma once

#include "py/runtime.h"

// pattern object
extern mp_obj_t canopy_pattern_make_new(const mp_obj_type_t *type,
                                        size_t n_args, size_t n_kw,
                                        const mp_obj_t *args);
extern mp_obj_t canopy_pattern_deinit(mp_obj_t self_in);
extern void canopy_pattern_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

// segment object
extern mp_obj_t canopy_segment_make_new(const mp_obj_type_t *type,
                                        size_t n_args, size_t n_kw,
                                        const mp_obj_t *args);
extern mp_obj_t canopy_segment_deinit(mp_obj_t self_in);

// params object
extern mp_obj_t canopy_params_make_new(const mp_obj_type_t *type, size_t n_args,
                                       size_t n_kw, const mp_obj_t *args);
extern mp_obj_t canopy_params_deinit(mp_obj_t self_in);
extern mp_obj_t canopy_params_keys(mp_obj_t self_in);
extern mp_obj_t canopy_params_values(mp_obj_t self_in);
extern mp_obj_t canopy_params_items(mp_obj_t self_in);
extern mp_obj_t canopy_params_contains(mp_obj_t self_in, mp_obj_t key);
extern mp_obj_t canopy_params_subscr(mp_obj_t self_in, mp_obj_t index,
                                     mp_obj_t value);

// canopy module
extern mp_obj_t canopy_init(mp_obj_t pins, mp_obj_t ledsPerChannel);
extern mp_obj_t canopy_render();
extern mp_obj_t canopy_clear();
extern mp_obj_t canopy_draw(size_t n_args, const mp_obj_t *args,
                            mp_map_t *kw_args);
extern void canopy_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

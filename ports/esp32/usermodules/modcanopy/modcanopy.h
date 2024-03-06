#pragma once

#include "py/runtime.h"

extern mp_obj_t canopy_pattern_make_new(const mp_obj_type_t *type,
                                        size_t n_args, size_t n_kw,
                                        const mp_obj_t *args);
extern mp_obj_t canopy_pattern_deinit(mp_obj_t self_in);
extern mp_obj_t canopy_init(mp_obj_t pins, mp_obj_t ledsPerChannel);
extern mp_obj_t canopy_render();
extern mp_obj_t canopy_clear();
extern mp_obj_t canopy_draw(mp_obj_t segment, mp_obj_t pattern,
                            mp_obj_t params);

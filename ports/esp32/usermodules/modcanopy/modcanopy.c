#include "modcanopy.h"
#include "py/runtime.h"

// canopy Pattern object

STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_pattern_deinit_obj,
                                 canopy_pattern_deinit);

STATIC const mp_rom_map_elem_t canopy_pattern_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&canopy_pattern_deinit_obj)},
};

STATIC MP_DEFINE_CONST_DICT(canopy_pattern_locals_dict,
                            canopy_pattern_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(canopy_pattern_type, MP_QSTR_Pattern,
                         MP_TYPE_FLAG_NONE, make_new, canopy_pattern_make_new,
                         attr, canopy_pattern_attr, locals_dict,
                         &canopy_pattern_locals_dict);

// Canopy Segment object
STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_segment_deinit_obj,
                                 canopy_segment_deinit);

STATIC const mp_rom_map_elem_t canopy_segment_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&canopy_segment_deinit_obj)},
};

STATIC MP_DEFINE_CONST_DICT(canopy_segment_locals_dict,
                            canopy_segment_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(canopy_segment_type, MP_QSTR_Segment,
                         MP_TYPE_FLAG_NONE, make_new, canopy_segment_make_new,
                         locals_dict, &canopy_segment_locals_dict);

// Canopy Params object

STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_params_deinit_obj,
                                 canopy_params_deinit);

STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_params_keys_obj, canopy_params_keys);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_params_values_obj,
                                 canopy_params_values);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_params_items_obj, canopy_params_items);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(canopy_params_contains_obj,
                                 canopy_params_contains);

STATIC const mp_rom_map_elem_t canopy_params_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&canopy_params_deinit_obj)},
    {MP_ROM_QSTR(MP_QSTR_keys), MP_ROM_PTR(&canopy_params_keys_obj)},
    {MP_ROM_QSTR(MP_QSTR_values), MP_ROM_PTR(&canopy_params_values_obj)},
    {MP_ROM_QSTR(MP_QSTR_items), MP_ROM_PTR(&canopy_params_items_obj)},
    {MP_ROM_QSTR(MP_QSTR_contains), MP_ROM_PTR(&canopy_params_contains_obj)},

};

STATIC MP_DEFINE_CONST_DICT(canopy_params_locals_dict,
                            canopy_params_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(canopy_params_type, MP_QSTR_Params, MP_TYPE_FLAG_NONE,
                         make_new, canopy_params_make_new, subscr,
                         canopy_params_subscr, locals_dict,
                         &canopy_params_locals_dict);

// canopy module

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(canopy_init_obj, 2, canopy_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_end_obj, canopy_end);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_clear_obj, canopy_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_render_obj, canopy_render);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(canopy_draw_obj, 2, canopy_draw);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(canopy_brightness_obj, 0, canopy_brightness);

STATIC const mp_rom_map_elem_t mp_module_canopy_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_canopy)},
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&canopy_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_end), MP_ROM_PTR(&canopy_end_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&canopy_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&canopy_render_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&canopy_draw_obj)},
    {MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&canopy_brightness_obj)},
    {MP_ROM_QSTR(MP_QSTR_Pattern), MP_ROM_PTR(&canopy_pattern_type)},
    {MP_ROM_QSTR(MP_QSTR_Segment), MP_ROM_PTR(&canopy_segment_type)},
    {MP_ROM_QSTR(MP_QSTR_Params), MP_ROM_PTR(&canopy_params_type)},
    {MP_ROM_QSTR(MP_QSTR_RGB), MP_ROM_INT(012)},
    {MP_ROM_QSTR(MP_QSTR_RBG), MP_ROM_INT(021)},
    {MP_ROM_QSTR(MP_QSTR_GRB), MP_ROM_INT(102)},
    {MP_ROM_QSTR(MP_QSTR_GBR), MP_ROM_INT(120)},
    {MP_ROM_QSTR(MP_QSTR_BRG), MP_ROM_INT(201)},
    {MP_ROM_QSTR(MP_QSTR_BGR), MP_ROM_INT(210)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_canopy_globals,
                            mp_module_canopy_globals_table);

const mp_obj_module_t mp_module_canopy = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_canopy_globals,
};

MP_REGISTER_MODULE(MP_QSTR_canopy, mp_module_canopy);

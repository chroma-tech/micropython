#include "modcanopy.h"
#include "py/runtime.h"

STATIC MP_DEFINE_CONST_FUN_OBJ_1(canopy_pattern_deinit_obj, canopy_pattern_deinit);

STATIC const mp_rom_map_elem_t canopy_pattern_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&canopy_pattern_deinit_obj) },
};

STATIC MP_DEFINE_CONST_DICT(canopy_pattern_locals_dict,
                            canopy_pattern_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(canopy_pattern_type, MP_QSTR_Pattern,
                         MP_TYPE_FLAG_NONE, make_new, canopy_pattern_make_new,
                         locals_dict, &canopy_pattern_locals_dict);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(canopy_init_obj, canopy_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_clear_obj, canopy_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_render_obj, canopy_render);
STATIC MP_DEFINE_CONST_FUN_OBJ_3(canopy_draw_obj, canopy_draw);

STATIC const mp_rom_map_elem_t mp_module_canopy_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_canopy)},
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&canopy_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&canopy_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&canopy_render_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&canopy_draw_obj)},
    {MP_ROM_QSTR(MP_QSTR_Pattern), MP_ROM_PTR(&canopy_pattern_type)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_canopy_globals,
                            mp_module_canopy_globals_table);

const mp_obj_module_t mp_module_canopy = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_canopy_globals,
};

MP_REGISTER_MODULE(MP_QSTR_canopy, mp_module_canopy);

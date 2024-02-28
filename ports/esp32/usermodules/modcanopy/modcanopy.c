#include "modcanopy.h"
#include "py/runtime.h"

STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_init_obj, canopy_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(canopy_draw_obj, canopy_draw);

STATIC const mp_rom_map_elem_t mp_module_canopy_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_canopy)},
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&canopy_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&canopy_draw_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_canopy_globals,
                            mp_module_canopy_globals_table);

const mp_obj_module_t mp_module_canopy = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_canopy_globals,
};

MP_REGISTER_MODULE(MP_QSTR_canopy, mp_module_canopy);

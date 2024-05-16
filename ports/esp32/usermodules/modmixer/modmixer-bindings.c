#include <py/builtin.h>
#include <py/runtime.h>

#include "modmixer.h"

typedef struct {
  mp_obj_base_t base;
  MixerVoice voice;
} mixer_voice_obj_t;

const mp_obj_type_t mixer_voice_type;

mp_obj_t mixer_voice_make_new(const mp_obj_type_t *type, size_t n_args,
                              size_t n_kw, const mp_obj_t *args) {

  if (n_args < 1) {
    mp_raise_TypeError(MP_ERROR_TEXT("filename required"));
  }

  mixer_voice_obj_t *o = m_new_obj(mixer_voice_obj_t);
  o->base.type = &mixer_voice_type;

  // const char *filename = mp_obj_str_get_str(args[0]);

  mp_obj_t arg = args[0];
  if (mp_obj_is_str(arg)) {
    arg = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_builtin_open_obj), arg,
                             MP_ROM_QSTR(MP_QSTR_rb));
  }

  // if (!mp_obj_is_type(arg, &mp_type_vfs_fat_fileio)) {
  //   mp_raise_TypeError(
  //       MP_ERROR_TEXT("file must be a file opened in byte mode"));
  // }

  if (!source_init_from_stream(&o->voice.source, arg)) {
    m_del_obj(mixer_voice_obj_t, o);
    mp_raise_msg(&mp_type_OSError,
                 "file not found or invalid wav file or invalid format");
  }

  // uint8_t *buffer = NULL;
  // size_t buffer_size = 0;
  // if (n_args >= 2) {
  //   mp_buffer_info_t bufinfo;
  //   mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);
  //   buffer = bufinfo.buf;
  //   buffer_size =
  //       mp_arg_validate_length_range(bufinfo.len, 8, 1024, MP_QSTR_buffer);
  // }

  // if (!source_init_from_wav(&o->voice.source, filename)) {
  //   m_del_obj(mixer_voice_obj_t, o);
  //   mp_raise_msg(&mp_type_OSError,
  //                "file not found or invalid wav file or invalid format");
  // }

  // TODO: optional kwargs for loop and volume
  o->voice.loop = false;
  o->voice.volume = 1.0f;

  return MP_OBJ_FROM_PTR(o);
}

mp_obj_t mixer_voice_deinit(mp_obj_t self_in) {
  printf("Inside mixer_voice_deinit\n");
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;
  wav_close(self->voice.source.context);
  self->voice.source.context = NULL;
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voice_deinit_obj, mixer_voice_deinit);

void mixer_voice_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;

  // get
  if (dest[0] == MP_OBJ_NULL) {
    if (attr == MP_QSTR_loop) {
      dest[0] = mp_obj_new_bool(self->voice.loop);
    } else if (attr == MP_QSTR_volume) {
      dest[0] = mp_obj_new_float(self->voice.volume);
    } else if (attr == MP_QSTR_sample_rate) {
      dest[0] = mp_obj_new_int(self->voice.source.sample_rate);
    } else if (attr == MP_QSTR_channels) {
      dest[0] = mp_obj_new_int(self->voice.source.channels);
    }
  } else {
    // set
    if (attr == MP_QSTR_loop) {
      self->voice.loop = mp_obj_is_true(dest[1]);
    } else if (attr == MP_QSTR_volume) {
      self->voice.volume = mp_obj_get_float(dest[1]);
      if (self->voice.volume < 0.0f) {
        self->voice.volume = 0.0f;
      } else if (self->voice.volume > 1.0f) {
        self->voice.volume = 1.0f;
      }
    }

    // indicate success
    dest[0] = MP_OBJ_NULL;
  }
}

STATIC const mp_rom_map_elem_t mixer_voice_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mixer_voice_deinit_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mixer_voice_locals_dict,
                            mixer_voice_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(mixer_voice_type, MP_QSTR_Voice,
                         MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS, make_new,
                         mixer_voice_make_new, attr, mixer_voice_attr,
                         locals_dict, &mixer_voice_locals_dict);

//----------------------------------------------------------------

// typedef struct {
//   mp_obj_base_t base;
//   Mixer mixer;
// } mixer_obj_t;

// mixer module

mp_obj_t mixer_mixinto(mp_obj_t voice_in, mp_obj_t buffer_in) {

  mixer_voice_obj_t *voice = (mixer_voice_obj_t *)voice_in;

  // // see if voices_in is a single voice or an iterable of voices
  // mp_obj_t iter = mp_getiter(voices_in);
  // mp_obj_t voice_in = mp_iternext(iter);
  // if (voice_in == MP_OBJ_STOP_ITERATION) {
  //   return mp_const_none;
  // }

  // get a writable buffer
  mp_buffer_info_t bufinfo;
  mp_get_buffer_raise(buffer_in, &bufinfo, MP_BUFFER_WRITE);

  // mix into the buffer, but how much
  size_t num_samples = bufinfo.len / sizeof(int16_t);
  mixer_read_samples(&voice->voice, 1, bufinfo.buf, num_samples);

  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mixer_mixinto_obj, mixer_mixinto);

STATIC const mp_rom_map_elem_t mp_module_mixer_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mixer)},
    {MP_ROM_QSTR(MP_QSTR_mixinto), MP_ROM_PTR(&mixer_mixinto_obj)},
    {MP_ROM_QSTR(MP_QSTR_Voice), MP_ROM_PTR(&mixer_voice_type)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_mixer_globals,
                            mp_module_mixer_globals_table);

const mp_obj_module_t mp_module_mixer = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_mixer_globals,
};

MP_REGISTER_MODULE(MP_QSTR_mixer, mp_module_mixer);

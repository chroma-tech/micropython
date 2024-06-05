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

  mp_obj_t arg = args[0];
  if (mp_obj_is_str(arg)) {
    arg = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_builtin_open_obj), arg,
                             MP_ROM_QSTR(MP_QSTR_rb));
    o->voice.source.needs_close = true;
  }

  if (!source_init_from_stream(&o->voice.source, arg)) {
    m_del_obj(mixer_voice_obj_t, o);
    mp_raise_msg(&mp_type_OSError,
                 "file not found or invalid wav file or invalid format");
  }

  // TODO: optional kwargs for loop and volume
  o->voice.loop = false;
  o->voice.volume = 1.0f;
  o->voice.playing = false;

  return MP_OBJ_FROM_PTR(o);
}

mp_obj_t mixer_voice_deinit(mp_obj_t self_in) {
  printf("Inside mixer_voice_deinit\n");
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;
  if (self->voice.source.needs_close) {
    mp_obj_t close = mp_load_attr(self->voice.source.stream_obj, MP_QSTR_close);
    mp_call_function_0(close);
  }
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voice_deinit_obj, mixer_voice_deinit);

mp_obj_t mixer_voice_play(mp_obj_t self_in) {
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;
  self->voice.playing = true;
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voice_play_obj, mixer_voice_play);

mp_obj_t mixer_voice_stop(mp_obj_t self_in) {
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;
  self->voice.playing = false;
  source_reset(&self->voice.source);
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voice_stop_obj, mixer_voice_stop);

mp_obj_t mixer_voice_pause(mp_obj_t self_in) {
  mixer_voice_obj_t *self = (mixer_voice_obj_t *)self_in;
  self->voice.playing = false;
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voice_pause_obj, mixer_voice_pause);

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
    } else if (attr == MP_QSTR_playing) {
      dest[0] = mp_obj_new_bool(self->voice.playing);
    } else {
      dest[1] = MP_OBJ_SENTINEL;
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
    {MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&mixer_voice_play_obj)},
    {MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mixer_voice_stop_obj)},
    {MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&mixer_voice_pause_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mixer_voice_locals_dict,
                            mixer_voice_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(mixer_voice_type, MP_QSTR_Voice,
                         MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS, make_new,
                         mixer_voice_make_new, attr, mixer_voice_attr,
                         locals_dict, &mixer_voice_locals_dict);

//----------------------------------------------------------------

// Mixer type

typedef struct {
  mp_obj_base_t base;
  mp_obj_t voices;
} mixer_mixer_obj_t;

const mp_obj_type_t mixer_mixer_type;

mp_obj_t mixer_mixer_make_new(const mp_obj_type_t *type, size_t n_args,
                              size_t n_kw, const mp_obj_t *args) {
  mixer_mixer_obj_t *o = m_new_obj(mixer_mixer_obj_t);
  o->base.type = &mixer_mixer_type;
  o->voices = mp_obj_new_list(0, NULL);
  return MP_OBJ_FROM_PTR(o);
}

mp_obj_t mixer_mixer_play(mp_obj_t self_in, mp_obj_t voice_in) {
  mixer_mixer_obj_t *self = (mixer_mixer_obj_t *)self_in;
  mixer_voice_obj_t *voice = (mixer_voice_obj_t *)voice_in;
  voice->voice.playing = true;

  // if the voice is already in the list, reset it and return
  mp_obj_t *items;
  size_t len;
  mp_obj_list_get(self->voices, &len, &items);
  for (ssize_t i = 0; i < len; ++i) {
    if (items[i] == voice_in) {
      source_reset(&voice->voice.source);
      return mp_const_none;
    }
  }

  // add the voice to the list
  mp_obj_list_append(self->voices, voice_in);

  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mixer_play_obj, mixer_mixer_play);

mp_obj_t mixer_mixinto(mp_obj_t self_in, mp_obj_t buffer_in) {
  mixer_mixer_obj_t *self = (mixer_mixer_obj_t *)self_in;

  // get a writable buffer
  mp_buffer_info_t bufinfo;
  mp_get_buffer_raise(buffer_in, &bufinfo, MP_BUFFER_WRITE);

  size_t num_samples = bufinfo.len / sizeof(int16_t);
  if (num_samples == 0) {
    return mp_const_none;
  }

  for (ssize_t i = 0; i < num_samples; ++i) {
    ((uint16_t *)bufinfo.buf)[i] = 0;
  }

  mp_obj_t *items;
  size_t len;
  mp_obj_list_get(self->voices, &len, &items);

  if (len == 0) {
    return mp_const_none;
  }

  // prune stopped voices. iterate from the end
  for (ssize_t i = len - 1; i >= 0; --i) {
    mp_obj_t item = items[i];
    mixer_voice_obj_t *voice_obj = (mixer_voice_obj_t *)item;
    if (!voice_obj->voice.playing) {
      mp_obj_list_remove(self->voices, item);
    }
  }

  mp_obj_list_get(self->voices, &len, &items);

  // mix the remaining voices
  bool first_voice = true;
  for (ssize_t i = 0; i < len; ++i) {
    mixer_voice_obj_t *voice_obj = (mixer_voice_obj_t *)items[i];
    if (!voice_obj->voice.playing || voice_obj->voice.volume == 0.0f) {
      continue;
    }
    mixer_mix_voice(&voice_obj->voice, bufinfo.buf, num_samples, first_voice);
    first_voice = false;
  }

  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mixer_mixinto_obj, mixer_mixinto);

mp_obj_t mixer_voices(mp_obj_t self_in) {
  mixer_mixer_obj_t *self = (mixer_mixer_obj_t *)self_in;
  return self->voices;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mixer_voices_obj, mixer_voices);

// create a type for Mixer

STATIC const mp_rom_map_elem_t mixer_mixer_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&mixer_play_obj)},
    {MP_ROM_QSTR(MP_QSTR_mixinto), MP_ROM_PTR(&mixer_mixinto_obj)},
    {MP_ROM_QSTR(MP_QSTR_voices), MP_ROM_PTR(&mixer_voices_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mixer_mixer_locals_dict,
                            mixer_mixer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(mixer_mixer_type, MP_QSTR_Voice, MP_TYPE_FLAG_NONE,
                         make_new, mixer_mixer_make_new, locals_dict,
                         &mixer_mixer_locals_dict);

STATIC const mp_rom_map_elem_t mp_module_mixer_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mixer)},
    {MP_ROM_QSTR(MP_QSTR_Voice), MP_ROM_PTR(&mixer_voice_type)},
    {MP_ROM_QSTR(MP_QSTR_Mixer), MP_ROM_PTR(&mixer_mixer_type)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_mixer_globals,
                            mp_module_mixer_globals_table);

const mp_obj_module_t mp_module_mixer = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_mixer_globals,
};

MP_REGISTER_MODULE(MP_QSTR_mixer, mp_module_mixer);

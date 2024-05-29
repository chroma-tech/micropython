#pragma once

#include <py/obj.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct _AudioSource {
  mp_obj_t stream_obj;
  uint16_t channels;
  uint32_t sample_rate;
  uint16_t sample_size;
  uint32_t data_size;
  uint32_t data_remaining;
  bool needs_close;
} AudioSource;

typedef struct {
  AudioSource source;
  float volume;
  bool loop;
  bool playing;
} MixerVoice;

bool source_init_from_stream(AudioSource *source, mp_obj_t stream_obj);

size_t mixer_mix_voice(MixerVoice *voice, int16_t *buffer, size_t num_samples,
                       bool overwrite);

size_t mixer_read_samples(MixerVoice *voices, size_t num_voices,
                          int16_t *buffer, size_t num_samples);

void source_reset(AudioSource *source);
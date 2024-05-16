#pragma once

#include <py/obj.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  void *context;
  size_t (*read_samples)(void *context, int16_t *buffer, size_t num_samples,
                         bool loop);
  void (*reset)(void *context);
  void (*close)(void *context);
  uint16_t channels;
  uint32_t sample_rate;
  uint16_t bits_per_sample;
} AudioSource;

typedef struct {
  FILE *file;
  uint32_t data_offset;
  uint16_t sample_size;
} WavFileSourceContext;

typedef struct {
  AudioSource source;
  float volume;
  bool loop;
  bool playing;
} MixerVoice;

size_t wav_read_samples(void *context, int16_t *buffer, size_t num_samples,
                        bool loop);
void wav_reset(void *context);
void wav_close(void *context);

bool source_init_from_stream(AudioSource *source, mp_obj_t stream_obj);
bool source_init_from_wav(AudioSource *source, const char *filename);

size_t mixer_read_samples(MixerVoice *voices, size_t num_voices,
                          int16_t *buffer, size_t num_samples);

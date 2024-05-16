#include <memory.h>

#include <py/runtime.h>
#include <py/stream.h>

#include "audiomath.h"
#include "modmixer.h"

typedef struct {
  uint8_t riff[4];          // "RIFF"
  uint32_t size;            // File size - 8 bytes
  uint8_t wave[4];          // "WAVE"
  uint8_t fmt[4];           // "fmt "
  uint32_t fmt_size;        // Size of the fmt chunk
  uint16_t format;          // Format type (1 for PCM)
  uint16_t channels;        // Number of channels
  uint32_t sample_rate;     // Sample rate
  uint32_t byte_rate;       // Byte rate
  uint16_t block_align;     // Block align
  uint16_t bits_per_sample; // Bits per sample
  uint8_t data[4];          // "data"
  uint32_t data_size;       // Size of the data chunk
} WAVHeader;

bool verify_wav_header(WAVHeader *header) {
  // Verify the header
  if (memcmp(header->riff, "RIFF", 4) != 0 ||
      memcmp(header->wave, "WAVE", 4) != 0 ||
      memcmp(header->fmt, "fmt ", 4) != 0 ||
      memcmp(header->data, "data", 4) != 0) {
    return false;
  }

  // Verify the format. we only deal with mono 16 bit signed PCM 16khz
  if (header->format != 1 || header->bits_per_sample != 16) {
    return false;
  }

  return true;
}

size_t stream_read(mp_obj_t stream_obj, void *buffer, size_t size) {
  // Get the stream protocol
  mp_stream_p_t *stream_p = mp_get_stream(stream_obj);

  if (!stream_p) {
    mp_raise_TypeError("object is not a stream");
  }

  // Read from the stream
  int error;
  size_t bytes_read = stream_p->read(stream_obj, buffer, size, &error);

  if (bytes_read == MP_STREAM_ERROR) {
    mp_raise_OSError(error);
  }

  return bytes_read;
}

void stream_seek(mp_obj_t stream_obj, uint32_t position) {
  mp_obj_t seek = mp_load_attr(stream_obj, MP_QSTR_seek);
  mp_call_function_2(seek, mp_obj_new_int(position), mp_obj_new_int(0));
}

void stream_reset(mp_obj_t stream_obj) { stream_seek(stream_obj, 44); }

void stream_close(mp_obj_t stream_obj) {
  mp_obj_t close = mp_load_attr(stream_obj, MP_QSTR_close);
  mp_call_function_0(close);
}

size_t stream_read_samples(void *context, int16_t *buffer, size_t num_samples,
                           bool loop) {
  mp_obj_t stream_obj = (mp_obj_t)context;
  size_t bytes_to_read = num_samples * 2;
  uint8_t *buffer8 = (uint8_t *)buffer;
  size_t bytes_read = stream_read(stream_obj, buffer8, bytes_to_read);

  while (loop && bytes_read < bytes_to_read) {
    stream_reset(stream_obj);
    bytes_read += stream_read(stream_obj, buffer8 + bytes_read,
                              bytes_to_read - bytes_read);
  }

  // fill the difference with silence
  if (bytes_read < bytes_to_read) {
    memset(buffer8 + bytes_read, 0, (bytes_to_read - bytes_read));
  }

  return bytes_read / 2;
}

bool source_init_from_stream(AudioSource *source, mp_obj_t stream_obj) {
  // Read the header
  WAVHeader header;
  if (stream_read(stream_obj, (uint8_t *)&header, 44) != 44) {
    return false;
  }

  if (!verify_wav_header(&header)) {
    return false;
  }

  source->channels = header.channels;
  source->sample_rate = header.sample_rate;
  source->bits_per_sample = header.bits_per_sample;

  // TODO: hold a reference to the stream object

  source->context = (void *)stream_obj;
  source->read_samples = stream_read_samples;
  source->reset = stream_reset;
  source->close = stream_close;

  stream_reset(stream_obj);
  return true;
}

size_t wav_read_samples(void *context, int16_t *buffer, size_t num_samples,
                        bool loop) {
  WavFileSourceContext *ctx = (WavFileSourceContext *)context;
  size_t samples_read = fread(buffer, ctx->sample_size, num_samples, ctx->file);
  while (samples_read < num_samples && loop) {
    fseek(ctx->file, ctx->data_offset, SEEK_SET);
    samples_read +=
        fread(buffer + samples_read * ctx->sample_size, ctx->sample_size,
              num_samples - samples_read, ctx->file);
  }

  // fill the difference with silence
  if (samples_read < num_samples) {
    memset(buffer + samples_read, 0,
           (num_samples - samples_read) * ctx->sample_size);
    samples_read += num_samples - samples_read;
  }

  return num_samples;
}

void wav_reset(void *context) {
  WavFileSourceContext *ctx = (WavFileSourceContext *)context;
  fseek(ctx->file, ctx->data_offset, SEEK_SET);
}

void wav_close(void *context) {
  WavFileSourceContext *ctx = (WavFileSourceContext *)context;
  fclose(ctx->file);
  free(ctx);
}

bool source_init_from_wav(AudioSource *source, const char *filename) {
  WAVHeader header;

  FILE *file = fopen(filename, "rb");
  if (!file) {
    return false;
  }

  if (fread(&header, sizeof(WAVHeader), 1, file) != 1) {
    fclose(file);
    return false;
  }

  if (!verify_wav_header(&header)) {
    fclose(file);
    return false;
  }

  WavFileSourceContext *context = malloc(sizeof(WavFileSourceContext));
  if (!context) {
    fclose(file);
    return false;
  }

  context->file = file;
  context->data_offset = sizeof(WAVHeader) + (header.fmt_size - 16);
  context->sample_size = header.bits_per_sample / 8;

  source->context = context;
  source->read_samples = wav_read_samples;
  source->reset = wav_reset;
  source->close = wav_close;

  wav_reset(context);
  return true;
}

size_t mixer_read_samples(MixerVoice *voices, size_t num_voices,
                          int16_t *buffer, size_t num_samples) {
  bool active = false;

  for (size_t i = 0; i < num_voices; i++) {
    // if (!voices[i].playing) {
    //   continue;
    // }

    AudioSource *source = &voices[i].source;
    size_t read = source->read_samples(source->context, buffer, num_samples,
                                       voices[i].loop);

    // if (read < num_samples) {
    //   voices[i].playing = false;
    //   source->reset(source->context);
    // }

    if (!active) {
      // first voice just scale by volume
      for (size_t j = 0; j < read; j++) {
        buffer[j] = fmult16signed(buffer[j], voices[i].volume);
      }
      active = true;

    } else {
      // every other voice needs to be mixed
      for (size_t j = 0; j < read; j++) {
        buffer[j] =
            add16signed(buffer[j], fmult16signed(buffer[j], voices[i].volume));
      }
    }
  }

  return num_samples;
}

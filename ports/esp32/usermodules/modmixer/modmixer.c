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

void source_reset(AudioSource *source) {
  stream_seek(source->stream_obj, 44);
  source->data_remaining = source->data_size;
}

size_t stream_read_samples(AudioSource *source, int16_t *buffer,
                           size_t num_samples, bool loop) {

  uint8_t *buffer8 = (uint8_t *)buffer;

  // this function will always fill the buffer up. if we're looping, we'll reset
  // and keep reading. If we're not looping, we'll fill the rest with silence

  // each call to stream_read needs to be limited by data_remaining, and
  // num_samples
  size_t total_bytes_to_read = num_samples * 2;
  size_t bytes_to_read = MIN(total_bytes_to_read, source->data_remaining);
  size_t total_bytes_read =
      stream_read(source->stream_obj, buffer8, bytes_to_read);
  source->data_remaining -= total_bytes_read;

  while (loop && total_bytes_read < total_bytes_to_read) {
    source_reset(source);
    bytes_to_read =
        MIN(total_bytes_to_read - total_bytes_read, source->data_remaining);
    size_t bytes_read = stream_read(source->stream_obj,
                                    buffer8 + total_bytes_read, bytes_to_read);
    total_bytes_read += bytes_read;
    source->data_remaining -= bytes_read;
  }

  // fill the difference with silence
  if (total_bytes_read < total_bytes_to_read) {
    source_reset(source);
    memset(buffer8 + total_bytes_read, 0,
           total_bytes_to_read - total_bytes_read);
  }

  return total_bytes_read / 2;
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

  source->stream_obj = stream_obj;
  source->channels = header.channels;
  source->sample_rate = header.sample_rate;
  source->sample_size = header.bits_per_sample / 8;
  source->data_size = header.data_size;
  source->data_remaining = header.data_size;

  // source_reset(source);
  return true;
}

// size_t mixer_read_samples(MixerVoice *voices, size_t num_voices,
//                           int16_t *buffer, size_t num_samples) {
//   bool active = false;

//   for (size_t i = 0; i < num_voices; i++) {
//     if (!voices[i].playing) {
//       continue;
//     }

//     AudioSource *source = &voices[i].source;

//     stream_read_samples(source, buffer, num_samples, voices[i].loop);

//     size_t read =
//         source->read_samples(source, buffer, num_samples, voices[i].loop);

//     // this reset logic only gets called if loop is false
//     if (read < num_samples) {
//       voices[i].playing = false;
//       source->reset(source->context);
//     }

//     if (!active) {
//       // first voice just scale by volume
//       for (size_t j = 0; j < read; j++) {
//         buffer[j] = fmult16signed(buffer[j], voices[i].volume);
//       }
//       active = true;

//     } else {
//       // every other voice needs to be mixed
//       for (size_t j = 0; j < read; j++) {
//         buffer[j] =
//             add16signed(buffer[j], fmult16signed(buffer[j],
//             voices[i].volume));
//       }
//     }
//   }

//   return num_samples;
// }

size_t mixer_mix_voice(MixerVoice *voice, int16_t *buffer, size_t num_samples,
                       bool overwrite) {

  int16_t chunk[128];

  if (!voice->playing) {
    return 0;
  }

  AudioSource *source = &voice->source;

  // read in chunks
  size_t samples_read = 0;
  while (samples_read < num_samples) {
    size_t chunk_size = num_samples - samples_read;
    if (chunk_size > 128) {
      chunk_size = 128;
    }

    if (!voice->playing) {
      memset(chunk, 0, sizeof(chunk));
    } else {
      if (stream_read_samples(source, chunk, chunk_size, voice->loop) <
          chunk_size) {
        voice->playing = false;
      }
    }

    if (overwrite) {
      // first voice just scale by volume
      for (size_t j = 0; j < chunk_size; j++) {
        buffer[samples_read + j] = fmult16signed(chunk[j], voice->volume);
      }
    } else {
      // every other voice needs to be mixed
      for (size_t j = 0; j < chunk_size; j++) {
        buffer[samples_read + j] = add16signed(
            buffer[samples_read + j], fmult16signed(chunk[j], voice->volume));
      }
    }

    samples_read += chunk_size;
  }

  return num_samples;
}

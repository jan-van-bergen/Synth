#include "util.h"

#include <cassert>

#include <immintrin.h>

#include <SDL2/SDL.h>

std::vector<Sample> util::load_wav(char const * filename) {
	Uint32        wav_length;
	Uint8       * wav_buffer;
	SDL_AudioSpec wav_spec;

	if (SDL_LoadWAV(filename, &wav_spec, &wav_buffer, &wav_length) == nullptr) {
		printf("ERROR: Unable to load sample '%s'!\n", filename);
		return { };
	}

	if (wav_spec.channels != 1 && wav_spec.channels != 2) {
		printf("ERROR: Sample '%s' has %i channels! Should be either 1 (Mono) or 2 (Stereo)\n", filename, wav_spec.channels);
		return { };
	}

	if (wav_spec.freq != SAMPLE_RATE) {
		printf("ERROR: Sample '%s' has a sample rate of %i! Only %i is supported\n", filename, wav_spec.freq, SAMPLE_RATE);
		return { };
	}

	std::vector<Sample> samples;

	switch (wav_spec.format) {
		case AUDIO_F32LSB: {
			samples.resize(wav_length / (wav_spec.channels * sizeof(float)));

			if (wav_spec.channels == 1) { // Mono
				for (int i = 0; i < samples.size(); i++) {
					float sample;
					memcpy(&sample, wav_buffer + i * sizeof(float), sizeof(float));

					samples[i] = sample;
				}
			} else { // Stereo
				samples.resize(wav_length / sizeof(Sample));
				memcpy(samples.data(), wav_buffer, wav_length);
			}

			break;
		}

		case AUDIO_S32LSB: {
			samples.resize(wav_length / (wav_spec.channels * sizeof(int)));

			if (wav_spec.channels == 1) { // Mono
				for (int i = 0; i < samples.size(); i++) {
					int sample;
					memcpy(&sample,  wav_buffer + i * sizeof(int), sizeof(int));
					
					samples[i] = float(sample) / float(std::numeric_limits<int>::max());
				}
			} else { // Stereo
				for (int i = 0; i < samples.size(); i++) {
					auto offset = 2 * i;

					int left, right;
					memcpy(&left,  wav_buffer + (offset)     * sizeof(int), sizeof(int));
					memcpy(&right, wav_buffer + (offset + 1) * sizeof(int), sizeof(int));

					samples[i].left  = float(left)  / float(std::numeric_limits<int>::max());
					samples[i].right = float(right) / float(std::numeric_limits<int>::max());
				}
			}

			break;
		}

		default: printf("ERROR: Sample '%s' has unsupported format 0x%x\n", filename, wav_spec.format);
	}

	SDL_FreeWAV(wav_buffer);

	return samples;
}

int util::round(float f) {
//	f -= copysignf(0.5f, f); // Round up/down based on sign of f
	return _mm_cvtss_si32(_mm_load_ss(&f));
}

int util::scancode_to_note(SDL_Scancode scancode) {
	switch (scancode) {
		case SDL_SCANCODE_Z:          return 24; // C
		case SDL_SCANCODE_S:          return 25; // C#
		case SDL_SCANCODE_X:          return 26; // D
		case SDL_SCANCODE_D:          return 27; // D#
		case SDL_SCANCODE_C:          return 28; // E
		case SDL_SCANCODE_V:          return 29; // F
		case SDL_SCANCODE_G:          return 30; // F#
		case SDL_SCANCODE_B:          return 31; // G
		case SDL_SCANCODE_H:          return 32; // G#
		case SDL_SCANCODE_N:          return 33; // A
		case SDL_SCANCODE_J:          return 34; // A#
		case SDL_SCANCODE_M:          return 35; // B
		case SDL_SCANCODE_COMMA:      return 36; // C
		case SDL_SCANCODE_L:          return 37; // C#
		case SDL_SCANCODE_PERIOD:     return 38; // D
		case SDL_SCANCODE_SEMICOLON:  return 39; // D#
		case SDL_SCANCODE_SLASH:      return 40; // E
		case SDL_SCANCODE_APOSTROPHE: return 41; // F

		case SDL_SCANCODE_Q:            return 36; // C
		case SDL_SCANCODE_2:            return 37; // C#
		case SDL_SCANCODE_W:            return 38; // D
		case SDL_SCANCODE_3:            return 39; // D#
		case SDL_SCANCODE_E:            return 40; // E
		case SDL_SCANCODE_R:            return 41; // F
		case SDL_SCANCODE_5:            return 42; // F#
		case SDL_SCANCODE_T:            return 43; // G
		case SDL_SCANCODE_6:            return 44; // G#
		case SDL_SCANCODE_Y:            return 45; // A
		case SDL_SCANCODE_7:            return 46; // A#
		case SDL_SCANCODE_U:            return 47; // B
		case SDL_SCANCODE_I:            return 48; // C
		case SDL_SCANCODE_9:            return 49; // C#
		case SDL_SCANCODE_O:            return 50; // D
		case SDL_SCANCODE_0:            return 51; // D#
		case SDL_SCANCODE_P:            return 52; // E
		case SDL_SCANCODE_LEFTBRACKET:  return 53; // F
		case SDL_SCANCODE_RIGHTBRACKET: return 55; // G

		default: return -1;
	}
}

float util::note_freq(int note) {
	assert(note >= 0);
	
	static constexpr float C_0 = 16.35f; // Tuning of C0 in Hz

	float freq = C_0;

	// Find octave
	while (note >= 12) {
		freq *= 2.0f;
		note -= 12;
	}

	static constexpr auto pow_lut = util::generate_lookup_table<float, 12>([](int i) {
		auto result = 1.0;

		while (i > 0) {
			result *= 1.059463094;
			i--;
		}

		return float(result);

	});

	return freq * pow_lut[note];
}

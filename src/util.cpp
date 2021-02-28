#include "util.h"

#include <cassert>
#include <ctime>
#include <bit>

#include <immintrin.h>

#include <SDL2/SDL.h>

static unsigned wang_hash(unsigned seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);

    return seed;
}

unsigned util::seed() {
	return wang_hash(std::time(nullptr));
}

unsigned util::rand(unsigned & seed) {
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);

	return seed;
}

float util::randf(unsigned & seed) {
	static constexpr auto ONE_OVER_MAX_UINT = std::bit_cast<float>(0x2f800004u);
	return rand(seed) * ONE_OVER_MAX_UINT;
}

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

std::vector<char> util::read_file(char const * filename) {
	FILE * file; fopen_s(&file, filename, "rb");

	if (file == nullptr) {
		printf("ERROR: Unable to open file '%s'!\n", filename);
		return { };
	}

	// Get file length
	fseek(file, 0, SEEK_END);
	auto file_size = ftell(file);
	rewind(file);

	std::vector<char> buffer(file_size);
	fread(buffer.data(), file_size, 1, file);

	fclose(file);

	return buffer;
}

int util::round(float f) {
//	f -= copysignf(0.5f, f); // Round up/down based on sign of f
	return _mm_cvtss_si32(_mm_load_ss(&f));
}

int util::scancode_to_note(SDL_Scancode scancode) {
	switch (scancode) {
		case SDL_SCANCODE_Z:          return note<NoteName::C,       2>();
		case SDL_SCANCODE_S:          return note<NoteName::C_SHARP, 2>();
		case SDL_SCANCODE_X:          return note<NoteName::D,       2>();
		case SDL_SCANCODE_D:          return note<NoteName::D_SHARP, 2>();
		case SDL_SCANCODE_C:          return note<NoteName::E,       2>();
		case SDL_SCANCODE_V:          return note<NoteName::F,       2>();
		case SDL_SCANCODE_G:          return note<NoteName::F_SHARP, 2>();
		case SDL_SCANCODE_B:          return note<NoteName::G,       2>();
		case SDL_SCANCODE_H:          return note<NoteName::G_SHARP, 2>();
		case SDL_SCANCODE_N:          return note<NoteName::A,       2>();
		case SDL_SCANCODE_J:          return note<NoteName::A_SHARP, 2>();
		case SDL_SCANCODE_M:          return note<NoteName::B,       2>();
		case SDL_SCANCODE_COMMA:      return note<NoteName::C,       3>();
		case SDL_SCANCODE_L:          return note<NoteName::C_SHARP, 3>();
		case SDL_SCANCODE_PERIOD:     return note<NoteName::D,       3>();
		case SDL_SCANCODE_SEMICOLON:  return note<NoteName::D_SHARP, 3>();
		case SDL_SCANCODE_SLASH:      return note<NoteName::E,       3>();
		case SDL_SCANCODE_APOSTROPHE: return note<NoteName::F,       3>();

		case SDL_SCANCODE_Q:            return note<NoteName::C,       3>();
		case SDL_SCANCODE_2:            return note<NoteName::C_SHARP, 3>();
		case SDL_SCANCODE_W:            return note<NoteName::D,       3>();
		case SDL_SCANCODE_3:            return note<NoteName::D_SHARP, 3>();
		case SDL_SCANCODE_E:            return note<NoteName::E,       3>();
		case SDL_SCANCODE_R:            return note<NoteName::F,       3>();
		case SDL_SCANCODE_5:            return note<NoteName::F_SHARP, 3>();
		case SDL_SCANCODE_T:            return note<NoteName::G,       3>();
		case SDL_SCANCODE_6:            return note<NoteName::G_SHARP, 3>();
		case SDL_SCANCODE_Y:            return note<NoteName::A,       3>();
		case SDL_SCANCODE_7:            return note<NoteName::A_SHARP, 3>();
		case SDL_SCANCODE_U:            return note<NoteName::B,       3>();
		case SDL_SCANCODE_I:            return note<NoteName::C,       4>();
		case SDL_SCANCODE_9:            return note<NoteName::C_SHARP, 4>();
		case SDL_SCANCODE_O:            return note<NoteName::D,       4>();
		case SDL_SCANCODE_0:            return note<NoteName::D_SHARP, 4>();
		case SDL_SCANCODE_P:            return note<NoteName::E,       4>();
		case SDL_SCANCODE_LEFTBRACKET:  return note<NoteName::F,       4>();
		case SDL_SCANCODE_EQUALS:       return note<NoteName::F_SHARP, 4>();
		case SDL_SCANCODE_RIGHTBRACKET: return note<NoteName::G,       4>();

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

void util::note_name(int note, char str[], int len) {
	if (note < 0) {
		str[0] = '\0';
		return;
	}

	auto tone   = note % 12;
	auto octave = note / 12;

	constexpr char const * tones[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	sprintf_s(str, len, "%s%i", tones[tone], octave);
}

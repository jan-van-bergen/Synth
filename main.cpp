#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <unordered_map>

#include <SDL2/SDL.h>

static constexpr float     PI = 3.14159265359f;
static constexpr float TWO_PI = 6.28318530718f;

static constexpr float C_0 = 16.35f; // Tuning of C0 in Hz

static constexpr int   SAMPLE_RATE     = 44100;
static constexpr float SAMPLE_RATE_INV = 1.0f / float(SAMPLE_RATE);

static constexpr int WINDOW_WIDTH  = 1280;
static constexpr int WINDOW_HEIGHT = 720;

static float note_freq(int note) {
	assert(note >= 0);

	float freq = C_0;

	// Find octave
	while (note >= 12) {
		freq *= 2.0f;
		note -= 12;
	}

	// Find note within octave
	static constexpr float pow[12] = { // LUT with 2^(i/12) at index i
		 1.0f,
		 1.059463094f,
		 1.122462048f,
		 1.189207115f,
		 1.259921050f,
		 1.334839854f,
		 1.414213562f,
		 1.498307077f,
		 1.587401052f,
		 1.681792831f,
		 1.781797436f,
		 1.887748625f
	};

	return freq * pow[note];
}

static int scancode_to_note(SDL_Scancode scancode) {
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

		case SDL_SCANCODE_Q:            return 36;  // C
		case SDL_SCANCODE_2:            return 37;  // C#
		case SDL_SCANCODE_W:            return 38;  // D
		case SDL_SCANCODE_3:            return 39;  // D#
		case SDL_SCANCODE_E:            return 40;  // E
		case SDL_SCANCODE_R:            return 41;  // F
		case SDL_SCANCODE_5:            return 42;  // F#
		case SDL_SCANCODE_T:            return 43;  // G
		case SDL_SCANCODE_6:            return 44;  // G#
		case SDL_SCANCODE_Y:            return 45;  // A
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

static float play_sine(float t, float freq, float amplitude = 1.0f) {
	return amplitude * std::sin(TWO_PI * t * freq);
}

static float play_saw(float t, float freq, float amplitude = 1.0f) {
	return amplitude * 2.0f * (t * freq - std::floor(t * freq + 0.5f));
}

static float play_square(float t, float freq, float amplitude = 1.0f) {
	return std::fmodf(t * freq, 1.0f) < 0.5f ? amplitude : -amplitude;
}

static float play_triangle(float t, float freq, float amplitude = 1.0f) {
	float x = t * freq + 0.25f;
	return amplitude * (4.0f * std::abs(x - std::floor(x + 0.5f)) - 1.0f);
}

static float t = 0.0f;

static float mouse_x = 0.0f;
static float mouse_y = 0.0f;

struct Note {
	float start_time;

	int note_idx;
};

static std::unordered_map<int, Note> notes;

static float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

static float clamp(float value, float min = 0.0f, float max = 1.0f) {
	if (value < min) return min;
	if (value > max) return max;

	return value;
}

static float envelope(float t) {
	static constexpr float attack  = 0.1f;
	static constexpr float hold    = 0.5f;
	static constexpr float decay   = 1.0f;
	static constexpr float sustain = 0.5f;
//	static constexpr float release = 1.0f;

	if (t < attack) {
		return t / attack;
	}
	t -= attack;

	if (t < hold) {
		return 1.0f;
	}
	t -= hold;

	if (t < decay) {
		return lerp(1.0f, sustain, t / decay);
	}

	return sustain;
}

static float bitcrush(float signal, float crush = 16.0f) {
	return crush * std::floor(signal / crush);
}

static float distort(float signal, float amount = 0.2f) {
	float cut = 0.5f * (1.0f - amount) + 0.00001f;

	if (signal < cut) {
		return lerp(0.0f, 1.0f - cut, signal / cut);
	} else {
		return lerp(1.0f - cut, 1.0f, (signal - cut) / (1.0f - cut));
	}
}

static float filter(float sample, float cutoff = 1000.0f, float resonance = 0.5f) {
	static float z1_A[2];
	static float z2_A[2];

	auto Q = 1.0f / (2.0f * (1.0f - resonance));

	auto g = std::tan(PI * cutoff * SAMPLE_RATE_INV); // Gain
	auto R = 1.0f / (2.0f * Q);	                      // Damping
    
	auto high_pass = (sample - (2.0f * R + g) * z1_A[0] - z2_A[0]) / (1.0f + (2.0f * R * g) + g * g);
	auto band_pass = high_pass * g + z1_A[0];
	auto  low_pass = band_pass * g + z2_A[0];
	
	z1_A[0] = g * high_pass + band_pass;
	z2_A[0] = g * band_pass + low_pass;

	return low_pass;
}

static float delay(float sample, float feedback = 0.6f) {
	static constexpr auto HISTORY_SIZE = SAMPLE_RATE * 462 / 1000;
	static float history[HISTORY_SIZE];
	static int offset = 0;

	auto offset_prev = offset;
	offset = (offset + 1) % HISTORY_SIZE;

	sample = sample + feedback * history[offset];
	history[offset_prev] = sample;
	
	return sample;
}

static void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
	for (int i = 0; i < len; i += 2) {
		auto sample = 0.0f;
		
		for (auto const & [note_idx, note] : notes) {
			float duration = t - note.start_time;

			sample += play_saw(t, note_freq(note_idx), 32.0f * envelope(duration));
		}

		sample = filter(sample, lerp(100.0f, 10000.0f, mouse_x), lerp(0.5f, 1.0f, mouse_y));
		sample = delay(sample);

		stream[i]     = char(clamp(sample, -128.0f, 127.0f));
		stream[i + 1] = char(clamp(sample, -128.0f, 127.0f));

		t += SAMPLE_RATE_INV;
	}
}

int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_EVERYTHING);

	auto window  = SDL_CreateWindow("Synthesizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	auto context = SDL_GL_CreateContext(window);

	SDL_GL_SetSwapInterval(0);

	SDL_AudioSpec audio_spec = { };
	audio_spec.freq = SAMPLE_RATE;
	audio_spec.format = AUDIO_S8;
	audio_spec.channels = 2;
	audio_spec.samples = 1024;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	// time_inv_freq = 1.0f / float(SDL_GetPerformanceFrequency());
	// time_last = SDL_GetPerformanceCounter();

	SDL_PauseAudioDevice(device, 0);

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					auto note = scancode_to_note(event.key.keysym.scancode);
					if (note != -1) {
						Note n = { t, note };	
						notes.insert(std::make_pair(note, n));
					}

					break;
				}

				case SDL_KEYUP: {
					auto note = scancode_to_note(event.key.keysym.scancode);
					if (note != -1) notes.erase(note);

					break;
				}

				default: break;
			}
		}

		int x, y; SDL_GetMouseState(&x, &y);
		mouse_x = float(x) / float(WINDOW_WIDTH);
		mouse_y = float(y) / float(WINDOW_HEIGHT);

		SDL_GL_SwapWindow(window);
	}

	SDL_CloseAudioDevice(device);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

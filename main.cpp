#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <unordered_set>

#include <SDL2/SDL.h>

static constexpr float TWO_PI = 6.28318530718f;

static constexpr float A_0 = 27.5f; // Tuning of A 0 in Hz

static constexpr int SAMPLE_RATE = 44100;

static float note_freq(int note) {
	return A_0 * std::pow(2.0f, float(note) / 12.0f);
}

static int scancode_to_note(SDL_Scancode scancode) {
	switch (scancode) {
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
		case SDL_SCANCODE_RIGHTBRACKET: return 54; // G

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

static Uint64 time_last;
static float  time_inv_freq;

static std::unordered_set<int> notes;

static void sdl_audio_callback(void* user_data, Uint8* stream, int len) {
	for (int i = 0; i < len; i += 2) {
		char val = 0;
		
		for (auto note : notes) {
			val += play_saw(t, note_freq(note), 32.0f);
		}

		stream[i]     = val;
		stream[i + 1] = val;

		t += 1.0f / float(SAMPLE_RATE);
	}
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_EVERYTHING);

	auto window = SDL_CreateWindow("Synthesizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);

	SDL_AudioSpec audio_spec = { };
	audio_spec.freq = SAMPLE_RATE;
	audio_spec.format = AUDIO_S8;
	audio_spec.channels = 2;
	audio_spec.samples = 1024;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	time_inv_freq = 1.0f / float(SDL_GetPerformanceFrequency());
	time_last = SDL_GetPerformanceCounter();

	SDL_PauseAudioDevice(device, 0);

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					auto note = scancode_to_note(event.key.keysym.scancode);
					if (note != -1) notes.insert(note);

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

		SDL_Delay(16);
	}

	SDL_CloseAudioDevice(device);
	SDL_Quit();

	return 0;
}

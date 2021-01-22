#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <SDL2/SDL.h>

static constexpr float TWO_PI = 6.28318530718f;

static constexpr float A_0 = 27.5f; // Tuning of A 0 in Hz

static constexpr int SAMPLE_RATE = 44100;

static float note_freq(int note) {
	return A_0 * std::pow(2.0f, float(note) / 12.0f);
}

static float play_sine(float t, float freq, float amplitude = 1.0f) {
	return amplitude * std::sin(TWO_PI * t * freq);
}

static float play_saw(float t, float freq, float amplitude = 1.0f) {
	return amplitude * 2.0f * (t*freq - std::floor(t*freq + 0.5f));
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

static void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
	int jakob[] = { 
		0,0, 2,2, 4,4, 0,0,
		0,0, 2,2, 4,4, 0,0, 
		4,4, 5,5, 7,7, 7,7,
		4,4, 5,5, 7,7, 7,7, 
		7,9, 7,5, 4,4, 0,0,
		7,9, 7,5, 4,4, 0,0,
		0,0, -5,-5, 0,0, 0,0,
		0,0, -5,-5, 0,0, 0,0
	};

	for (int i = 0; i < len; i += 2) {
		auto val = 
			char(play_square  (t, note_freq(36 + jakob[(int(t * 2))      % 64]), 48.0f)) +
			char(play_saw     (t, note_freq(24 + jakob[(int(t * 2) + 16) % 64]), 32.0f)) + 
			char(play_triangle(t, note_freq(12 + jakob[(int(t * 2) + 32) % 64]), 36.0f));

		stream[i]     = val;
		stream[i + 1] = val;
		
		t += 1.0f / float(SAMPLE_RATE);
	}
}

int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec audio_spec = { };
	audio_spec.freq     = SAMPLE_RATE;
	audio_spec.format   = AUDIO_S8;
	audio_spec.channels = 2;
	audio_spec.samples  = 1024;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	time_inv_freq = 1.0f / float(SDL_GetPerformanceFrequency());
	time_last     = SDL_GetPerformanceCounter();

	SDL_PauseAudioDevice(device, 0);

	while (true) SDL_Delay(1000);

	SDL_CloseAudioDevice(device);
	SDL_Quit();

	return 0;
}

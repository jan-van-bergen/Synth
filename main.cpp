#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <SDL2/SDL.h>

constexpr float TWO_PI = 6.28318530718f;

constexpr float A_0 = 27.5f; // Tuning of A 0 in Hz

float note_freq(int note) {
    return A_0 * std::pow(2.0f, float(note) / 12.0f);
}

constexpr int SAMPLE_RATE = 44100;

static float t = 0.0f;

void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
    int jakob[] = { 0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 7, 4, 5, 7, 7 };

    for (int i = 0; i < len; i++) {
        char sample = std::sin(TWO_PI * t * note_freq(36 + jakob[int(t) % 16])) * 512.0f;
        stream[i] = sample / 4;

        t += 1.0f / float(SAMPLE_RATE);
    }
}

int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec audio_spec = { };
    audio_spec.freq     = SAMPLE_RATE;
    audio_spec.format   = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples  = 1024;
    audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

    // unpausing the audio device (starts playing):
    SDL_PauseAudioDevice(device, 0);

    SDL_Delay(30000);

	SDL_CloseAudioDevice(device);
	SDL_Quit();

	return 0;
}

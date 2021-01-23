#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <atomic>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "midi.h"

struct Sample {
	float left;
	float right;

	Sample()                 : left(0), right(0) { }
	Sample(float f)          : left(f), right(f) { }
	Sample(float l, float r) : left(l), right(r) { }

	static Sample floor(Sample const & s) { return { std::floor(s.left), std::floor(s.right) }; }

	static Sample lerp(Sample const & a, Sample const & b, Sample const & t);

	void operator+=(Sample const & other) { left += other.left; right += other.right; }
	void operator-=(Sample const & other) { left -= other.left; right -= other.right; }
	void operator*=(Sample const & other) { left *= other.left; right *= other.right; }
	void operator/=(Sample const & other) { left /= other.left; right /= other.right; }

	void operator+=(float f) { left += f; right += f; }
	void operator-=(float f) { left -= f; right -= f; }
	void operator*=(float f) { left *= f; right *= f; }
	void operator/=(float f) { left /= f; right /= f; }
};

Sample operator+(Sample const & a, Sample const & b) { return { a.left + b.left, a.right + b.right }; }
Sample operator-(Sample const & a, Sample const & b) { return { a.left - b.left, a.right - b.right }; }
Sample operator*(Sample const & a, Sample const & b) { return { a.left * b.left, a.right * b.right }; }
Sample operator/(Sample const & a, Sample const & b) { return { a.left / b.left, a.right / b.right }; }

Sample operator+(Sample const & s, float f) { return { s.left + f, s.right + f }; }
Sample operator-(Sample const & s, float f) { return { s.left - f, s.right - f }; }
Sample operator*(Sample const & s, float f) { return { s.left * f, s.right * f }; }
Sample operator/(Sample const & s, float f) { return { s.left / f, s.right / f }; }

Sample operator+(float f, Sample const & s) { return { s.left + f, s.right + f }; }
Sample operator-(float f, Sample const & s) { return { s.left - f, s.right - f }; }
Sample operator*(float f, Sample const & s) { return { s.left * f, s.right * f }; }
Sample operator/(float f, Sample const & s) { return { s.left / f, s.right / f }; }

Sample Sample::lerp(Sample const & a, Sample const & b, Sample const & t) { return a + (b - a) * t; }

struct Note {
	float start_time;

	int note_idx;
};


static constexpr auto     PI = 3.14159265359f;
static constexpr auto TWO_PI = 6.28318530718f;


static constexpr auto SAMPLE_RATE     = 44100;
static constexpr auto SAMPLE_RATE_INV = 1.0f / float(SAMPLE_RATE);


static constexpr int BUFFER_SIZE  = 1024;
static constexpr int BUFFER_COUNT = 3;

static Sample buffers[BUFFER_COUNT][BUFFER_SIZE];

static std::atomic<int> buffer_read  = 0;
static std::atomic<int> buffer_write = 0;


static constexpr auto WINDOW_WIDTH  = 1280;
static constexpr auto WINDOW_HEIGHT = 720;


static float note_freq(int note) {
	assert(note >= 0);
	
	static constexpr float C_0 = 16.35f; // Tuning of C0 in Hz

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


static Sample play_sine(float t, float freq, float amplitude = 1.0f) {
	return amplitude * std::sin(TWO_PI * t * freq);
}

static Sample play_saw(float t, float freq, float amplitude = 1.0f) {
	return amplitude * 2.0f * (t * freq - std::floor(t * freq + 0.5f));
}

static Sample play_square(float t, float freq, float amplitude = 1.0f) {
	return std::fmodf(t * freq, 1.0f) < 0.5f ? amplitude : -amplitude;
}

static Sample play_triangle(float t, float freq, float amplitude = 1.0f) {
	float x = t * freq + 0.25f;
	return amplitude * (4.0f * std::abs(x - std::floor(x + 0.5f)) - 1.0f);
}


template<typename T>
static float lerp(T a, T b, float t) {
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


static Sample bitcrush(Sample signal, float crush = 16.0f) {
	return crush * Sample::floor(signal / crush);
}

static float distort(float signal, float amount = 0.2f) {
	float cut = 0.5f * (1.0f - amount) + 0.00001f;

	if (signal < cut) {
		return lerp(0.0f, 1.0f - cut, signal / cut);
	} else {
		return lerp(1.0f - cut, 1.0f, (signal - cut) / (1.0f - cut));
	}
}

static Sample filter(Sample sample, float cutoff = 1000.0f, float resonance = 0.5f) {
	static Sample z1;
	static Sample z2;

	auto g = std::tan(PI * cutoff * SAMPLE_RATE_INV); // Gain
	auto R = 1.0f - resonance;                        // Damping
    
	auto high_pass = (sample - (2.0f * R + g) * z1 - z2) / (1.0f + (2.0f * R * g) + g * g);
	auto band_pass = high_pass * g + z1;
	auto  low_pass = band_pass * g + z2;
	
	z1 = g * high_pass + band_pass;
	z2 = g * band_pass + low_pass;

	return low_pass;
}

static float delay(float sample, float feedback = 0.7f) {
	static constexpr auto HISTORY_SIZE = SAMPLE_RATE * 462 / 1000;
	static float history[HISTORY_SIZE];
	static int offset = 0;

	sample = sample + feedback * history[offset];
	history[offset] = sample;
	
	offset = (offset + 1) % HISTORY_SIZE;

	return sample;
}


static void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
	assert(len == 2 * BUFFER_SIZE);

	auto buffer_curr = buffer_read.load();
	while (buffer_curr == buffer_write.load()) { }
	
	assert(buffer_curr >= 0);

	auto buf = buffers[buffer_curr];

	for (int i = 0; i < BUFFER_SIZE; i++) {
		stream[2*i    ] = (char)clamp(buf[i].left,  -128.0f, 127.0f);
		stream[2*i + 1] = (char)clamp(buf[i].right, -128.0f, 127.0f);
	}
	
	buffer_read.store((buffer_curr + 1) % BUFFER_COUNT);
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
	audio_spec.samples = BUFFER_SIZE;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	auto time_inv_freq = 1.0 / double(SDL_GetPerformanceFrequency());
	auto time_start    = double(SDL_GetPerformanceCounter());

	SDL_PauseAudioDevice(device, false);

	auto midi = MidiTrack::load("loop.mid");
	auto midi_offset = 0;
	
	auto t = 0.0f;
	
	std::unordered_map<int, Note> notes;

	auto mouse_x = 0.0f;
	auto mouse_y = 0.0f;

	auto window_is_open = true;

	while (window_is_open) {
		auto time = (double(SDL_GetPerformanceCounter()) - time_start) * time_inv_freq;
		
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

				case SDL_QUIT: window_is_open = false; break;

				default: break;
			}
		}

		//while (midi_offset < midi.events.size()) {
		//	auto const & event = midi.events[midi_offset];

		//	if (event.time > time) break;

		//	if (event.press) {
		//		Note n = { t, event.note };
		//		notes.insert(std::make_pair(event.note, n));
		//	} else {
		//		notes.erase(event.note);
		//	}

		//	midi_offset++;
		//}
		
		int x, y; SDL_GetMouseState(&x, &y);
		mouse_x = float(x) / float(WINDOW_WIDTH);
		mouse_y = float(y) / float(WINDOW_HEIGHT);

		auto buffer_curr = buffer_write.load();
		auto buf         = buffers[buffer_curr];
		
		for (int i = 0; i < BUFFER_SIZE; i++) {
			Sample sample;
		
			for (auto const & [note_idx, note] : notes) {
				float duration = t - note.start_time;

				sample += play_saw(t, note_freq(note_idx), 20.0f * envelope(duration));
			}

//			sample = filter(sample, lerp(100.0f, 10000.0f, mouse_x), lerp(0.5f, 1.0f, mouse_y));
//			sample = delay(sample);

			buf[i] = sample;

			t += SAMPLE_RATE_INV;
		}
		
		auto buffer_next = (buffer_curr + 1) % BUFFER_COUNT;
		while (buffer_next == buffer_read.load()) { }
		
		buffer_write.store(buffer_next);
		
		SDL_GL_SwapWindow(window);
	}

	SDL_CloseAudioDevice(device);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

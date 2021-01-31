#include "util.h"

#include <cassert>

#include <immintrin.h>

int util::float_to_int(float f) {
	f -= copysignf(0.5f, f); // Round up/down based on sign of f
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

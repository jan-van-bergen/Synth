#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <unordered_map>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_sdl.h>
#include <ImGui/imgui_impl_opengl3.h>

#include "util.h"
#include "ring_buffer.h"

#include "midi.h"
#include "sample.h"

#include "components.h"

// extern "C" { _declspec(dllexport) unsigned NvOptimusEnablement = true; }


static RingBuffer<Sample[BLOCK_SIZE], 3> buffers;

static constexpr auto WINDOW_WIDTH  = 1280;
static constexpr auto WINDOW_HEIGHT = 720;



static Sample bitcrush(Sample signal, float crush = 16.0f) {
	return crush * Sample::floor(signal / crush);
}

static float distort(float signal, float amount = 0.2f) {
	float cut = 0.5f * (1.0f - amount) + 0.00001f;

	if (signal < cut) {
		return util::lerp(0.0f, 1.0f - cut, signal / cut);
	} else {
		return util::lerp(1.0f - cut, 1.0f, (signal - cut) / (1.0f - cut));
	}
}


static void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
	assert(len == 2 * BLOCK_SIZE);

	auto buf = buffers.get_read();

	for (int i = 0; i < BLOCK_SIZE; i++) {
		stream[2*i    ] = (char)util::clamp(buf[i].left,  -128.0f, 127.0f);
		stream[2*i + 1] = (char)util::clamp(buf[i].right, -128.0f, 127.0f);
	}
	
	buffers.advance_read();
}


int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	auto window  = SDL_CreateWindow("Synthesizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	auto context = SDL_GL_CreateContext(window);
	
	SDL_SetWindowResizable(window, SDL_TRUE);
	
	SDL_GL_SetSwapInterval(0);

	auto status = glewInit();
	if (status != GLEW_OK) {
		printf("Glew failed to initialize!\n");
		abort();
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init("#version 330");
	
	SDL_AudioSpec audio_spec = { };
	audio_spec.freq     = SAMPLE_RATE;
	audio_spec.format   = AUDIO_S8;
	audio_spec.channels = 2;
	audio_spec.samples  = BLOCK_SIZE;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	auto time_inv_freq = 1.0 / double(SDL_GetPerformanceFrequency());
	auto time_start    = double(SDL_GetPerformanceCounter());

	SDL_PauseAudioDevice(device, false);

	midi::open();

	auto midi = midi::Track::load("loop.mid");
	auto midi_offset = 0;
	auto midi_rounds = 0;
	
	auto mouse_x = 0.0f;
	auto mouse_y = 0.0f;

	Synth synth;
	auto oscilator = synth.add_component<OscilatorComponent>();
	auto filter    = synth.add_component<FilterComponent>();
	auto delay     = synth.add_component<DelayComponent>();
	auto speaker   = synth.add_component<SpeakerComponent>();

	synth.connect(oscilator->outputs[0], filter->inputs[0]);
	synth.connect(filter->outputs[0], delay->inputs[0]);
	synth.connect(delay->outputs[0], speaker->inputs[0]);
	
	auto window_is_open = true;

	while (window_is_open) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_KEYDOWN: {
					auto note = util::scancode_to_note(event.key.keysym.scancode);
					if (note != -1) {
						synth.note_press(note);
					}

					break;
				}

				case SDL_KEYUP: {
					auto note = util::scancode_to_note(event.key.keysym.scancode);
					if (note != -1) {
						synth.note_release(note);
					}

					break;
				}

				case SDL_QUIT: window_is_open = false; break;

				default: break;
			}
		}

		/*
		auto midi_ticks_to_samples = [](size_t time, size_t tempo, size_t ticks) -> size_t {
			return SAMPLE_RATE * time * tempo / 1000000 / ticks;
		};
		
		auto midi_length = midi_ticks_to_samples(midi.events[midi.events.size() - 1].time, midi.tempo, midi.ticks);
		auto a = time / midi_length;
		auto t = time % midi_length;

		while (true) {
			auto const & midi_event = midi.events[midi_offset];

			auto midi_time = midi_ticks_to_samples(midi_event.time, midi.tempo, midi.ticks);

			if (midi_time > t && a == midi_rounds) break;

			if (midi_event.type == midi::Event::Type::PRESS) {
				Note n = { midi_event.note.note, 1.0f, time };	
				notes.insert(std::make_pair(midi_event.note.note, n));
			} else if (midi_event.type == midi::Event::Type::RELEASE) {
				 notes.erase(midi_event.note.note);
			}

			midi_offset++;
			if (midi_offset == midi.events.size()) {
				midi_offset = 0;
				midi_rounds++;
			}
		}
		*/

		while (true) {
			auto midi_event = midi::get_event();
			if (!midi_event.has_value()) break;

			auto const & event = midi_event.value();

			switch (event.type) {
				case midi::Event::Type::PRESS: {
					synth.note_press(event.note.note, event.note.velocity / 255.0f);
					break;
				}
				case midi::Event::Type::RELEASE: synth.note_release(event.note.note); break;
				case midi::Event::Type::CONTROL: synth.control_update(event.control.id, event.control.value / 127.0f); break;

				default: break;
			}
		}

		int x, y; SDL_GetMouseState(&x, &y);
		mouse_x = float(x) / float(WINDOW_WIDTH);
		mouse_y = float(y) / float(WINDOW_HEIGHT);

		auto buf = buffers.get_write();
			
//			sample = filter(sample, lerp(100.0f, 10000.0f, controls[0x4A]), lerp(0.5f, 1.0f, controls[0x47]));
//			sample = delay(sample);
//			sample = bitcrush(sample, 8.0f);

		synth.update(buf);

		buffers.advance_write();
		
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		// ImGui::SetNextWindowPos (ImVec2(0, 0));
		// ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

		synth.render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}

	midi::close();
	
	ImGui_ImplSDL2_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	SDL_CloseAudioDevice(device);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

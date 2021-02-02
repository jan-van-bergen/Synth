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

#include "synth.h"

extern "C" { _declspec(dllexport) unsigned NvOptimusEnablement = true; }


static RingBuffer<Sample[BLOCK_SIZE], 3> buffers;
static bool terminated = false;

static constexpr auto WINDOW_WIDTH  = 1600;
static constexpr auto WINDOW_HEIGHT = 900;


static void sdl_audio_callback(void * user_data, Uint8 * stream, int len) {
	assert(len == BLOCK_SIZE * sizeof(Sample));

	if (terminated) return;

	auto buf = buffers.get_read();
	memcpy(stream, buf, len);
	
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
	audio_spec.format   = AUDIO_F32;
	audio_spec.channels = 2;
	audio_spec.samples  = BLOCK_SIZE;
	audio_spec.callback = sdl_audio_callback;

	auto device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

	auto time_inv_freq = 1.0 / double(SDL_GetPerformanceFrequency());
	auto time_start    = double(SDL_GetPerformanceCounter());

	SDL_PauseAudioDevice(device, false);

	midi::open();

	Synth synth;
	synth.add_component<SpeakerComponent>();
	
	auto window_is_open = true;

	while (window_is_open) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			
			switch (event.type) {
				case SDL_KEYDOWN: {
					auto note = util::scancode_to_note(event.key.keysym.scancode);
					if (note != -1) {
						synth.note_press(note, 0.2f);
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

		auto buf = buffers.get_write();
		synth.update(buf);

		buffers.advance_write();
		
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		synth.render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}

	terminated = true;

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

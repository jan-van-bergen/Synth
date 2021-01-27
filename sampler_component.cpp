#include "components.h"

void SamplerComponent::load() {
	Uint32        wav_length;
	Uint8       * wav_buffer;
	SDL_AudioSpec wav_spec;

	if (SDL_LoadWAV(filename, &wav_spec, &wav_buffer, &wav_length) == nullptr) {
		printf("ERROR: Unable to load sample '%s'!\n", filename);
		return;
	}

	if (wav_spec.channels != 1 && wav_spec.channels != 2) {
		printf("ERROR: Sample '%s' has %i channels! Should be either 1 (Mono) or 2 (Stereo)\n", filename, wav_spec.channels);
		return;
	}

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
}

void SamplerComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		static constexpr auto EPSILON = 0.001f;

		auto abs = Sample::apply_function(std::fabsf, inputs[0].get_value(i));
		if (abs.left > EPSILON && abs.right > EPSILON) {
			velocity = 127.0f * abs.left;
			current_sample = 0; // Trigger sample on input
		}

		if (current_sample < samples.size()) {
			outputs[0].values[i] = velocity * samples[current_sample];
		}

		current_sample++;
	}
}

void SamplerComponent::render(Synth const & synth) {
	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) load();
}

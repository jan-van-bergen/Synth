#include "components.h"

#include "synth.h"

void ImproviserComponent::update(Synth const & synth) {
	constexpr auto transfer = util::generate_lookup_table<float, 7 * 7>([](int index) -> float {
		constexpr float matrix[7 * 7] = {
			1.0f, 2.0f, 2.0f, 3.0f, 5.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 2.0f, 1.0f, 5.0f, 3.0f, 2.0f,
			2.0f, 1.0f, 1.0f, 2.0f, 3.0f, 2.0f, 3.0f,
			2.0f, 1.0f, 2.0f, 1.0f, 2.0f, 2.0f, 1.0f,
			7.0f, 1.0f, 2.0f, 2.0f, 1.0f, 2.0f, 3.0f,
			3.0f, 2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 2.0f,
			5.0f, 2.0f, 2.0f, 1.0f, 1.0f, 2.0f, 1.0f
		};

		auto row    = index / 7;
		auto offset = index % 7;

		auto cumulative = 0.0f;
		auto row_sum    = 0.0f;

		for (int i = 0; i < 7; i++) {
			auto val = matrix[row * 7 + i];

			if (i <= offset) cumulative += val;
			row_sum += val;
		}

		return cumulative / row_sum;
	});

	constexpr int const MAJOR_SCALE[7] = { 0, 2, 4, 5, 7, 9, 11 };
	constexpr int const MINOR_SCALE[7] = { 0, 2, 3, 5, 7, 8, 10 };

	constexpr auto BASE_NOTE = 36;
	constexpr auto VELOCITY  = 0.5f;

	auto const scale = mode == Mode::MAJOR ? MAJOR_SCALE : MINOR_SCALE;

	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);
	auto samples = 16.0f * SAMPLE_RATE / steps_per_second;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		if (current_time >= samples) {
			for (auto note : chord) {
				outputs[0].add_event(NoteEvent::make_release(synth.time + i, note));		
			}
			chord.clear();

			auto u = rand() / float(RAND_MAX);

			auto next_chord = 0;
			while (next_chord < 7 && transfer[current_chord * 7 + next_chord] < u) next_chord++;

			current_chord = next_chord;
			current_time = 0;
			
			for (int n = 0; n < num_notes; n++) {
				auto note = BASE_NOTE + tonality + scale[util::wrap(current_chord + 2*n, 7)];

				chord.emplace_back(note);

				outputs[0].add_event(NoteEvent::make_press(synth.time + i, note, VELOCITY));
			}
		}

		current_time++;
	}
}

void ImproviserComponent::render(Synth const & synth) {
	tonality.render(); ImGui::SameLine();
	num_notes.render();

	auto mode_index = int(mode);

	if (ImGui::BeginCombo("Mode", mode_names[mode_index])) {
		for (int i = 0; i < util::array_element_count(mode_names); i++) {
			if (ImGui::Selectable(mode_names[i], mode_index == i)) {
				mode_index = i;
			}
		}

		ImGui::EndCombo();
	}

	mode = Mode(mode_index);
}

void ImproviserComponent::serialize_custom(json::Writer & writer) const {
	writer.write("mode", int(mode));
}

void ImproviserComponent::deserialize_custom(json::Object const & object) {
	mode = Mode(object.find_int("mode"));
}

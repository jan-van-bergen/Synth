#include "arp.h"

#include "synth/synth.h"

void ArpComponent::update(Synth const & synth) {
	auto note_events = inputs[0].get_events();

	auto first_note = false;
	auto start_time = synth.time + BLOCK_SIZE;

	// Handle MIDI inputs
	for (auto const & note_event : note_events) {
		// Try to find existing entry of the same note
		auto existing = std::find_if(notes.begin(), notes.end(), [n = note_event.note](auto note) {
			return note.note == n;	
		});

		auto already_exists = existing != notes.end();

		if (note_event.pressed) {
			if (already_exists) {
				existing->velocity = note_event.velocity;
			} else {
				if (notes.empty()) {
					first_note = true;
				}

				Note note = { note_event.note, note_event.velocity };

				auto index = util::binary_search(notes, note);
				
				if (index <= current_note) current_note++;
				notes.insert(notes.begin() + index, note);

				start_time = std::min(start_time, note_event.time);
			}
		} else if (already_exists){
			auto note_index = existing - notes.begin();

			notes.erase(existing);

			if (current_note == note_index) {
				outputs[0].add_event(NoteEvent::make_release(0, note_event.note));
			}

			if (current_note > 0 && current_note >= note_index) current_note--;
		}
	}

	if (first_note) {
		current_note = mode == Mode::DOWN ? notes.size() - 1 : 0;
		current_time = synth.time - start_time;

		going_up = true;

		outputs[0].add_event(NoteEvent::make_press(start_time, notes[current_note].note, notes[current_note].velocity));
	}
	
	if (notes.size() <= 1) return;

	assert(0 <= current_note && current_note < notes.size());
	
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);
	auto samples = steps * SAMPLE_RATE / steps_per_second;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		if (current_time > samples) {
			// Release previous note, if there was one
			if (!first_note) outputs[0].add_event(NoteEvent::make_release(synth.time + i, notes[current_note].note));

			switch (mode){
				case Mode::UP:   going_up = true;  break;
				case Mode::DOWN: going_up = false; break;
				case Mode::UPDOWN: {
					if (going_up) {
						if (current_note == notes.size() - 1) going_up = false;
					} else {
						if (current_note == 0) going_up = true;
					}

					break;
				}
				case Mode::RANDOM: going_up = util::rand(seed) & 1; break;

				default: abort();
			}
			
			auto next_note = going_up ? current_note + 1 : current_note - 1;

			current_note = util::wrap<int>(next_note, notes.size());
			current_time -= samples;

			outputs[0].add_event(NoteEvent::make_press(synth.time + i, notes[current_note].note, notes[current_note].velocity));

			first_note = false;
		}

		current_time += 1.0f;
	}
}

void ArpComponent::render(Synth const & synth) {
	auto mode_index = int(mode);

	if (ImGui::BeginCombo("Mode", mode_names[mode_index])) {
		for (int j = 0; j < util::array_count(mode_names); j++) {
			if (ImGui::Selectable(mode_names[j], mode_index == j)) {
				mode_index = j;
			}
		}

		ImGui::EndCombo();
	}

	mode = Mode(mode_index);

	steps.render();

	char note_name[8] = { };

	for (int i = notes.size() - 1; i >= 0; i--) {
		util::note_name(notes[i].note, note_name, sizeof(note_name));
		ImGui::Text("%s %s", note_name, i == current_note ? "<-" : "");
	}

	if (notes.size() > 0) {
		auto note = notes.end();

		do {
			note--;
			
		} while (note != notes.begin());
	}
}

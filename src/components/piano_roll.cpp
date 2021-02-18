#include "components.h"

#include "synth.h"

void PianoRollComponent::reload_file() {
	// Release currently pressed notes
	for (auto const & note : notes) {
		outputs[0].add_event(NoteEvent::make_release(0, note.note));
	}

	notes.clear();

	auto track = midi::Track::load(filename);

	if (!track.has_value()) {
		midi_offset = 0;
		midi_length = 0;
		return;
	}

	midi = std::move(track.value());

	if (midi.events.size() == 0) {
		midi_length = 0;
	} else {		
		midi_length = util::round_up(midi.events[midi.events.size() - 1].time, 4 * midi.ticks);
	}

	midi_offset = 0;
}

void PianoRollComponent::update(Synth const & synth) {
	auto midi_ticks_to_samples = [](size_t time, size_t tempo, size_t ticks, size_t bpm) -> size_t {
		return SAMPLE_RATE * time * 60 / (bpm * ticks);
	};

	if (midi_length == 0) return;

	auto sixteenth_note = size_t(60) * SAMPLE_RATE / (4 * synth.settings.tempo);

	auto length = midi_ticks_to_samples(midi_length, midi.tempo, midi.ticks, synth.settings.tempo);

	auto time_start = synth.time % length;
	auto time_end   = time_start + BLOCK_SIZE;
	
	auto wrapped = false;
	size_t time_wrap = 0;

	if (time_start > midi_ticks_to_samples(midi.events[midi.events.size() - 1].time, midi.tempo, midi.ticks, synth.settings.tempo)) {
		if (time_end < length) return;

		time_wrap = length;
	}

	auto t = time_end;

	while (true) {
		auto const & midi_event = midi.events[midi_offset];

		auto midi_time = time_wrap + midi_ticks_to_samples(midi_event.time, midi.tempo, midi.ticks, synth.settings.tempo);
		if  (midi_time > t) break;

		int time_offset = midi_time - time_start;

		if (midi_event.type == midi::Event::Type::PRESS) {
			outputs[0].add_event(NoteEvent::make_press(synth.time + time_offset, midi_event.note.note, midi_event.note.velocity / 128.0f));

			notes.emplace_back(midi_event.note.note, midi_event.note.velocity);
		} else if (midi_event.type == midi::Event::Type::RELEASE) {
			outputs[0].add_event(NoteEvent::make_release(synth.time + time_offset, midi_event.note.note));
			
			auto found = std::find_if(notes.begin(), notes.end(), [n = midi_event.note.note](auto note) {
				return note.note = n;
			});

			if (found != notes.end()) notes.erase(found);
		}

		midi_offset++;
		if (midi_offset == midi.events.size()) {
			midi_offset = 0;

			time_wrap += length;

			assert(!wrapped);
			wrapped = true;
		}
	}
}

void PianoRollComponent::render(Synth const & synth) {
	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) reload_file();
}

void PianoRollComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filename", filename);
}

void PianoRollComponent::deserialize_custom(json::Object const & object) {
	auto name = object.find_string("filename", DEFAULT_FILENAME);
	strcpy_s(filename, name.c_str());

	reload_file();
}

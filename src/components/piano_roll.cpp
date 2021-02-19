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

	// Draw keyboard
	static constexpr auto KEY_WIDTH  = 12.0f;
	static constexpr auto KEY_HEIGHT = 48.0f;

	static constexpr auto BASE_NOTE = 36;
	static_assert(BASE_NOTE % 12 == 0, "BASE_NOTE should be a C!");

	auto const colour_black = ImColor( 50,  50,  50, 255);
	auto const colour_white = ImColor(250, 250, 250, 255);
	auto const colour_red   = ImColor(250, 100, 100, 255);

	int const note_offsets_white[7] = { 0, 2,  4, 5, 7, 9,  11 }; // Offsets within octave of C D E F G A B
	int const note_offsets_black[7] = { 1, 3, -1, 6, 8, 10, -1 }; // Offsets within octave of C# D# *skip* F# G# A# *skip*
	
	auto avail = ImGui::GetContentRegionAvailWidth();

	auto window_pos = ImGui::GetWindowPos();
	auto cursor_pos = ImGui::GetCursorPos();

	auto x = window_pos.x + cursor_pos.x;
	auto y = window_pos.y + cursor_pos.y;
	
	auto draw_list = ImGui::GetWindowDrawList();

	auto num_white_keys = int(avail / KEY_WIDTH);
	
	// Draw white keys
	for (int i = 0; i < num_white_keys; i++) {
		auto        octave = i / 7;
		auto within_octave = i % 7;

		auto note = BASE_NOTE + 12 * octave + note_offsets_white[within_octave];

		auto is_playing = std::find_if(notes.begin(), notes.end(), [n = note](auto note) {
			return note.note == n;	
		}) != notes.end();
		
		draw_list->AddRectFilled(
			ImVec2(x,                    y),
			ImVec2(x + KEY_WIDTH - 2.0f, y + KEY_HEIGHT), 
			is_playing ? colour_red : colour_white
		);

		x += KEY_WIDTH;
	}
	
	x = window_pos.x + cursor_pos.x + 0.5f * KEY_WIDTH;
	
	// Draw black keys
	// We do this separately so that the black keys appear on top of the white keys
	for (int i = 0; i < num_white_keys; i++) {
		auto        octave = i / 7;
		auto within_octave = i % 7;

		if (within_octave != 2 && within_octave != 6) { // E and B don't have a black note after them
			auto note = BASE_NOTE + 12 * octave + note_offsets_black[within_octave];

			auto is_playing = std::find_if(notes.begin(), notes.end(), [n = note](auto note) {
				return note.note == n;	
			}) != notes.end();
		
			draw_list->AddRectFilled(
				ImVec2(x,                     y),
				ImVec2(x + 0.75f * KEY_WIDTH, y + 0.75f * KEY_HEIGHT), 
				is_playing ? colour_red : colour_black
			);
		}
		
		x += KEY_WIDTH;
	}

	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
}

void PianoRollComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filename", filename);
}

void PianoRollComponent::deserialize_custom(json::Object const & object) {
	auto name = object.find_string("filename", DEFAULT_FILENAME);
	strcpy_s(filename, name.c_str());

	reload_file();
}

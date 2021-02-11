#include "components.h"

#include "synth.h"

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

		if (midi_time > t) break;

		auto time_offset = midi_time - time_start;

		if (midi_event.type == midi::Event::Type::PRESS) {
			synth.note_press(midi_event.note.note, midi_event.note.velocity / 128.0f, channel, time_offset);
		} else if (midi_event.type == midi::Event::Type::RELEASE) {
			synth.note_release(midi_event.note.note, channel, time_offset);
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
	auto channel_prev = channel;
	if (ImGui::DragInt("Channel Out", &channel, 0.1f, 0, Synth::NUM_CHANNELS - 1)) {
		// Release all notes to ensure no infinitely playing notes
		for (auto const & midi_event : midi.events) {
			synth.note_release(midi_event.note.note, channel_prev, 0);
		}
	}

	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) reload_file();
}

void PianoRollComponent::serialize(json::Writer & writer) const {
	writer.write("filename", filename);
	writer.write("channel",  channel);
}

void PianoRollComponent::deserialize(json::Object const & object) {
	auto name = object.find_string("filename", DEFAULT_FILENAME);
	strcpy_s(filename, name.c_str());

	channel = object.find_int("channel");

	reload_file();
}

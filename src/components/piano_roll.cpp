#include "components.h"

#include "synth.h"

void PianoRollComponent::update(Synth const & synth) {
	auto midi_ticks_to_samples = [](size_t time, size_t tempo, size_t ticks, size_t bpm) -> size_t {
		return SAMPLE_RATE * time * 60 / (bpm * ticks);
	};

	if (midi_length == 0) return;

	auto sixteenth_note = size_t(60) * SAMPLE_RATE / (4 * synth.settings.tempo);

	auto length = midi_ticks_to_samples(midi_length, midi.tempo, midi.ticks, synth.settings.tempo);

	auto a = (synth.time + BLOCK_SIZE - 1) / length;
	auto t = (synth.time + BLOCK_SIZE - 1) % length;

	auto first = midi_offset;

	while (true) {
		auto const & midi_event = midi.events[midi_offset];

		auto midi_time = midi_ticks_to_samples(midi_event.time, midi.tempo, midi.ticks, synth.settings.tempo);

		if (midi_time > t || midi_rounds >= a) break;

		if (midi_event.type == midi::Event::Type::PRESS) {
			synth.note_press(midi_event.note.note, 1.0f);
		} else if (midi_event.type == midi::Event::Type::RELEASE) {
			synth.note_release(midi_event.note.note);
		}

		midi_offset++;
		if (midi_offset == midi.events.size()) {
			midi_offset = 0;
			midi_rounds++;
		}

//		if (midi_offset == first) break;
	}
}

void PianoRollComponent::render(Synth const & synth) {

}

void PianoRollComponent::serialize(json::Writer & writer) const {
	writer.object_begin("PianoRollComponent");
	writer.write("id", id);
	writer.write("pos_x", pos[0]);
	writer.write("pos_y", pos[1]);
	writer.object_end();
}

void PianoRollComponent::deserialize(json::Object const & object) {

}

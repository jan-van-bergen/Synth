#include "connector.h"

#include "util/util.h"

Sample ConnectorIn::get_sample(int i) const {
	Sample sample = 0.0f;

	for (auto const & [other, weight] : others) {
		sample += weight * weight * reinterpret_cast<Sample *>(other->data)[i];
	}

	return sample;
}

std::vector<NoteEvent> ConnectorIn::get_events() const {
	std::vector<NoteEvent> result;

	for (auto const & [other, weight] : others) {
		auto events = other->get_events();

		for (auto const & event : events) {
			auto & e = result.emplace_back(event);
			e.velocity *= weight;
		}
	}

	return result;
}

void ConnectorOut::add_event(NoteEvent midi_event) {
	assert(is_midi);

	if (num_midi_events >= sizeof(data) / sizeof(NoteEvent)) return;

	reinterpret_cast<NoteEvent *>(data)[num_midi_events++] = midi_event;
}

std::span<NoteEvent const> ConnectorOut::get_events() const {
	assert(is_midi);

	return std::span<NoteEvent const>(reinterpret_cast<NoteEvent const *>(data),  num_midi_events);
}

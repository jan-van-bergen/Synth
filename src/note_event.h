
#pragma once

struct NoteEvent {
	bool pressed;
	int  time;

	int   note;
	float velocity;

	struct Compare {
		bool operator()(NoteEvent const & a, NoteEvent const & b) const {
			if (a.time == b.time) {
				if (a.note == b.note) {
					return a.pressed > b.pressed;
				} else {
					return a.note < b.note;
				}
			} else {
				return a.time < b.time;
			}
		}
	};
};

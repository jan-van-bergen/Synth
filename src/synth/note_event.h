#pragma once

struct Note {
	int   note;
	float velocity;

	bool operator<(Note const & other) const {
		return note < other.note;
	}
};
	
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

	static NoteEvent make_press(int time, int note, float velocity) {
		return { true, time, note, velocity };
	}

	static NoteEvent make_release(int time, int note) {
		return { false, time, note, 0.0f };
	}

private:
	NoteEvent(bool pressed, int time, int note, float velocity) : pressed(pressed), time(time), note(note), velocity(velocity) { }
};

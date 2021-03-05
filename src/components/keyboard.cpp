#include "keyboard.h"

#include "synth.h"

void KeyboardComponent::update(Synth const & synth) {
	for (auto const & note_event : synth.note_events) {
		outputs[0].add_event(note_event);
	}
}

void KeyboardComponent::render(Synth const & synth) {

}

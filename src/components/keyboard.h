#pragma once
#include "component.h"

struct KeyboardComponent : Component {
	KeyboardComponent(int id) : Component(id, "Keyboard", { }, { { this, "MIDI Out", true } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

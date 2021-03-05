#pragma once
#include "component.h"

struct DecibelComponent : Component {
	float decibels = -INFINITY;

	DecibelComponent(int id) : Component(id, "Decibel Meter", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	static constexpr auto HISTORY_LENGTH_IN_SECONDS = 1.5f;
	static constexpr auto HISTORY_LENGTH            = int(HISTORY_LENGTH_IN_SECONDS * SAMPLE_RATE / BLOCK_SIZE);

	std::vector<float> history;
	int                history_index = 0;

	float previous_height_factor = 0.0f;
};

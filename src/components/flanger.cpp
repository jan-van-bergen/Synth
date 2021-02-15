#include "components.h"

void FlangerComponent::update(Synth const & synth) {
	// Convert ms to seconds
	auto min_seconds = (delay)         * 0.001f;
	auto max_seconds = (delay + depth) * 0.001f;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);

		auto lfo_left  = util::remap(std::sin(TWO_PI * (lfo_phase)),         -1.0f, 1.0f, min_seconds, max_seconds);
		auto lfo_right = util::remap(std::sin(TWO_PI * (lfo_phase + phase)), -1.0f, 1.0f, min_seconds, max_seconds);

		lfo_phase += rate * SAMPLE_RATE_INV;

		auto index_left  = float(history_offset) - lfo_left  * SAMPLE_RATE;
		auto index_right = float(history_offset) - lfo_right * SAMPLE_RATE;

		auto delayed_left  = util::sample_linear(history_left,  HISTORY_SIZE, index_left);
		auto delayed_right = util::sample_linear(history_right, HISTORY_SIZE, index_right);

		auto delayed_sample = Sample(delayed_left, delayed_right);

		outputs[0].set_sample(i, util::lerp(sample, delayed_sample, drywet));
		
		auto fb = sample + delayed_sample * feedback;

		history_left [history_offset] = fb.left;
		history_right[history_offset] = fb.right;

		history_offset = util::wrap(history_offset + 1, HISTORY_SIZE);
	}
}

void FlangerComponent::render(Synth const & synth) {
	delay   .render();
	depth   .render();
	rate    .render();
	phase   .render();
	feedback.render();
	drywet  .render();
}

void FlangerComponent::serialize(json::Writer & writer) const {
	writer.write("depth",    depth);
	writer.write("rate",     rate);
	writer.write("feedback", feedback);
	writer.write("drywet",   drywet);
}

void FlangerComponent::deserialize(json::Object const & object) {
	depth    = object.find_float("depth",    depth   .default_value);
	rate     = object.find_float("rate",     rate    .default_value);
	feedback = object.find_float("feedback", feedback.default_value);
	drywet   = object.find_float("drywet",   drywet  .default_value);
}

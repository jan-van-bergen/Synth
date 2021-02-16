#include "components.h"

void PhaserComponent::AllPassFilter::set(float frequency, float Q) {
	auto omega = TWO_PI * frequency * SAMPLE_RATE_INV;

	auto sin_omega = std::sin(omega);
	auto cos_omega = std::cos(omega);

	auto alpha = sin_omega / (2.0f * Q);
		
	auto a0 = 1.0f + alpha;
	auto a1 = 2.0f * cos_omega;
	auto a2 = alpha - 1.0f;
	auto b0 = 1.0f - alpha;
	auto b1 = -2.0f * cos_omega;
	auto b2 = 1.0f + alpha;

	a = b0 / a0;
	b = b1 / a0;
	c = b2 / a0;
	d = a1 / a0;
	e = a2 / a0;
}

float PhaserComponent::AllPassFilter::process(float sample) {
	auto result = a*sample + b*x1 + c*x2 + d*y1 + e*y2;

	x2 = x1;
	x1 = sample;
	y2 = y1;
	y1 = result;

	return result;
}

void PhaserComponent::update(Synth const & synth) {
	static constexpr auto Q_FACTOR = 0.49f;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto dry = inputs[0].get_sample(i);

		// auto lfo_left  = util::remap<float>(std::sin(TWO_PI * (lfo_phase)),         -1.0f, 1.0f, min_depth, max_depth);
		// auto lfo_right = util::remap<float>(std::sin(TWO_PI * (lfo_phase + phase)), -1.0f, 1.0f, min_depth, max_depth);

		auto lfo_left  = util::log_interpolate<float>(min_depth, max_depth, 0.5f + 0.5f * std::sin(TWO_PI * (lfo_phase)));
		auto lfo_right = util::log_interpolate<float>(min_depth, max_depth, 0.5f + 0.5f * std::sin(TWO_PI * (lfo_phase + phase)));
		
		lfo_phase += rate * SAMPLE_RATE_INV;

		// Set up All Pass Filter
		all_pass_left .set(lfo_left,  Q_FACTOR);
		all_pass_right.set(lfo_right, Q_FACTOR);

		// Apply feedback
		auto sample = dry + feedback_sample * feedback;

		// Apply All Pass Filter iteratively
		for (int n = 0; n < num_stages; n++) {
			sample.left  = all_pass_left .process(sample.left);
			sample.right = all_pass_right.process(sample.right);
		}
		
		feedback_sample = sample;

		outputs[0].set_sample(i, util::lerp(dry, sample, drywet));
	}
}

void PhaserComponent::render(Synth const & synth) {
	static constexpr auto EPSILON = 0.0001f;

	rate.render();
	if (min_depth.render() && max_depth < min_depth) max_depth = min_depth + EPSILON; 
	if (max_depth.render() && min_depth > max_depth) min_depth = max_depth - EPSILON;
	phase     .render();
	num_stages.render();
	feedback  .render();
	drywet    .render();
}

void PhaserComponent::serialize(json::Writer & writer) const {
	writer.write("rate",       rate);
	writer.write("min_depth",  min_depth);
	writer.write("max_depth",  max_depth);
	writer.write("phase",      phase);
	writer.write("num_stages", num_stages);
	writer.write("feedback",   feedback);
	writer.write("drywet",     drywet);
}

void PhaserComponent::deserialize(json::Object const & object) {
	rate       = object.find_float("rate",       rate      .default_value);
	min_depth  = object.find_float("min_depth",  min_depth .default_value);
	max_depth  = object.find_float("max_depth",  max_depth .default_value);
	phase      = object.find_float("phase",      phase     .default_value);
	num_stages = object.find_int  ("num_stages", num_stages.default_value);
	feedback   = object.find_float("feedback",   feedback  .default_value);
	drywet     = object.find_float("drywet",     drywet    .default_value);
}

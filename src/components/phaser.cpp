#include "components.h"

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
		all_pass_left .set(dsp::BiQuadFilterMode::ALL_PASS, lfo_left,  Q_FACTOR);
		all_pass_right.set(dsp::BiQuadFilterMode::ALL_PASS, lfo_right, Q_FACTOR);

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

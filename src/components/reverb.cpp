#include "reverb.h"

void ReverbComponent::calc_comb_filter_delays() {
	// Constants from: https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
	int comb_filter_delays[NUM_COMB_FILTERS] = { 1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116 };
	static_assert(SAMPLE_RATE == 44100, "Sample rate of 44.1kHz is assumed here!");

	for (int i = 0; i < NUM_COMB_FILTERS; i++) {
		comb_filters_left [i].set_delay(room * (comb_filter_delays[i]));
		comb_filters_right[i].set_delay(room * (comb_filter_delays[i] + spread * MAX_SPREAD));

		comb_filters_left [i].damp = damp;
		comb_filters_right[i].damp = damp;
	}

	calc_comb_filter_feedbacks();

	int allpass_delays[NUM_ALLPASS_FILTERS] = { 225, 556, 441, 341 };

	for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
		allpass_filters_left [i].set_delay(room * (allpass_delays[i]));
		allpass_filters_right[i].set_delay(room * (allpass_delays[i] + spread * MAX_SPREAD));

		allpass_filters_left [i].feedback = 0.5f;
		allpass_filters_right[i].feedback = 0.5f;
	}
}

void ReverbComponent::calc_comb_filter_feedbacks() {
	for (int i = 0; i < NUM_COMB_FILTERS; i++) {
		auto delay_in_seconds_left  = comb_filters_left [i].history.size() * SAMPLE_RATE_INV;
		auto delay_in_seconds_right = comb_filters_right[i].history.size() * SAMPLE_RATE_INV;

		comb_filters_left [i].feedback = std::pow(10.0f, -3.0f * delay_in_seconds_left  / decay);
		comb_filters_right[i].feedback = std::pow(10.0f, -3.0f * delay_in_seconds_right / decay);
	}
}

void ReverbComponent::update(Synth const & synth) {
	auto linear_dry = util::db_to_linear(dry);
	auto linear_wet = util::db_to_linear(wet);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto input = inputs[0].get_sample(i);

		Sample sample = { };

		// Apply Comb Filters in parallel
		for (int f = 0; f < NUM_COMB_FILTERS; f++) {
			sample.left  += comb_filters_left [f].process(input.left);
			sample.right += comb_filters_right[f].process(input.right);
		}

		// Apply All Pass Filters in series
		for (int f = 0; f < NUM_ALLPASS_FILTERS; f++) {
			sample.left  = allpass_filters_left [f].process(sample.left);
			sample.right = allpass_filters_right[f].process(sample.right);
		}

		sample = linear_dry * input + linear_wet * sample;
		outputs[0].set_sample(i, sample);
	}
}

void ReverbComponent::render(Synth const & synth) {
	if (room.render()) {
		calc_comb_filter_delays();
	}
	ImGui::SameLine();

	if (decay.render()) {
		calc_comb_filter_feedbacks();
	}
	ImGui::SameLine();

	if (damp.render()) {
		for (int i = 0; i < NUM_COMB_FILTERS; i++) {
			comb_filters_left [i].damp = damp;
			comb_filters_right[i].damp = damp;
		}
	}

	spread.render(); ImGui::SameLine();
	dry   .render(); ImGui::SameLine();
	wet   .render();
}

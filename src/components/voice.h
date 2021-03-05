#pragma once
#include "component.h"

struct Voice {
	int   note;
	float velocity;
	
	int   start_time;              // In samples, absolute
	float release_time = INFINITY; // In steps, relative to start

	Voice(int note, float velocity, int start_time) : note(note), velocity(velocity), start_time(start_time) { }

	int get_first_sample(int time) const {
		if (time < start_time){
			return start_time - time;
		} else {
			return 0;
		}
	}

	bool apply_envelope(float time_in_steps, float a, float h, float d, float s, float r, float & amplitude) const {
		amplitude = velocity * util::envelope(time_in_steps, a, h, d, s);

		auto released = release_time < time_in_steps;

		if (released) {
			auto time_since_release = time_in_steps - release_time;

			if (time_since_release < r) {
				amplitude = util::lerp(amplitude, 0.0f, time_since_release / r);
			} else {
				return true; // Voice is done and should be removed
			}
		}

		return false;
	}

	bool apply_envelope(float time_in_steps, float & amplitude) const {
		return apply_envelope(time_in_steps, 0.1f, INFINITY, 0.0f, 1.0f, 0.1f, amplitude);
	}
};

template<typename TVoice> requires std::is_base_of_v<Voice, TVoice>
struct VoiceComponent : Component {
protected:
	std::vector<TVoice> voices;

	VoiceComponent(int id,
		std::string name,
		std::vector<ConnectorIn>  && inputs,
		std::vector<ConnectorOut> && outputs) : Component(id, name, std::move(inputs), std::move(outputs)) { }

	void update_voices(float steps_per_second) {		
		auto note_events = inputs[0].get_events();

		for (auto const & note_event : note_events) {
			if (note_event.pressed) {
				voices.emplace_back(note_event.note, note_event.velocity, note_event.time);
			} else {
				while (true) {
					auto voice = std::find_if(voices.begin(), voices.end(), [note = note_event.note](auto voice) {
						return voice.note == note && std::isinf(voice.release_time);
					});

					if (voice == voices.end()) break;
						
					voice->release_time = float(note_event.time - voice->start_time) * SAMPLE_RATE_INV * steps_per_second;
				}
			}
		}
	}

public:
	void clear() { voices.clear(); }
};

#include "speaker.h"

enum struct WavFmt : short {
	PCM        = 0x0001,
	IEEEFloat  = 0x0003,
	ALaw       = 0x0006,
	MULaw      = 0x0007,
	Extensible = 0xFFFE
};

struct WAVHeader {
	char     riff[4];
	unsigned chunk_size;
	char     wave[4];

	char     fmt[4];
	unsigned ext;
	WavFmt   signal_format;
	short    num_channels;
	unsigned sample_rate;
	unsigned byte_rate;
	short    block_size;
	short    bits_per_sample;

	char     chunk_data[4];
	unsigned chunk_data_size;
};

void SpeakerComponent::update(Synth const & synth) {
	auto start_offset = recorded_samples.size();

	if (recording) {
		recorded_samples.resize(start_offset + BLOCK_SIZE);
	}

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);

		if (recording) {
			recorded_samples[start_offset + i] = sample;
		}

		outputs[0].set_sample(i, sample);
	}
}

void SpeakerComponent::render(Synth const & synth) {
	if (ImGui::Button(recording ? "Pause" : "Record")) {
		recording = !recording;
	}
	
	if (recording) {
		ImGui::SameLine();
		ImGui::TextUnformatted("Recording...");
	} else if (recorded_samples.size() > 0) {
		ImGui::SameLine();
		ImGui::Text("%is", util::round(recorded_samples.size() * SAMPLE_RATE_INV));
	}

	if (ImGui::Button("Save")) {
		recording = false;

		char filename[32];
		auto num = 0;

		// Find unique file name
		while (true) {
			sprintf_s(filename, "samples/recording_%i.wav", num);

			if (!util::file_exists(filename)) break;

			num++;
		}

		WAVHeader header = { };
		memcpy(&header.riff,       "RIFF", 4);
		memcpy(&header.wave,       "WAVE", 4);
		memcpy(&header.fmt,        "fmt ", 4);
		memcpy(&header.chunk_data, "data", 4);

		auto data_size = recorded_samples.size() * sizeof(Sample);

		header.chunk_size = sizeof(WAVHeader) - 8 + data_size;
		header.ext = 16;

		header.signal_format = WavFmt::IEEEFloat;
		header.num_channels = 2;
		header.sample_rate = SAMPLE_RATE;
		header.byte_rate   = SAMPLE_RATE * sizeof(Sample);
		header.block_size      = sizeof(Sample);
		header.bits_per_sample = sizeof(Sample::left) * 8;

		header.chunk_data_size = data_size;
		
		FILE * file; fopen_s(&file, filename, "wb");

		if (file == nullptr) {
			printf("ERROR: Unable to save recording to file '%s'!\n", filename);
			return;
		}

		fwrite(&header, sizeof(header), 1, file);
		fwrite(recorded_samples.data(), sizeof(Sample), recorded_samples.size(), file);

		fclose(file);

		recorded_samples.clear();
	}
}

#include "components.h"

#include <filesystem>

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
	if (!recording) return;

	auto start = recorded_samples.size();
	recorded_samples.resize(start + BLOCK_SIZE);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		recorded_samples[start + i] = inputs[0].get_value(i);
	}
}

void SpeakerComponent::render(Synth const & synth) {
	if (ImGui::Button(recording ? "Pause" : "Record")) {
		recording = !recording;
	}
	
	if (recording) {
		ImGui::SameLine();
		ImGui::Text("Recording...");
	}

	if (ImGui::Button("Save")) {
		recording = false;

		char filename[32];
		auto num = 0;

		// Find unique file name
		while (true) {
			sprintf_s(filename, "samples/recording_%i.wav", num);

			if (!std::filesystem::exists(filename)) break;

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

void SpeakerComponent::serialize(json::Writer & writer) const {
	writer.object_begin("SpeakerComponent");
	writer.write("id", id);
	writer.write("pos_x", pos[0]);
	writer.write("pos_y", pos[1]);
	writer.object_end();
}

void SpeakerComponent::deserialize(json::Object const & object) {

}

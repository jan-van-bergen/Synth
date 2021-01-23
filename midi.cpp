#include "midi.h"

#include <cassert>

// Reference: https://faydoc.tripod.com/formats/mid.htm

MidiTrack MidiTrack::load(std::string const & filename) {
	FILE * file; fopen_s(&file, filename.c_str(), "rb");
	if (file == nullptr) abort();

	char file_header[14]; fread_s(file_header, sizeof(file_header), 1, sizeof(file_header), file);
	assert(memcmp(file_header, "MThd", 4) == 0);
	
	auto format     = file_header[8]  << 8 | file_header[9];
	auto num_tracks = file_header[10] << 8 | file_header[11];
	auto ticks      = file_header[12] << 8 | file_header[13];
	
	MidiTrack midi;

	auto time = 0;

	auto get_time = [&]() {
		return (float(time) * midi.tempo / ticks) / 1000000.0f;
	};
	
	for (int i = 0; i < num_tracks; i++) {
		char chunk_header[8]; fread_s(chunk_header, sizeof(chunk_header), 1, sizeof(chunk_header), file);
		assert(memcmp(chunk_header, "MTrk", 4) == 0);
	
		auto chunk_length = chunk_header[4] << 24 | chunk_header[5] << 16 | chunk_header[6] << 8 | chunk_header[7];
		
		auto bytes_parsed = 0;

		while (bytes_parsed < chunk_length) {
			auto delta  = 0;
			auto offset = 0;
			unsigned char byte;

			// Read RLE delta time, 7 bits at at time while the MSB is nonzero
			do {
				byte = getc(file);
				bytes_parsed++;

				delta = (byte & 0b1111111) | (delta << 7);
			} while (byte >> 7);

			time += delta;

			auto command = getc(file);
			assert(command >= 128); // Should have MSB set to 1

			bytes_parsed++;

			if (command == 0xff) { // Meta-events
				auto meta_cmd  = getc(file);
				auto num_bytes = getc(file);
			
				bytes_parsed += 2 + num_bytes;

				switch (meta_cmd) {
					case 0x2f: { // This event must come at the end of each track
						assert(num_bytes == 0);
						assert(bytes_parsed == chunk_length);

						break;
					}

					case 0x51: { // Set Tempo
						assert(num_bytes == 0x03);

						midi.tempo = getc(file) << 16 | getc(file) << 8 | getc(file);

						break;
					}

					default: {
						printf("WARNING: MIDI Meta Command 0x%02x is unsupported and will be ignored!\n", meta_cmd); 

						fseek(file, num_bytes, SEEK_CUR);

						break;
					}
				}
			} else {
				auto nib_command = (command & 0xf0) >> 4;
				auto nib_channel = (command & 0x0f);

				switch (nib_command) {
					case 0x8: {
						auto note     = getc(file);
						auto velocity = getc(file);

						bytes_parsed += 2;

						midi.events.push_back({ false, note, velocity, get_time() });
						
						break;
					}

					case 0x9: {
						auto note     = getc(file);
						auto velocity = getc(file);

						bytes_parsed += 2;

						midi.events.push_back({ true, note, velocity, get_time() });
						
						break;
					}

					default: abort();
				}
			}
		}
	}

	fclose(file);

	return midi;
}

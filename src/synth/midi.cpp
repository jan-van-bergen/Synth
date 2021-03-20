#include "midi.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmeapi.h>

#include <cassert>
#include <algorithm>

#include "util/ring_buffer.h"

//////////////////////////////////////////////////////////
// Reference: https://faydoc.tripod.com/formats/mid.htm //
//////////////////////////////////////////////////////////

#define CHECK_MM(result) check_mm(result, __LINE__, __FILE__);

void check_mm(MMRESULT result, int line, char const * file) {
	switch (result) {
		case MMSYSERR_NOERROR: return;

		case MMSYSERR_ERROR:        printf("MM ERROR at line %i of %s: MMSYSERR_ERROR",        line, file); break;
		case MMSYSERR_BADDEVICEID:  printf("MM ERROR at line %i of %s: MMSYSERR_BADDEVICEID",  line, file); break;
		case MMSYSERR_NOTENABLED:   printf("MM ERROR at line %i of %s: MMSYSERR_NOTENABLED",   line, file); break;
		case MMSYSERR_ALLOCATED:    printf("MM ERROR at line %i of %s: MMSYSERR_ALLOCATED",    line, file); break;
		case MMSYSERR_INVALHANDLE:  printf("MM ERROR at line %i of %s: MMSYSERR_INVALHANDLE",  line, file); break;
		case MMSYSERR_NODRIVER:     printf("MM ERROR at line %i of %s: MMSYSERR_NODRIVER",     line, file); break;
		case MMSYSERR_NOMEM:        printf("MM ERROR at line %i of %s: MMSYSERR_NOMEM",        line, file); break;
		case MMSYSERR_NOTSUPPORTED: printf("MM ERROR at line %i of %s: MMSYSERR_NOTSUPPORTED", line, file); break;
		case MMSYSERR_BADERRNUM:    printf("MM ERROR at line %i of %s: MMSYSERR_BADERRNUM",    line, file); break;
		case MMSYSERR_INVALFLAG:    printf("MM ERROR at line %i of %s: MMSYSERR_INVALFLAG",    line, file); break;
		case MMSYSERR_INVALPARAM:   printf("MM ERROR at line %i of %s: MMSYSERR_INVALPARAM",   line, file); break;
		case MMSYSERR_HANDLEBUSY:   printf("MM ERROR at line %i of %s: MMSYSERR_HANDLEBUSY",   line, file); break;
		case MMSYSERR_INVALIDALIAS: printf("MM ERROR at line %i of %s: MMSYSERR_INVALIDALIAS", line, file); break;
		case MMSYSERR_BADDB:        printf("MM ERROR at line %i of %s: MMSYSERR_BADDB",        line, file); break;
		case MMSYSERR_KEYNOTFOUND:  printf("MM ERROR at line %i of %s: MMSYSERR_KEYNOTFOUND",  line, file); break;
		case MMSYSERR_READERROR:    printf("MM ERROR at line %i of %s: MMSYSERR_READERROR",    line, file); break;
		case MMSYSERR_WRITEERROR:   printf("MM ERROR at line %i of %s: MMSYSERR_WRITEERROR",   line, file); break;
		case MMSYSERR_DELETEERROR:  printf("MM ERROR at line %i of %s: MMSYSERR_DELETEERROR",  line, file); break;
		case MMSYSERR_VALNOTFOUND:  printf("MM ERROR at line %i of %s: MMSYSERR_VALNOTFOUND",  line, file); break;
		case MMSYSERR_NODRIVERCB:   printf("MM ERROR at line %i of %s: MMSYSERR_NODRIVERCB",   line, file); break;
		case WAVERR_BADFORMAT:      printf("MM ERROR at line %i of %s: WAVERR_BADFORMAT",      line, file); break;
		case WAVERR_STILLPLAYING:   printf("MM ERROR at line %i of %s: WAVERR_STILLPLAYING",   line, file); break;
		case WAVERR_UNPREPARED:     printf("MM ERROR at line %i of %s: WAVERR_UNPREPARED",     line, file); break;
	}
	
	__debugbreak();
}

std::optional<midi::Track> midi::Track::load(char const * filename) {
	FILE * file; fopen_s(&file, filename, "rb");
	if (file == nullptr) return { };

	unsigned char file_header[14]; fread_s(file_header, sizeof(file_header), 1, sizeof(file_header), file);
	assert(memcmp(file_header, "MThd", 4) == 0);
	
	auto format     = file_header[8]  << 8 | file_header[9];
	auto num_tracks = file_header[10] << 8 | file_header[11];
	auto ticks      = file_header[12] << 8 | file_header[13];
	
	midi::Track midi;
	midi.ticks = ticks;

	int time;

	for (int i = 0; i < num_tracks; i++) {
		time = 0;

		unsigned char chunk_header[8]; fread_s(chunk_header, sizeof(chunk_header), 1, sizeof(chunk_header), file);
		assert(memcmp(chunk_header, "MTrk", 4) == 0);
	
		unsigned chunk_length = chunk_header[4] << 24 | chunk_header[5] << 16 | chunk_header[6] << 8 | chunk_header[7];
		
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

				static constexpr auto META_COMMAND_END   = 0x2F;
				static constexpr auto META_COMMAND_TEMPO = 0x51;

				switch (meta_cmd) {
					case META_COMMAND_END: { // This event must come at the end of each track
						assert(num_bytes == 0);
						assert(bytes_parsed == chunk_length);

						break;
					}

					case META_COMMAND_TEMPO: {
						assert(num_bytes == 0x03);

						midi.tempo = getc(file) << 16 | getc(file) << 8 | getc(file);

						break;
					}

					default: {
						printf("WARNING: MIDI Meta Command 0x%02x in file '%s' was ignored!\n", meta_cmd, filename); 

						fseek(file, num_bytes, SEEK_CUR);

						break;
					}
				}
			} else {
				auto nib_command = (command & 0xf0) >> 4;
				auto nib_channel = (command & 0x0f);

				static constexpr auto COMMAND_NOTE_OFF           = 0x8;
				static constexpr auto COMMAND_NOTE_ON            = 0x9;
				static constexpr auto COMMAND_NOTE_AFTERTOUCH    = 0xA;
				static constexpr auto COMMAND_CONTROL_CHANGE     = 0xB;
				static constexpr auto COMMAND_PRORAM_CHANGE      = 0xC;
				static constexpr auto COMMAND_CHANNEL_AFTERTOUCH = 0xD;
				static constexpr auto COMMAND_PITCH_BEND         = 0xE;

				switch (nib_command) {
					case COMMAND_NOTE_OFF: {
						auto note     = getc(file);
						auto velocity = getc(file);
						bytes_parsed += 2;

						midi.events.push_back(midi::Event::make_release(time, note, velocity));
						
						break;
					}

					case COMMAND_NOTE_ON: {
						auto note     = getc(file);
						auto velocity = getc(file);
						bytes_parsed += 2;

						midi.events.push_back(midi::Event::make_press(time, note, velocity));
						
						break;
					}

					case COMMAND_NOTE_AFTERTOUCH: {
						auto note     = getc(file);
						auto velocity = getc(file);
						bytes_parsed += 2;

						break;
					}

					case COMMAND_CONTROL_CHANGE: {
						auto control = getc(file);
						auto value   = getc(file);
						bytes_parsed += 2;

						break;
					}

					case COMMAND_PRORAM_CHANGE: {
						auto program = getc(file);
						bytes_parsed++;

						break;
					}

					case COMMAND_CHANNEL_AFTERTOUCH: {
						auto channel = getc(file);
						bytes_parsed++;

						break;
					}

					case COMMAND_PITCH_BEND: {
						auto pitch_bottom = getc(file);
						auto pitch_top    = getc(file);
						bytes_parsed += 2;

						break;
					}

					default: printf("WARNING: Unknown MIDI command %x!\n", nib_command);
				}
			}
		}
	}

	fclose(file);

	std::sort(midi.events.begin(), midi.events.end(), [](Event const & a, Event const & b) -> bool {
		if (a.time == b.time) {
			if (a.type == b.type) {
				if (a.type == Event::Type::CONTROL) {
					if (a.control.id == b.control.id) {
						return a.control.value < b.control.value;
					}

					return a.control.id < b.control.id;
				} else {
					if (a.note.note == b.note.note) {
						return a.note.velocity < b.note.velocity;
					}

					return a.note.note < b.note.note;
				}
			}

			return a.type < b.type;
			
		}
		return a.time < b.time;	
	});

	return midi;
}

static RingBuffer<midi::Event, 1024> buffer_events;

// Based on: http://midi.teragonaudio.com/tech/lowmidi.htm
void CALLBACK midi_callback(HMIDIIN handle, UINT msg, DWORD instance, DWORD param_1, DWORD param_2) {
	switch (msg) {
		case MIM_DATA: {
			auto nib_command = (param_1 & 0xf0) >> 4;
			auto nib_channel = (param_1 & 0x0f);

			if (nib_command == 0x9 || nib_command == 0x8) { // Press / Release
				int note     = (param_1 >> 8)  & 0x000000ff;
				int velocity = (param_1 >> 16) & 0x000000ff;

				auto event = nib_command == 9 ?
					midi::Event::make_press  (0, note, velocity) :
					midi::Event::make_release(0, note, velocity);

				buffer_events.get_write() = event;
				buffer_events.advance_write();
			} else if (nib_command == 0xB) { // Control changed value
				int control = (param_1 >> 8)  & 0x000000ff;
				int value   = (param_1 >> 16) & 0x000000ff;
				
				buffer_events.get_write() = midi::Event::make_control(0, control, value);
				buffer_events.advance_write();
			} else {
				printf("0x%08X 0x%02X 0x%02X 0x%02X\r\n", param_2, param_1 & 0x000000ff, (param_1 >> 8) & 0x000000ff, (param_1 >> 16) & 0x000000ff);
			}

			break;
		}

		default: break;
	}
}

static HMIDIIN midi_handle;
static MIDIHDR midi_hdr;

void midi::open() {
	auto midi_device_count = midiInGetNumDevs();

	if (midi_device_count == 0) return;

	for (int i = 0; i < midi_device_count; i++) {
		MIDIINCAPS midi_in_caps;
		if (!midiInGetDevCaps(i, &midi_in_caps, sizeof(MIDIINCAPS))) {
			printf("MIDI Device %i: %ls\r\n", i, midi_in_caps.szPname);
		}
	}
	
	CHECK_MM(midiInOpen(&midi_handle, 0, reinterpret_cast<DWORD_PTR>(midi_callback), 0, CALLBACK_FUNCTION));
	
	// Store pointer to input buffer for System Exclusive messages in MIDIHDR
	midi_hdr.lpData         = nullptr;
	midi_hdr.dwBufferLength = 0;
	
	// Prepare the buffer queue input buffer
	CHECK_MM(midiInPrepareHeader(midi_handle, &midi_hdr, sizeof(MIDIHDR)));
	CHECK_MM(midiInAddBuffer    (midi_handle, &midi_hdr, sizeof(MIDIHDR)));

	// Start recording MIDI
	CHECK_MM(midiInStart(midi_handle));
}

void midi::close() {
	if (midi_handle) {
		// Stop recording
		CHECK_MM(midiInReset(midi_handle));
		
		CHECK_MM(midiInUnprepareHeader(midi_handle, &midi_hdr, sizeof(MIDIHDR)));

		// Close MIDI In device
		while (midiInClose(midi_handle) == MIDIERR_STILLPLAYING) Sleep(0);
	}
}

std::optional<midi::Event> midi::get_event() {
	if (buffer_events.can_read()) {
		auto event = buffer_events.get_read();
		buffer_events.advance_read();

		return event;
	} else {
		return { };
	}
}

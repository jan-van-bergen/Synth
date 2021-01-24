#include "midi.h"

#include <Windows.h>

#include <cassert>

#include "ring_buffer.h"

// Reference: https://faydoc.tripod.com/formats/mid.htm

midi::Track midi::Track::load(std::string const & filename) {
	FILE * file; fopen_s(&file, filename.c_str(), "rb");
	if (file == nullptr) abort();

	char file_header[14]; fread_s(file_header, sizeof(file_header), 1, sizeof(file_header), file);
	assert(memcmp(file_header, "MThd", 4) == 0);
	
	auto format     = file_header[8]  << 8 | file_header[9];
	auto num_tracks = file_header[10] << 8 | file_header[11];
	auto ticks      = file_header[12] << 8 | file_header[13];
	
	midi::Track midi;

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

// Based on: http://midi.teragonaudio.com/tech/lowmidi.htm
static unsigned char SysXBuffer[256];
static unsigned char SysXFlag = 0;

static RingBuffer<midi::Event, 1024> buffer_events;

void CALLBACK midi_callback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	TCHAR			buffer[80];
	unsigned char 	bytes;

	/* Determine why Windows called me */
	switch (uMsg) {
		/* Received some regular MIDI message */
		case MIM_DATA: {
			/* Display the time stamp, and the bytes. (Note: I always display 3 bytes even for
			Midi messages that have less) */
			wsprintf(&buffer[0], L"0x%08X 0x%02X 0x%02X 0x%02X\r\n", dwParam2, dwParam1 & 0x000000FF, (dwParam1>>8) & 0x000000FF, (dwParam1>>16) & 0x000000FF);
			_cputws(&buffer[0]);

			auto nib_command = (dwParam1 & 0xf0) >> 4;
			auto nib_channel = (dwParam1 & 0x0f);

			auto pressed = nib_command == 9;

			int note     = (dwParam1 >> 8) & 0x000000ff;
			int velocity = (dwParam1 >> 8) & 0x000000ff;

			buffer_events.get_write() = { pressed, note, velocity };
			buffer_events.advance_write();

			break;
		}

		/* Received all or part of some System Exclusive message */
		case MIM_LONGDATA: {
			/* If this application is ready to close down, then don't midiInAddBuffer() again */
			if (!(SysXFlag & 0x80)) {
				/*	Assign address of MIDIHDR to a LPMIDIHDR variable. Makes it easier to access the
					field that contains the pointer to our block of MIDI events */
				auto lpMIDIHeader = (LPMIDIHDR)dwParam1;

				/* Get address of the MIDI event that caused this call */
				auto ptr = (unsigned char *)(lpMIDIHeader->lpData);

				/* Is this the first block of System Exclusive bytes? */
				if (!SysXFlag) {
					/* Print out a noticeable heading as well as the timestamp of the first block.
						(But note that other, subsequent blocks will have their own time stamps). */
					printf("*************** System Exclusive **************\r\n0x%08X ", dwParam2);

					/* Indicate we've begun handling a particular System Exclusive message */
					SysXFlag |= 0x01;
				}

				/* Is this the last block (ie, the end of System Exclusive byte is here in the buffer)? */
				if (*(ptr + (lpMIDIHeader->dwBytesRecorded - 1)) == 0xF7) {
					/* Indicate we're done handling this particular System Exclusive message */
					SysXFlag &= (~0x01);
				}

				/* Display the bytes -- 16 per line */
				bytes = 16;

				while((lpMIDIHeader->dwBytesRecorded--)) {
					if (!(--bytes)) {
						wsprintf(&buffer[0], L"0x%02X\r\n", *(ptr)++);
						bytes = 16;
					} else {
						wsprintf(&buffer[0], L"0x%02X ", *(ptr)++);
					}

					_cputws(&buffer[0]);
				}

				/* Was this the last block of System Exclusive bytes? */
				if (!SysXFlag) {
					/* Print out a noticeable ending */
					_cputws(L"\r\n******************************************\r\n");
				}

				/* Queue the MIDIHDR for more input */
				midiInAddBuffer(handle, lpMIDIHeader, sizeof(MIDIHDR));
			}

			break;
		}
	}
}

static HMIDIIN midi_handle;
static MIDIHDR midi_hdr;

#define CHECK_MM(result) check_mm(result, __LINE__, __FILE__);

void check_mm(MMRESULT result, int line, char const * file) {
	if (result) {
		printf("ERROR!");

		__debugbreak();
	}
}

void midi::open() {
	auto midi_device_count = midiInGetNumDevs();

	for (int i = 0; i < midi_device_count; i++) {
		MIDIINCAPS midi_in_caps;
		if (!midiInGetDevCaps(i, &midi_in_caps, sizeof(MIDIINCAPS))) {
			printf("MIDI Device %i: %ls\r\n", i, midi_in_caps.szPname);
		}
	}
	
	CHECK_MM(midiInOpen(&midi_handle, 0, reinterpret_cast<DWORD_PTR>(midi_callback), 0, CALLBACK_FUNCTION));
	
	// Store pointer to input buffer for System Exclusive messages in MIDIHDR
	midi_hdr.lpData         = (LPSTR)&SysXBuffer[0];
	midi_hdr.dwBufferLength = sizeof(SysXBuffer);
	
	// Prepare the buffer queue input buffer
	CHECK_MM(midiInPrepareHeader(midi_handle, &midi_hdr, sizeof(MIDIHDR)));
	CHECK_MM(midiInAddBuffer    (midi_handle, &midi_hdr, sizeof(MIDIHDR)));

	// Start recording MIDI
	CHECK_MM(midiInStart(midi_handle));
}

void midi::close() {
	/* We need to set a flag to tell our callback midiCallback()
		not to do any more midiInAddBuffer(), because when we
		call midiInReset() below, Windows will send a final
		MIM_LONGDATA message to that callback. If we were to
		allow midiCallback() to midiInAddBuffer() again, we'd
		never get the driver to finish with our midiHdr
	*/
	SysXFlag |= 0x80;
	
	// Stop recording
	midiInReset(midi_handle);

	/* Close the MIDI In device */
	while (midiInClose(midi_handle) == MIDIERR_STILLPLAYING) Sleep(0);

	midiInUnprepareHeader(midi_handle, &midi_hdr, sizeof(MIDIHDR));
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

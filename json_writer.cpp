#include "json.h"

#include <cstdlib>

json::Writer::Writer(char const * filename) {
	fopen_s(&file, filename, "wb");
	
	if (file == nullptr) abort();

	fputc('{', file);

	indent = 1;
	first_line = true;
}

json::Writer::~Writer() {
	fputc('\n', file);
	fputc('}',  file);

	fclose(file);
}

void json::Writer::new_line(bool comma) {
	if (comma) fputc(',', file);

	fputc('\n', file);

	for (int i = 0; i < indent; i++) fputc('\t', file);

	first_line = false;
}

void json::Writer::object_begin(char const * name) {
	if (name) write_key(name);
	
	fputc('{',  file);
	
	indent++;
	first_line = true;
}

void json::Writer::object_end() {
	indent--;
	new_line(false);

	fputc('}',  file);
}

void json::Writer::write_key(char const * name) {
	new_line(!first_line);

	fputc('"', file);
	fwrite(name,  strlen(name), sizeof(char), file);
	fputc('"', file);
	fputc(':', file);
	fputc(' ', file);
}

void json::Writer::write(char const * name, int length, float const data[]) {
	write_key(name);

	fputc('[', file);
	fputc(' ', file);

	for (int i = 0; i < length; i++) {
		fprintf_s(file, "%f", data[i]);

		if (i != length - 1) {
			fputc(',', file);
			fputc(' ', file);
		}
	}

	fputc(' ', file);
	fputc(']', file);
}

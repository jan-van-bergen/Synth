#include "json.h"

#include <cassert>

bool json::Object::find_array(char const * key, int length, float array[], float default_value) const {
	auto found = find<json::Array const>(key);
	int  found_length;

	if (found) {
		found_length = found->values.size();

		if (found_length != length) {
			printf("WARNING: The length of JSON array '%s' does not match expected value! Expected %i, got %i\n", key, length, found_length);

			if (found_length > length) found_length = length; // Don't read more than the array buffer allows for
		}
	} else {
		found_length = 0;
		
		printf("WARNING: Unable to find array '%s' in JSON!\n", key);
	}

	// Copy over howevery many elements were found in the JSON
	for (int i = 0; i < found_length; i++) {
		auto const & json_value = found->values[i];

		assert(json_value->type == json::JSON::Type::VALUE_FLOAT);
		auto value = static_cast<json::ValueFloat const *>(json_value.get());

		array[i] = value->value;
	}

	// Fill the remaining elements (if any) with the default value
	std::fill(array + found_length, array + length, default_value);

	return found_length == length;
}

int json::Object::find_int(char const * key, int default_value) const {
	auto found = find<json::ValueInt const>(key);
	if (!found) {
		printf("WARNING: Unable to find int '%s' in JSON!\n", key);
		return default_value;
	}

	return found->value;
}

float json::Object::find_float(char const * key, float default_value) const {
	auto found = find<json::ValueFloat const>(key);
	if (!found) {
		printf("WARNING: Unable to find float '%s' in JSON!\n", key);
		return default_value;
	}

	return found->value;
}

std::string json::Object::find_string(char const * key, std::string const & default_value) const {
	auto found = find<json::ValueString const>(key);
	if (!found) {
		printf("WARNING: Unable to find string '%s' in JSON!\n", key);
		return default_value;
	}

	return found->value;
}

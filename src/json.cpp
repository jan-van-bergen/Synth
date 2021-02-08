#include "json.h"

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

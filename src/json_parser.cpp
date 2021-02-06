#include "json.h"

#include <cctype>
#include <cstdlib>

#include <charconv>

#include "util.h"

template<int N>
static bool match(char const *& cur, char const (& target)[N]) {
	if (strncmp(cur, target, N - 1) != 0) return false;

	cur += N - 1;

	return true;
}

void skip_space(char const *& cur) {
	while (isspace(*cur)) cur++;
}

template<int N>
static void advance(char const *& cur, char const (& target)[N]) {
	if (!match(cur, target)) abort();
}

static int parse_int(char const *& cur, char const * end) {
	int result;
	auto [p, e] = std::from_chars(cur, end, result);

	cur = p;

	return result;
}

static float parse_float(char const *& cur, char const * end) {
	float result;
	auto [p, e] = std::from_chars(cur, end, result);

	cur = p;

	return result;
}

static std::unique_ptr<json::JSON> parse_json(char const *& cur) {
	skip_space(cur);

	std::string name = { };

	if (match(cur, "\"")) {
		auto start = cur;
		while (!match(cur, "\"")) cur++;

		name = std::string(start, cur - 1);

		advance(cur, ":");
		skip_space(cur);
	}

	if (match(cur, "{")) {
		// Parse Object
		std::vector<std::unique_ptr<json::JSON>> attributes;

		while (!match(cur, "}")) {
			attributes.push_back(parse_json(cur));

			match(cur, ",");
			skip_space(cur);
		}

		return std::make_unique<json::Object>(name, std::move(attributes));
	} else if (match(cur, "[")) {
		// Parse Array
		std::vector<std::unique_ptr<json::JSON>> values;

		while (!match(cur, "]")) {
			values.push_back(parse_json(cur));

			match(cur, ",");
			skip_space(cur);
		}

		return std::make_unique<json::Array>(name, std::move(values));
	}else if (match(cur, "\"")) {
		// Parse String
		auto start = cur;
		while (!match(cur, "\"")) cur++;

		return std::make_unique<json::ValueString>(name, std::string(start, cur - 1));
	} else {
		// Parse Float or Int
		auto start = cur;

		match(cur, "-");
		while (isdigit(*cur)) cur++;

		if (match(cur, ".")) {
			// Parse Float
			while (isdigit(*cur)) cur++;

			return std::make_unique<json::ValueFloat>(name, parse_float(start, cur));
		} else {
			// Parse Int
			return std::make_unique<json::ValueInt>(name, parse_int(start, cur));
		}
	}
}

json::Parser::Parser(char const * filename) {
	auto const input = util::read_file(filename);

	auto cur = input.data();
	auto end = input.data() + input.size();
	
	root = parse_json(cur);
}

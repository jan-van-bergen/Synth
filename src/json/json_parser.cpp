#include "json.h"

#include <cctype>
#include <cstdlib>

#include <charconv>

#include "util.h"

template<int N>
static bool match(char const *& cur, char const * end, char const (& target)[N]) {
	static constexpr auto LEN = N - 1;

	if (cur + LEN > end || strncmp(cur, target, LEN) != 0) return false;

	cur += LEN;

	return true;
}

template<int N>
static void advance(char const *& cur, char const * end, char const (& target)[N]) {
	if (!match(cur, end, target)) throw new std::exception("Unable to advance!");
}

void skip_space(char const *& cur, char const * end) {
	while (cur < end && isspace(*cur)) cur++;
}

template<typename T>
static T parse_num(char const *& cur, char const * end) {
	T result = { };
	auto [p, e] = std::from_chars(cur, end, result);

	cur = p;

	return result;
}

static std::unique_ptr<json::JSON> parse_json(char const *& cur, char const * end) {
	skip_space(cur, end);

	std::string name = { };

	if (match(cur, end, "\"")) {
		auto start = cur;
		while (!match(cur, end, "\"")) cur++;

		name = std::string(start, cur - 1);

		advance(cur, end, ":");
		skip_space(cur, end);
	}

	if (match(cur, end, "{")) {
		// Parse Object
		std::vector<std::unique_ptr<json::JSON>> attributes;

		while (!match(cur, end, "}")) {
			attributes.push_back(parse_json(cur, end));

			match(cur, end, ",");
			skip_space(cur, end);
		}

		return std::make_unique<json::Object>(name, std::move(attributes));
	} else if (match(cur, end, "[")) {
		// Parse Array
		std::vector<std::unique_ptr<json::JSON>> values;

		while (!match(cur, end, "]")) {
			values.push_back(parse_json(cur, end));

			match(cur, end, ",");
			skip_space(cur, end);
		}

		return std::make_unique<json::Array>(name, std::move(values));
	} else if (match(cur, end, "\"")) {
		// Parse String
		auto start = cur;
		while (!match(cur, end, "\"")) cur++;

		return std::make_unique<json::ValueString>(name, std::string(start, cur - 1));
	} else if (cur < end) {
		// Parse Float or Int
		auto start = cur;

		match(cur, end, "-");
		while (cur < end && isdigit(*cur)) cur++;

		if (match(cur, end, ".")) {
			// Parse Float
			while (cur < end && isdigit(*cur)) cur++;

			if (cur < end) {
				return std::make_unique<json::ValueFloat>(name, parse_num<float>(start, cur));
			}
		} else if (cur < end) {
			// Parse Int
			return std::make_unique<json::ValueInt>(name, parse_num<int>(start, cur));
		}
	}

	throw std::exception("Invalid JSON File!");
}

json::Parser::Parser(char const * filename) {
	auto const input = util::read_file(filename);

	auto cur = input.data();
	auto end = input.data() + input.size();
	
	try {
		root = parse_json(cur, end);
	} catch (std::exception const & ex) {
		printf("ERROR: Unable to load '%s':  %s\n", filename, ex.what());

		root = std::make_unique<json::Object>("Default", std::vector<std::unique_ptr<json::JSON>> { });
	}
}

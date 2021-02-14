#pragma once
#include <cstdio>
#include <cstring>
#include <varargs.h>

#include <string_view>
#include <vector>
#include <memory>

namespace json {
	struct JSON {
		enum struct Type {
			OBJECT,

			ARRAY,

			VALUE_INT,
			VALUE_FLOAT,
			VALUE_STRING
		} type;

		std::string name;

		JSON(Type type, std::string name) : type(type), name(std::move(name)) { }
		virtual ~JSON() = default;
		
		JSON            (JSON && other) = default;
		JSON & operator=(JSON && other) = default;

		JSON            (JSON const & other) = delete;
		JSON & operator=(JSON const & other) = delete;
	};

	struct Object : JSON {
		std::vector<std::unique_ptr<JSON>> attributes;

		Object(std::string name, std::vector<std::unique_ptr<JSON>> attributes) : JSON(JSON::Type::OBJECT, name), attributes(std::move(attributes)) { }
		
		template<typename T> requires std::derived_from<T, json::JSON>
		T * find(char const * key) const {
			auto found = std::find_if(attributes.begin(), attributes.end(), [key](auto const & attribute) {
				return attribute->name == key;
			});

			if (found == attributes.end()) {
				return nullptr;
			} else {
				return static_cast<T *>(found->get());
			}
		}

		int         find_int   (char const * key, int                 default_value = 0)    const;
		float       find_float (char const * key, float               default_value = 0.0f) const;
		std::string find_string(char const * key, std::string const & default_value = "")   const;
	};

	struct Array : JSON {
		std::vector<std::unique_ptr<JSON>> values;

		Array(std::string name, std::vector<std::unique_ptr<JSON>> values) : JSON(JSON::Type::ARRAY, name), values(std::move(values)) { }
	};
	
	struct ValueInt : JSON {
		int value;

		ValueInt(std::string name, int value) : JSON(JSON::Type::VALUE_INT, name), value(value) { }
	};

	struct ValueFloat : JSON {
		float value;

		ValueFloat(std::string name, float value) : JSON(JSON::Type::VALUE_FLOAT, name), value(value) { }
	};

	struct ValueString : JSON {
		std::string value;

		ValueString(std::string name, std::string value) : JSON(JSON::Type::VALUE_STRING, name), value(std::move(value)) { }
	};

	struct Parser {
		std::unique_ptr<JSON> root;

		Parser(char const * filename);
	};

	struct Writer {
	private:
		FILE * file;

		int  indent;
		bool first_line;

		void new_line(bool comma);

	public:
		Writer(char const * filename);
		~Writer();

		void object_begin(char const * name);
		void object_end();

		template<typename ... T>
		void writef(char const * name, char const * fmt, T const & ... args) {
			write_key(name);

			fprintf_s(file, fmt, args ...);
		}

		void write_key(char const * name);

		void write(char const * name, int   value)         { writef(name, "%i", value); }
		void write(char const * name, float value)         { writef(name, "%f", value); }
		void write(char const * name, char const * string) { writef(name, "\"%s\"", string); }

		void write(char const * name, int length, float const array[]);
	};
}

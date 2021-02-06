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
		~Object() = default;

		Object            (Object && other) = delete;
		Object & operator=(Object && other) = delete;

		Object            (Object const & other) = delete;
		Object & operator=(Object const & other) = delete;

		template<typename T> requires std::derived_from<T, json::JSON>
		T * find(char const * key) const {
			auto found = std::find_if(attributes.begin(), attributes.end(), [key](auto const & attribute) {
				return strcmp(attribute->name.c_str(), key) == 0;
			});

			if (found == attributes.end()) {
				return nullptr;
			} else {
				return static_cast<T *>(found->get());
			}
		}
	};

	struct Array : JSON {
		std::vector<std::unique_ptr<JSON>> values;

		Array(std::string name, std::vector<std::unique_ptr<JSON>> values) : JSON(JSON::Type::ARRAY, name), values(std::move(values)) { }
		~Array() = default;
		
		Array            (Array && other) = default;
		Array & operator=(Array && other) = default;

		Array            (Array const & other) = delete;
		Array & operator=(Array const & other) = delete;
	};
	
	struct ValueInt : JSON {
		int value;

		ValueInt(std::string name, int value) : JSON(JSON::Type::VALUE_INT, name), value(value) { }
		~ValueInt() = default;
		
		ValueInt            (ValueInt && other) = default;
		ValueInt & operator=(ValueInt && other) = default;

		ValueInt            (ValueInt const & other) = delete;
		ValueInt & operator=(ValueInt const & other) = delete;
	};

	struct ValueFloat : JSON {
		float value;

		ValueFloat(std::string name, float value) : JSON(JSON::Type::VALUE_FLOAT, name), value(value) { }
		~ValueFloat() = default;
		
		ValueFloat            (ValueFloat && other) = default;
		ValueFloat & operator=(ValueFloat && other) = default;

		ValueFloat            (ValueFloat const & other) = delete;
		ValueFloat & operator=(ValueFloat const & other) = delete;
	};

	struct ValueString : JSON {
		std::string value;

		ValueString(std::string name, std::string value) : JSON(JSON::Type::VALUE_STRING, name), value(std::move(value)) { }
		~ValueString() = default;
		
		ValueString            (ValueString && other) = default;
		ValueString & operator=(ValueString && other) = default;

		ValueString            (ValueString const & other) = delete;
		ValueString & operator=(ValueString const & other) = delete;
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

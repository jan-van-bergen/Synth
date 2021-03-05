#pragma once
#include <string>
#include <vector>

#include "parameter.h"
#include "connector.h"

struct Component {
	std::string name;

	std::vector<ConnectorIn>  inputs;
	std::vector<ConnectorOut> outputs;

	int const id;
	float pos [2] = { };
	float size[2] = { };

	Component(int id,
		std::string name,
		std::vector<ConnectorIn>  && inputs,
		std::vector<ConnectorOut> && outputs) : id(id), name(name), inputs(inputs), outputs(outputs) { }

	virtual void update(struct Synth const & synth) = 0;
	virtual void render(struct Synth const & synth) = 0;

	void serialize(json::Writer & writer) const {
		writer.write("id",     id);
		writer.write("pos_x",  pos[0]);
		writer.write("pos_y",  pos[1]);
		writer.write("size_x", size[0]);
		writer.write("size_y", size[1]);

		for (auto param : params) {
			param->serialize(writer);
		}

		serialize_custom(writer);
	}
	
	void deserialize(json::Object const & object) {
		pos [0] = object.find_float("pos_x", 100.0f);
		pos [1] = object.find_float("pos_y", 100.0f);
		size[0] = object.find_float("size_x");
		size[1] = object.find_float("size_y");

		for (auto param : params) {
			param->deserialize(object);
		}

		deserialize_custom(object);
	}

protected:
	friend Param;
	std::vector<Param *> params;
	
	virtual void   serialize_custom(json::Writer & writer) const { }
	virtual void deserialize_custom(json::Object const & object) { }
};

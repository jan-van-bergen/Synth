#pragma once
#include <string>
#include <vector>

#include <ImGui/imgui.h>

#include "sample.h"

struct Component;

struct Connector {
	const bool is_input;

	Component * component;

	std::string name;

	ImVec2 pos;

	std::vector<Connector *> connected;

	Connector(bool is_input, Component * component, std::string const & name) : is_input(is_input), component(component), name(name) { }
};

struct ConnectorIn : Connector {
	std::vector<std::pair<struct ConnectorOut *, float>> others; 
	
	ConnectorIn(Component * component, std::string const & name) : Connector(true, component, name) { }

	Sample get_value(int i) const;
};

struct ConnectorOut : Connector {
	std::vector<ConnectorIn *> others;
		
	Sample values[BLOCK_SIZE];

	ConnectorOut(Component * component, std::string const & name) : Connector(false, component, name) {
		clear();
	}

	void clear() { memset(values, 0, sizeof(values)); }
};

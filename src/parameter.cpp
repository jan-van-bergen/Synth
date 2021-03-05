#include "parameter.h"

#include "components/component.h"

Param::Param(Component * component, char const * serialization_name) : serialization_name(serialization_name){
	if (component) component->params.push_back(this);
}

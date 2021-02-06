#include "connector.h"

Sample ConnectorIn::get_value(int i) const {
	Sample sample = 0.0f;

	for (auto const & [other, weight] : others) sample += weight * other->values[i];
	
	return sample;
}

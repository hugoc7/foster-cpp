#pragma once

#define PRECISION 0.00001f //precision pour la comparaisons de float


#include <cmath>

bool equals(float a, float b) {
	return std::abs(a - b) < PRECISION;
}

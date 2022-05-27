#pragma once

#include <cmath>

namespace geo {

struct Coordinates {
	double lat;
	double lng;
};

double ComputeDistance(Coordinates from, Coordinates to);

}// namespace geo

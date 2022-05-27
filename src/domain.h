#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "geo.h"

namespace location {

struct Request {
	int id;
	std::string type;
	std::string name;
	std::string opt_str;
};

struct RouteData {
	std::string_view route_number;
	int stops_num;
	int unique_stops_num;
	double length;
	double curvature;
};

struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus {
	std::string route_number;
	bool is_circular;
	std::vector<const Stop*> route_stops;
};

} //namespace location

#include "transport_catalogue.h"

namespace location {

using namespace detail;
using namespace geo;

size_t StopsPairHasher::operator()(const std::pair<const Stop*, const Stop*>& target_pair) const {
	return hasher_(std::get<0>(target_pair)) + hasher_(std::get<1>(target_pair));
}

void TransportCatalogue::AddDistances(std::map<std::string_view, std::vector<std::pair<std::string_view, int>>>& raw_data) {
	for (auto& [countdown_name, distances_vector] : raw_data) {
		const Stop* countdown_ptr(FindStop(countdown_name));
		for (auto& [destination_name, distance] : distances_vector) {
			const Stop* destination_ptr(FindStop(destination_name));
			stops_distances_.insert({{countdown_ptr, destination_ptr}, distance});
		}
	}
}

void TransportCatalogue::AddRoute(std::string_view route_number, bool route_type, std::vector<std::string_view> stops_list) {
	std::vector<const Stop*> route_result;
	std::string route_number_str(route_number.begin(), route_number.end());
	for (auto item : stops_list) {
		const Stop* current_ptr = FindStop(item);
		route_result.push_back(current_ptr);
		available_routes_[current_ptr].insert(route_number_str);
	}
	buses_.push_back({move(route_number_str), route_type, route_result});
	buses_auxiliary_map_.insert({buses_.back().route_number, &buses_.back()});
}

void TransportCatalogue::AddStop(std::string_view name, Coordinates coordinates) {
	std::string name_str(name.begin(), name.end());
	stops_.push_back({move(name_str), coordinates});
	available_routes_.insert({&stops_.back(), {} });
	stops_auxiliary_map_.insert({stops_.back().name, &stops_.back()});
}

int TransportCatalogue::GetDistance(const Stop* ptr_from, const Stop* ptr_to) const {
	int result = 0;
		if (stops_distances_.find({ptr_from, ptr_to}) != stops_distances_.end()) {
			result = stops_distances_.at({ptr_from, ptr_to});
		} else {
			result = stops_distances_.at({ptr_to, ptr_from});
		}
	return result;
}

const Bus* TransportCatalogue::FindRoute(std::string_view request_number) {
	return buses_auxiliary_map_.count(request_number) ? buses_auxiliary_map_.at(request_number) : nullptr;
}

const Stop* TransportCatalogue::FindStop(std::string_view stop_name) {
	return stops_auxiliary_map_.count(stop_name) ? stops_auxiliary_map_.at(stop_name) : nullptr;
}

std::set<std::string>* TransportCatalogue::FindAvailableRoutes(std::string_view stop_name) {
	if (available_routes_.count(FindStop(stop_name))) {
		return &available_routes_.at(FindStop(stop_name));
	}
	return nullptr;
}

RouteData TransportCatalogue::GetRouteInformation(std::string_view request_number) {
	const Bus* selected_bus(FindRoute(request_number));
	double path_length_temp = 0.0;
	if (selected_bus) {
		int stops_on_route = 0;
		int unique_stops = 0;
		double curvature = 0;
		double straight_distance = 0;
		Coordinates* previous_position = nullptr;
		const Stop* temp_stop_ptr = nullptr;
		for (const auto Stop : selected_bus->route_stops) {
			if (previous_position) {
				straight_distance += ComputeDistance(*previous_position, Stop->coordinates);
			}
		previous_position = const_cast<Coordinates*>(&Stop->coordinates);
			if (temp_stop_ptr) {
				path_length_temp += GetDistance(temp_stop_ptr, Stop);
			}
			temp_stop_ptr = Stop;
		}
		std::set<const Stop*> temp_stops_ptr;
		for (auto item : selected_bus->route_stops) {
			temp_stops_ptr.insert(item);
		}
		unique_stops = temp_stops_ptr.size();
		if (!selected_bus->is_circular) {
			stops_on_route = (selected_bus->route_stops.size() * 2) - 1;
			straight_distance *= 2;
			temp_stop_ptr = nullptr;
			for (auto iter = selected_bus->route_stops.rbegin(); iter != selected_bus->route_stops.rend(); ++iter) {
				if (temp_stop_ptr) {
					path_length_temp += GetDistance(temp_stop_ptr, *iter);
				}
				temp_stop_ptr = *iter;
			}
			curvature = path_length_temp / straight_distance;
		} else {
			stops_on_route = selected_bus->route_stops.size();
			curvature = path_length_temp / straight_distance;
		}
		return {request_number, stops_on_route, unique_stops, path_length_temp, curvature};
	} else {
		return {};
	}
}

void TransportCatalogue::AddDeserializedStop(location::Stop& stop, std::set<std::string>& aviable_routes) {
	stops_.push_back(std::move(stop));
	if (!aviable_routes.empty()) {
		available_routes_.insert({&stops_.back(), std::move(aviable_routes)});
	}
	stops_auxiliary_map_.insert({stops_.back().name,  &stops_.back()});
}

void TransportCatalogue::AddDeserializedBus(location::Bus& bus) {
	buses_.push_back(std::move(bus));
	buses_auxiliary_map_.insert({buses_.back().route_number, &buses_.back()});

}

void TransportCatalogue::AddDeserializedDistance(const Stop* from, const Stop* to, int distance) {
	stops_distances_.insert({{from, to}, distance});
}


}// namespace location

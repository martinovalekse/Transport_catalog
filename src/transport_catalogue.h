#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>

#include "domain.h"

namespace location {
namespace detail {

class StopsPairHasher {
public:
	size_t operator()(const std::pair<const Stop*, const Stop*>& target_pair) const ;
private:
	std::hash<const void*> hasher_;
};

}// namespace detail

class TransportCatalogue {
public:
	void AddDistances(std::map<std::string_view, std::vector<std::pair<std::string_view, int>>>& raw_data);
	void AddStop(std::string_view name, geo::Coordinates coordinates);
	void AddRoute(std::string_view route_number, bool route_type, std::vector<std::string_view> stops_list);

	int GetDistance(const Stop* ptr_from, const Stop* ptr_to) const;
	const std::deque<Bus>& GetRoutes() const { return buses_; }
	const std::deque<Stop>& GetStops() const { return stops_; }
 	std::unordered_map<std::pair<const Stop*, const Stop*>, int, location::detail::StopsPairHasher>* GetDistances() {
 		return &stops_distances_;
 	}

	const Bus* FindRoute(std::string_view request_number);
	const Stop* FindStop(std::string_view stop_name);
	std::set<std::string>* FindAvailableRoutes(std::string_view stop_name);

	void AddDeserializedStop(location::Stop& stop, std::set<std::string>& aviable_routes);
	void AddDeserializedBus(location::Bus& bus);
	void AddDeserializedDistance(const Stop* from, const Stop* to, int distance);

	RouteData GetRouteInformation(std::string_view request_number);

private:
	std::deque<Stop> stops_;
	std::deque<Bus> buses_;
	std::unordered_map<const Stop*, std::set<std::string>> available_routes_;
	std::unordered_map<std::pair<const Stop*, const Stop*>, int, location::detail::StopsPairHasher> stops_distances_;
	std::unordered_map<std::string_view, const Stop*, std::hash<std::string_view>> stops_auxiliary_map_;
	std::unordered_map<std::string_view, const Bus*, std::hash<std::string_view>> buses_auxiliary_map_;
};

}// namespace location

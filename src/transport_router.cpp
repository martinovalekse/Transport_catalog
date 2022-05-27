#include "transport_router.h"

namespace graph {

void TransportRouter::SetupRouter(location::TransportCatalogue& transport_catalog, int velocity, int wait_time) {
	this->SetRoutingSettings(velocity, wait_time);
	this->PrepareGraphAndRouter(transport_catalog);
}

void TransportRouter::CalculateRoute(std::string_view from, std::string_view to, json::Builder& request_result) {
		using namespace std::literals;
		if (from == to) {
			request_result.Key("total_time"s).Value(0);
			request_result.Key("items"s).StartArray();
			request_result.EndArray();
		} else {
			auto thing = router_->BuildRoute(id_list_.stop_to_id.at(from), id_list_.stop_to_id.at(to));
			if (thing.has_value()) {
				request_result.Key("total_time"s).Value(thing.value().weight.time);
				request_result.Key("items"s).StartArray();
				for (auto& element : thing.value().edges) {
					auto some_ = graph_holder_->GetEdge(element);
					request_result.StartDict();
					std::string_view stop_strv = id_list_.id_to_stop.at(some_.from);
					std::string stop_namer(stop_strv.begin(), stop_strv.end());
					request_result.Key("stop_name"s).Value(stop_namer);
					request_result.Key("time"s).Value(settings.wait_time);
					request_result.Key("type"s).Value("Wait");
					request_result.EndDict();
					request_result.StartDict();
					std::string_view destination_strv = id_list_.id_to_stop.at(some_.to);
					std::string destination_name(destination_strv.begin(), destination_strv.end());
					std::string route_name(some_.weight.route_name.begin(), some_.weight.route_name.end());
					request_result.Key("bus"s).Value(route_name);
					request_result.Key("span_count"s).Value(some_.weight.span_count);
					request_result.Key("time"s).Value(some_.weight.time - settings.wait_time);
					request_result.Key("type"s).Value("Bus");
					request_result.EndDict();
				}
				request_result.EndArray();
			} else { request_result.Key("error_message"s).Value("not found"); }
		}
	}

//   -----------------------private-----------------------

void TransportRouter::BuildPaths(location::TransportCatalogue& transport_catalog) {
	for (const location::Bus& bus : transport_catalog.GetRoutes()) {
		for (size_t pos_from = 0; pos_from < bus.route_stops.size() - 1; ++pos_from) {
			double result_time = 0.0;
			int stop_count = 0;
			double reverse_result_time = 0.0;
			int reverse_stop_count = 0;
			size_t prev_reverse_start = (bus.route_stops.size() - 1) - pos_from;
			result_time += CalculateTime(transport_catalog, bus, pos_from, pos_from+1);
			++stop_count;
			for (size_t pos_to = pos_from + 1; pos_to < bus.route_stops.size(); ++pos_to) {
				edges_.push_back({id_list_.stop_to_id.at(bus.route_stops[pos_from]->name), id_list_.stop_to_id.at(bus.route_stops[pos_to]->name), {bus.route_number, stop_count, result_time + settings.wait_time}});
				if (pos_to != bus.route_stops.size() - 1) {
					result_time += CalculateTime(transport_catalog, bus, pos_to, pos_to+1);
					++stop_count;
				}
				if (!bus.is_circular) {
					int reverse_from = (bus.route_stops.size() - 1) - pos_from;
					int reverse_to = (bus.route_stops.size() - 1) - pos_to;
					reverse_result_time += CalculateTime(transport_catalog, bus, prev_reverse_start, reverse_to);
					++reverse_stop_count;
					prev_reverse_start = reverse_to;
					edges_.push_back({id_list_.stop_to_id.at(bus.route_stops[reverse_from]->name), id_list_.stop_to_id.at(bus.route_stops[reverse_to]->name), {bus.route_number, reverse_stop_count, reverse_result_time + settings.wait_time}});
				}
			}
		}
	}
}

inline double TransportRouter::CalculateTime(location::TransportCatalogue& transport_catalog, const location::Bus& bus, size_t from, size_t to) {
	double dist_km = (transport_catalog.GetDistance(bus.route_stops[from], bus.route_stops[to]) / 1000.0) ;
	return (dist_km / settings.velocity) * 60.0;
}

void TransportRouter::SetRoutingSettings(int velocity, int wait_time) {
	settings.velocity = velocity;
	settings.wait_time = wait_time;
}

void TransportRouter::PrepareGraphAndRouter(location::TransportCatalogue& transport_catalog) {
	int counter = 0;
	for (auto& item : transport_catalog.GetStops()) {;
		id_list_.stop_to_id.insert({item.name, counter});
		id_list_.id_to_stop.insert({counter++, item.name});
	}
	std::set<size_t> checked_id = {};
	BuildPaths(transport_catalog);
	graph_holder_ = std::make_unique<graph::DirectedWeightedGraph<EdgeData>>(id_list_.id_to_stop.size());
	for (auto item : edges_) {
		graph_holder_->AddEdge(item);
	}
	router_ = std::make_unique<graph::Router<EdgeData>>(*graph_holder_);
}

}// namespace graph

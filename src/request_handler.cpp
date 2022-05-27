#include "request_handler.h"

namespace location {
namespace input {

void RequestHandler::AddRequest(int id, std::string_view type, std::string_view name, std::string_view opt) {
	std::string type_str(type.begin(), type.end());
	std::string name_str(name.begin(), name.end());
	std::string opt_str(opt.begin(), opt.end());
	stat_requests_.push_back({id, type_str, name_str, opt_str});
}

json::Node RequestHandler::Result() const {
	using namespace std::literals;
	json::Builder request_result;
	request_result.StartArray();
	for (const Request& item : stat_requests_) {
		if (item.type == "Bus") {
			RouteData result = transport_catalog_.GetRouteInformation(item.name);
			request_result.StartDict();
			if (!result.route_number.empty()) {
				request_result.Key("curvature"s).Value(result.curvature);
				request_result.Key("request_id"s).Value(item.id);
				request_result.Key("route_length"s).Value(result.length);
				request_result.Key("stop_count"s).Value(result.stops_num);
				request_result.Key("unique_stop_count"s).Value(result.unique_stops_num);
			} else {
				request_result.Key("error_message"s).Value("not found"s);
				request_result.Key("request_id"s).Value(item.id);
			}
			request_result.EndDict();
		}

		if (item.type == "Stop") {
			const Stop* result = transport_catalog_.FindStop(item.name);
			request_result.StartDict();
			if (result) {
				request_result.Key("buses"s).StartArray();
				std::set<std::string>* routes = transport_catalog_.FindAvailableRoutes(item.name);
				if (routes != nullptr) { // откуда-то взялась вероятность получить тут пустоту при сериализации или десериализации
					for (std::string item : *routes) {
						request_result.Value(item);
					}
				}
				request_result.EndArray();
			} else {
				request_result.Key("error_message"s).Value("not found"s);
			}
			request_result.Key("request_id"s).Value(item.id);
			request_result.EndDict();
		}

		if (item.type == "Map") {
			request_result.StartDict();
			request_result.Key("map"s).Value(renderer_.RenderMap(transport_catalog_).AsString());
			request_result.Key("request_id"s).Value(item.id);
			request_result.EndDict();
		}
		if (item.type == "Route") {
			request_result.StartDict();
			request_result.Key("request_id"s).Value(item.id);
			transport_router_.CalculateRoute(item.name, item.opt_str, request_result);
			request_result.EndDict();
		}
	}
	request_result.EndArray();
	return request_result.Build();
}

void RequestHandler::AddSerializationFilename(std::string_view name) {
	std::string name_str(name.begin(), name.end());
	serialization_filename = std::move(name_str);
}

void RequestHandler::AddDeserializationFilename(std::string_view name) {
	std::string name_str(name.begin(), name.end());
	deserialization_filename = std::move(name_str);
}

void RequestHandler::Save() {
	std::ofstream fout(serialization_filename, std::ios::binary);
	Serialization(fout);
	fout.close();
}

void RequestHandler::Load() {
	std::fstream fin(deserialization_filename, std::ios::in);
	Deserialization(fin);
	fin.close();
}


//   -----------------------private-----------------------

void RequestHandler::Serialization(std::ostream& out_str) const {
	transport_catalogue_serialize::TransportCatalogue setialized_data;
	*setialized_data.mutable_catalog_data() = TransportCatalogSerialization();
	*setialized_data.mutable_routing_settings() = RoutingSettingsSerialization();
	*setialized_data.mutable_render_settings() = RenderSettingsSerialization();
	setialized_data.SerializeToOstream(&out_str);
}

void RequestHandler::Deserialization(std::istream& input_st) {
	transport_catalogue_serialize::TransportCatalogue setialized_data;
	setialized_data.ParseFromIstream(&input_st);
	DeserializationTransportCatalog(setialized_data.catalog_data());
	DeserializationRoutingSettings(setialized_data.routing_settings());
	DeserializationRenderSettings(setialized_data.render_settings());
}

transport_catalogue_serialize::StopsList RequestHandler::StopListSerialization(std::map<std::string, int>& stops_pointer) const {
	int num_pointer = 0;
	transport_catalogue_serialize::StopsList stop_list;
	for (auto& stop_item : transport_catalog_.GetStops()) {
		transport_catalogue_serialize::Coordinates coordinates;
		coordinates.set_lat(stop_item.coordinates.lat);
		coordinates.set_lng(stop_item.coordinates.lng);
		transport_catalogue_serialize::Stop stop;
		stop.set_name(stop_item.name);
		*stop.mutable_coordinates() = coordinates;
		stop.set_id(num_pointer);
		transport_catalogue_serialize::AvailableRoutes aviable_routes;
		if (!transport_catalog_.FindAvailableRoutes(stop_item.name)->empty()) {
			aviable_routes.set_id_form(num_pointer);
			for (auto& element : *transport_catalog_.FindAvailableRoutes(stop_item.name)) {
				aviable_routes.add_route(element);
			}
			*stop.mutable_aviable_routes() = aviable_routes;
		}
		*stop_list.add_stops() = stop;
		stops_pointer.insert({stop_item.name, num_pointer++});
	}
	return stop_list;
}

transport_catalogue_serialize::BusesList RequestHandler::BusesListSerialization(std::map<std::string, int>& stops_pointer) const {
	transport_catalogue_serialize::BusesList buses_list;
	for (auto& bus_item : transport_catalog_.GetRoutes()) {
		transport_catalogue_serialize::Bus bus;
		bus.set_route_number(bus_item.route_number);
		bus.set_is_circular(bus_item.is_circular);
		for (auto& stop_ptr : bus_item.route_stops) {
			bus.add_stop_id(stops_pointer.at(stop_ptr->name));
		}
		*buses_list.add_bus() = bus;
	}
	return buses_list;
}

transport_catalogue_serialize::DistancesList RequestHandler::DistancesListSerialization(std::map<std::string, int>& stops_pointer) const {
	transport_catalogue_serialize::DistancesList distances_list;
	for (auto& [stop_pair, distance] : *transport_catalog_.GetDistances()) {
		transport_catalogue_serialize::StopsDistance stop_distance;
		stop_distance.set_id_form(stops_pointer.at(stop_pair.first->name));
		stop_distance.set_id_to(stops_pointer.at(stop_pair.second->name));
		stop_distance.set_distance(distance);
		*distances_list.add_distances() = stop_distance;
	}
	return distances_list;
}

transport_catalogue_serialize::CatalogData RequestHandler::TransportCatalogSerialization() const {
		std::map<std::string, int> stops_pointer;
		transport_catalogue_serialize::CatalogData catalog_data;
		*catalog_data.mutable_stops_list() = StopListSerialization(stops_pointer);
		*catalog_data.mutable_buses_list() = BusesListSerialization(stops_pointer);
		*catalog_data.mutable_distances_list() = DistancesListSerialization(stops_pointer);
		return catalog_data;
	}

transport_catalogue_serialize::RoutingSettings RequestHandler::RoutingSettingsSerialization() const {
	transport_catalogue_serialize::RoutingSettings routing_settings;
	routing_settings.set_bus_velocity(transport_router_.GetSettings()->velocity);
	routing_settings.set_bus_wait_time(transport_router_.GetSettings()->wait_time);
	return routing_settings;
}

inline transport_catalogue_serialize::Color RequestHandler::ColorSerialization(svg::Color& input_color) const {
	transport_catalogue_serialize::Color output_color;
	output_color.set_monostate(false);
	if (std::holds_alternative<std::monostate>(input_color)) {
		output_color.set_monostate(true);
	} else if (std::holds_alternative<svg::Rgb>(input_color)) {
		transport_catalogue_serialize::RGB_A rgb_a;
		rgb_a.set_is_rgba(false);
		rgb_a.set_red(std::get<svg::Rgb>(input_color).red);
		rgb_a.set_green(std::get<svg::Rgb>(input_color).green);
		rgb_a.set_blue(std::get<svg::Rgb>(input_color).blue);
		*output_color.mutable_representation_rgb_a() = rgb_a;
	} else if (std::holds_alternative<svg::Rgba>(input_color)) {
		transport_catalogue_serialize::RGB_A rgb_a;
		rgb_a.set_is_rgba(true);
		rgb_a.set_red(std::get<svg::Rgba>(input_color).red);
		rgb_a.set_green(std::get<svg::Rgba>(input_color).green);
		rgb_a.set_blue(std::get<svg::Rgba>(input_color).blue);
		rgb_a.set_opacity(std::get<svg::Rgba>(input_color).opacity);
		*output_color.mutable_representation_rgb_a() = rgb_a;
	} else if (std::holds_alternative<std::string>(input_color)) {
		output_color.set_representation_string(std::get<std::string>(input_color));
	}
	return output_color;
}

transport_catalogue_serialize::RenderSettings RequestHandler::RenderSettingsSerialization() const {
	transport_catalogue_serialize::RenderSettings render_settings;
	const svg::output::RenderSettings* rs_current = renderer_.GetSettings();
	render_settings.set_width(rs_current->width);
	render_settings.set_height(rs_current->height);
	render_settings.set_padding(rs_current->padding);
	render_settings.set_line_width(rs_current->line_width);
	render_settings.set_stop_radius(rs_current->stop_radius);
	render_settings.set_bus_label_font_size(rs_current->bus_label_font_size);
	render_settings.add_bus_label_offset(rs_current->bus_label_offset.first);
	render_settings.add_bus_label_offset(rs_current->bus_label_offset.second);
	render_settings.set_stop_label_font_size(rs_current->stop_label_font_size);
	render_settings.add_stop_label_offset(rs_current->stop_label_offset.first);
	render_settings.add_stop_label_offset(rs_current->stop_label_offset.second);
	svg::Color input_color = rs_current->underlayer_color;
	*render_settings.mutable_underlayer_color() = ColorSerialization(input_color);
	render_settings.set_underlayer_width(rs_current->underlayer_width);
	for (auto color_elem : rs_current->color_palette) {
		*render_settings.add_color_palette() = ColorSerialization(color_elem);
	}
	return render_settings;
}

void RequestHandler::DeserializationStopsList(transport_catalogue_serialize::StopsList stop_list, std::map<int, std::string>& stops_pointer) {
	for (int i = 0; i < stop_list.stops_size(); ++i) {
		transport_catalogue_serialize::Stop stop_serialized = stop_list.stops(i);
		stops_pointer.insert({stop_serialized.id(), stop_serialized.name()});
		transport_catalogue_serialize::Coordinates coordinates_serialized = stop_serialized.coordinates();
		geo::Coordinates coordinates({coordinates_serialized.lat(), coordinates_serialized.lng()});
		location::Stop stop({stop_serialized.name(), coordinates});
		transport_catalogue_serialize::AvailableRoutes aviable_routes = stop_serialized.aviable_routes();
		std::set<std::string> aviable_routes_set;
		for (int i = 0; i < aviable_routes.route_size(); ++i) {
			aviable_routes_set.insert(std::move(aviable_routes.route(i)));
		}
		transport_catalog_.AddDeserializedStop(stop, aviable_routes_set);
	}
}

void RequestHandler::DeserializationBusesList(transport_catalogue_serialize::BusesList buses_list, std::map<int, std::string>& stops_pointer) {
	for (int i = 0; i <  buses_list.bus_size(); ++i) {
		transport_catalogue_serialize::Bus bus_serialized = buses_list.bus(i);
		std::vector<const Stop*> route_stops;
		route_stops.reserve(bus_serialized.stop_id_size() + 1);
		for (int i = 0; i < bus_serialized.stop_id_size(); ++i) {
			route_stops.push_back(transport_catalog_.FindStop(stops_pointer.at(bus_serialized.stop_id(i))));
		}
		location::Bus bus({bus_serialized.route_number(), bus_serialized.is_circular(), std::move(route_stops)});
		transport_catalog_.AddDeserializedBus(bus);
	}
}

void RequestHandler::DeserializationDistancesList(transport_catalogue_serialize::DistancesList distances_list, std::map<int, std::string>& stops_pointer) {
	for (int i = 0; i < distances_list.distances_size(); ++i) {
		transport_catalogue_serialize::StopsDistance stop_distance = distances_list.distances(i);
		const Stop* ptr_from(transport_catalog_.FindStop(stops_pointer.at(stop_distance.id_form())));
		const Stop* ptr_to(transport_catalog_.FindStop(stops_pointer.at(stop_distance.id_to())));
		transport_catalog_.AddDeserializedDistance(ptr_from, ptr_to, stop_distance.distance());
	}
}

void RequestHandler::DeserializationTransportCatalog(transport_catalogue_serialize::CatalogData catalog_data) {
	std::map<int, std::string> stops_pointer;
	DeserializationStopsList(catalog_data.stops_list(), stops_pointer);
	DeserializationBusesList(catalog_data.buses_list(), stops_pointer);
	DeserializationDistancesList(catalog_data.distances_list(), stops_pointer);
}

void RequestHandler::DeserializationRoutingSettings(transport_catalogue_serialize::RoutingSettings routing_settings) {
	transport_router_.SetupRouter(transport_catalog_, routing_settings.bus_velocity(), routing_settings.bus_wait_time());
}

inline svg::Color RequestHandler::DeserializationColor(transport_catalogue_serialize::Color color_serialized) {
	svg::Color color;
	if (color_serialized.monostate()) {
		return color;
	}
	if (color_serialized.has_representation_rgb_a()) {
		transport_catalogue_serialize::RGB_A rgb_a_serialized = color_serialized.representation_rgb_a();
		if (rgb_a_serialized.is_rgba()) {
			svg::Rgba rgba_result(rgb_a_serialized.red(), rgb_a_serialized.green(), rgb_a_serialized.blue(), rgb_a_serialized.opacity());
			color.emplace<svg::Rgba>(std::move(rgba_result));
		} else {
			svg::Rgb rgb_result(rgb_a_serialized.red(), rgb_a_serialized.green(), rgb_a_serialized.blue());
			color.emplace<svg::Rgb>(std::move(rgb_result));
		}
	} else {
		color.emplace<std::string>(color_serialized.representation_string());
	}
	return color;
}

void RequestHandler::DeserializationRenderSettings(transport_catalogue_serialize::RenderSettings render_settings_serialized) {
	svg::output::RenderSettings render_settings;
	render_settings.width = render_settings_serialized.width();
	render_settings.height = render_settings_serialized.height();
	render_settings.padding = render_settings_serialized.padding();
	render_settings.line_width = render_settings_serialized.line_width();
	render_settings.stop_radius = render_settings_serialized.stop_radius();
	render_settings.bus_label_font_size = render_settings_serialized.bus_label_font_size();
	render_settings.bus_label_offset = {render_settings_serialized.bus_label_offset(0), render_settings_serialized.bus_label_offset(1)};
	render_settings.stop_label_font_size = render_settings_serialized.stop_label_font_size();
	render_settings.stop_label_offset = {render_settings_serialized.stop_label_offset(0), render_settings_serialized.stop_label_offset(1)};
	render_settings.underlayer_color = DeserializationColor(render_settings_serialized.underlayer_color());
	render_settings.underlayer_width = render_settings_serialized.underlayer_width();
	std::vector<svg::Color> color_palette_deserialized;
	color_palette_deserialized.reserve(render_settings_serialized.color_palette_size() + 1);
	for (int i = 0; i < render_settings_serialized.color_palette_size(); ++i) {
		color_palette_deserialized.push_back(DeserializationColor(render_settings_serialized.color_palette(i)));
	}
	render_settings.color_palette = std::move(color_palette_deserialized);
	renderer_.InplacedSettings(render_settings);
}

}// namespace input
}// namespace location

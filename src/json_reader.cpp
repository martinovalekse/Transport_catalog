#include "json_reader.h"

namespace location {
namespace input {

using namespace std::literals;

void ParseRoute(TransportCatalogue& transport_catalog, const json::Node* bus_node) {
	std::vector<std::string_view> stops_list;
	if (!bus_node->AsDict().at("stops").AsArray().empty()) {
		stops_list.reserve( bus_node->AsDict().at("stops").AsArray().size() + 1);
		for (const json::Node& stop : bus_node->AsDict().at("stops").AsArray()) {
			stops_list.push_back(stop.AsString());
		}
	}
	transport_catalog.AddRoute(bus_node->AsDict().at("name").AsString(), bus_node->AsDict().at("is_roundtrip").AsBool(), move(stops_list));
}

StopData ParseStop(TransportCatalogue& transport_catalog, const json::Node* stop_node) {
	StopData distances_reserved;
	double latitude = stop_node->AsDict().at("latitude").AsDouble();
	if (latitude < 0) {
		latitude *= -1;
	}
	double longitude = stop_node->AsDict().at("longitude").AsDouble();
	if (longitude < 0) {
		longitude *= -1;
	}
	transport_catalog.AddStop(stop_node->AsDict().at("name").AsString(), {latitude, longitude});
	if (!stop_node->AsDict().at("road_distances").AsDict().empty()) {
		std::vector<std::pair<std::string_view, int>> distance_temp;
		distance_temp.reserve(stop_node->AsDict().at("road_distances").AsDict().size() + 1);
		for (auto& [key, value] : stop_node->AsDict().at("road_distances").AsDict()) {
			std::string_view str_v = key;
			distance_temp.push_back(std::make_pair(str_v, value.AsInt()));
		}
		distances_reserved.insert({stop_node->AsDict().at("name").AsString(), distance_temp});
	}
	return distances_reserved;
}

std::variant<std::string, std::vector<double>>  DiscernColor(const json::Node* color_node) {
	std::variant<std::string, std::vector<double>> result;
	if (color_node->IsString()) {
		result.emplace<std::string>(color_node->AsString());
	} else if (color_node->IsArray()) {
		std::vector<double> color;
		color.reserve(color_node->AsArray().size());
		color.push_back(color_node->AsArray()[0].AsInt());
		color.push_back(color_node->AsArray()[1].AsInt());
		color.push_back(color_node->AsArray()[2].AsInt());
		if (color_node->AsArray().size() == 4) {
			color.push_back(color_node->AsArray()[3].AsDouble());
		}
		result.emplace<std::vector<double>>(color);
	}
	return result;
}

void ParseMap(svg::output::MapRenderer& render, const json::Node* settings_node) {
	svg::output::RenderSettings settings_;
	settings_.width = settings_node->AsDict().at("width").AsDouble();
	settings_.height = settings_node->AsDict().at("height").AsDouble();
	settings_.padding = settings_node->AsDict().at("padding").AsDouble();
	settings_.line_width = settings_node->AsDict().at("line_width").AsDouble();
	settings_.stop_radius = settings_node->AsDict().at("stop_radius").AsDouble();
	settings_.bus_label_font_size = settings_node->AsDict().at("bus_label_font_size").AsInt();
	settings_.bus_label_offset = {settings_node->AsDict().at("bus_label_offset").AsArray()[0].AsDouble(),
								  settings_node->AsDict().at("bus_label_offset").AsArray()[1].AsDouble()};
	settings_.stop_label_font_size = settings_node->AsDict().at("stop_label_font_size").AsInt();
	settings_.stop_label_offset = {settings_node->AsDict().at("stop_label_offset").AsArray()[0].AsDouble(),
								   settings_node->AsDict().at("stop_label_offset").AsArray()[1].AsDouble()};
	auto color_raw = DiscernColor(&settings_node->AsDict().at("underlayer_color"));
	settings_.underlayer_color = svg::output::MakeColor(color_raw);
	settings_.underlayer_width = settings_node->AsDict().at("underlayer_width").AsDouble();
	std::vector<std::variant<std::string, std::vector<double>>> color_palette;
	color_palette.reserve(settings_node->AsDict().at("color_palette").AsArray().size() + 1);
	for (auto elem : settings_node->AsDict().at("color_palette").AsArray()) {
		color_palette.push_back(DiscernColor(&elem));
	}
	settings_.color_palette.reserve(color_palette.size() + 1);
	for (auto elem : color_palette) {
		settings_.color_palette.push_back(svg::output::MakeColor(elem));
	}
	render.InplacedSettings(settings_);
}


void FillData(TransportCatalogue& transport_catalog, svg::output::MapRenderer& render, RequestHandler& handler, std::istream& input) {
	json::Document doc = json::Load(input);
	if (doc.GetRoot().IsDict()) {
		std::map<std::string_view, std::vector<std::pair<std::string_view, int>>> distances_reserved;
		if (!doc.GetRoot().AsDict().at("serialization_settings").AsDict().empty()) {
			handler.AddSerializationFilename(doc.GetRoot().AsDict().at("serialization_settings").AsDict().at("file").AsString());
		}
		if (!doc.GetRoot().AsDict().at("base_requests").AsArray().empty()) {
			std::vector<const json::Node*> Buses_list;
			for (auto& item : doc.GetRoot().AsDict().at("base_requests").AsArray()) {
				if (item.AsDict().at("type").AsString() == "Stop") {
					distances_reserved.merge(ParseStop(transport_catalog, &item));
				}
				if (item.AsDict().at("type").AsString() == "Bus") {
					Buses_list.push_back(&item);
				}
			}
			if (!Buses_list.empty()) {
				for (auto& node : Buses_list) {
					ParseRoute(transport_catalog, node);
				}
			}
		}
		if (!distances_reserved.empty()) {
			transport_catalog.AddDistances(distances_reserved);
		}
		if (!doc.GetRoot().AsDict().at("render_settings").AsDict().empty()) {
			ParseMap(render, &doc.GetRoot().AsDict().at("render_settings"));
		}
		if (!doc.GetRoot().AsDict().at("routing_settings").AsDict().empty()) {
			handler.GetTransportRouter().SetupRouter(
					transport_catalog,
					doc.GetRoot().AsDict().at("routing_settings").AsDict().at("bus_velocity").AsInt(),
					doc.GetRoot().AsDict().at("routing_settings").AsDict().at("bus_wait_time").AsInt()
					);
		}
	} else {
		throw std::invalid_argument("Invalid input struct");
	}
}

void FillRequestsData(TransportCatalogue& transport_catalog, svg::output::MapRenderer& render, RequestHandler& handler, std::istream& input) {
	json::Document doc = json::Load(input);
	if (doc.GetRoot().IsDict()) {
		if (!doc.GetRoot().AsDict().at("serialization_settings").AsDict().empty()) {
			handler.AddDeserializationFilename(doc.GetRoot().AsDict().at("serialization_settings").AsDict().at("file").AsString());
		}
		if (!doc.GetRoot().AsDict().at("stat_requests").AsArray().empty()) {
			for (auto& item : doc.GetRoot().AsDict().at("stat_requests").AsArray()) {
				if (item.AsDict().at("type").AsString() == "Map") {
					handler.AddRequest(item.AsDict().at("id").AsInt(), item.AsDict().at("type").AsString(), {}, {});
				} else if (item.AsDict().at("type").AsString() == "Route") {
					handler.AddRequest(item.AsDict().at("id").AsInt(), item.AsDict().at("type").AsString(), item.AsDict().at("from").AsString(), item.AsDict().at("to").AsString());
				} else {
					handler.AddRequest(item.AsDict().at("id").AsInt(), item.AsDict().at("type").AsString(), item.AsDict().at("name").AsString(), {});
				}
			}
		}
	} else {
		throw std::invalid_argument("Invalid input struct");
	}

}

}// namespace input
}// namespace location


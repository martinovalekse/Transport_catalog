#include "map_renderer.h"

namespace svg {
namespace output {

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
	return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_};
}

bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

svg::Color MakeColor(RawColor& raw_color) {
	svg::Color result;
	using namespace std::literals;
	if (std::holds_alternative<std::string>(raw_color)) {
		 result.emplace<std::string>(std::get<std::string>(raw_color));
	} else if (std::holds_alternative<std::vector<double>>(raw_color)) {
		if (std::get<std::vector<double>>(raw_color).size() == 3) {
			svg::Rgb temp(static_cast<int>(std::get<std::vector<double>>(raw_color)[0]),
						  static_cast<int>(std::get<std::vector<double>>(raw_color)[1]),
					 	  static_cast<int>(std::get<std::vector<double>>(raw_color)[2]));
			result.emplace<svg::Rgb>(temp);
		} else {
			svg::Rgba temp(static_cast<int>(std::get<std::vector<double>>(raw_color)[0]),
						   static_cast<int>(std::get<std::vector<double>>(raw_color)[1]),
						   static_cast<int>(std::get<std::vector<double>>(raw_color)[2]),
						   std::get<std::vector<double>>(raw_color)[3]);
			result.emplace<svg::Rgba>(temp);
		}
	}
	return result;
}

//--------------------------------- MapRenderer ---------------------------------

void MapRenderer::InplacedSettings(RenderSettings& settings) {
		settings_ = settings;
	}

void MapRenderer::AddText(svg::Document& doc, svg::Point position, svg::Color color, std::string data) const {
	Text bus_name;
	Text bus_name_underlayer;
	bus_name.SetPosition(position);
	bus_name.SetFillColor(color);
	bus_name.SetOffset({std::get<0>(settings_.bus_label_offset), std::get<1>(settings_.bus_label_offset)});
	bus_name.SetFontSize(settings_.bus_label_font_size);
	bus_name.SetFontFamily("Verdana");
	bus_name.SetFontWeight("bold");
	bus_name.SetData(data);
	//--
	bus_name_underlayer = bus_name;
	bus_name_underlayer.SetFillColor(settings_.underlayer_color);
	bus_name_underlayer.SetStrokeColor(settings_.underlayer_color);
	bus_name_underlayer.SetStrokeWidth(settings_.underlayer_width);
	bus_name_underlayer.SetStrokeLineCap(StrokeLineCap::ROUND);
	bus_name_underlayer.SetStrokeLineJoin(StrokeLineJoin::ROUND);
	doc.Add(bus_name_underlayer);
	doc.Add(bus_name);
}

void MapRenderer::AddLines(SphereProjector& converter, svg::Document& result_doc, const location::Bus& route,  size_t colour_selection) const {
	Polyline route_line;
	for (const location::Stop* item :  route.route_stops) {
		route_line.AddPoint(converter(item->coordinates));
	}
	if (!route.is_circular) {
		for (auto iter = (route.route_stops.end() - 2); iter > route.route_stops.begin() - 1; --iter) {
			const location::Stop* item = *iter;
			route_line.AddPoint(converter(item->coordinates));
		}
	}
	route_line.SetFillColor("none");
	route_line.SetStrokeColor(settings_.color_palette[colour_selection]);
	route_line.SetStrokeWidth(settings_.line_width);
	route_line.SetStrokeLineCap(StrokeLineCap::ROUND);
	route_line.SetStrokeLineJoin(StrokeLineJoin::ROUND);
	result_doc.Add(route_line);
}

void MapRenderer::AddStopMarkers(SphereProjector& converter, svg::Document& result_doc, std::vector<const location::Stop*>& stop_list) const { //  uniq_stops_vect;
		for (const location::Stop* item :  stop_list) {
			Circle stop_marker;
			stop_marker.SetCenter(converter(item->coordinates));
			stop_marker.SetRadius(settings_.stop_radius);
			stop_marker.SetFillColor("white");
			result_doc.Add(stop_marker);
		}
}

void MapRenderer::AddStopName(svg::Document& doc, svg::Point position, std::string name) const {
	Text stop_name;
	Text stop_name_underlayer;
	stop_name.SetPosition(position);
	stop_name.SetFillColor("black");
	stop_name.SetOffset({std::get<0>(settings_.stop_label_offset), std::get<1>(settings_.stop_label_offset)});
	stop_name.SetFontSize(settings_.stop_label_font_size);
	stop_name.SetFontFamily("Verdana");
	stop_name.SetData(name);
	//--
	stop_name_underlayer = stop_name;
	stop_name_underlayer.SetFillColor(settings_.underlayer_color);
	stop_name_underlayer.SetStrokeColor(settings_.underlayer_color);
	stop_name_underlayer.SetStrokeWidth(settings_.underlayer_width);
	stop_name_underlayer.SetStrokeLineCap(StrokeLineCap::ROUND);
	stop_name_underlayer.SetStrokeLineJoin(StrokeLineJoin::ROUND);
	doc.Add(stop_name_underlayer);
	doc.Add(stop_name);
}

void MapRenderer::CreateMap(Document& result_doc, std::deque<location::Bus>& buses,  SphereProjector& converter,  std::vector<const location::Stop*>& uniq_stops_vect) const {
	size_t colour_selection = 0;
	std::vector<size_t> color_reminder;
	for (const location::Bus& route : buses) {  //make route lines
		if (route.route_stops.size() != 0) {
		if (colour_selection == settings_.color_palette.size()) {
			colour_selection = 0;
		}
		color_reminder.push_back(colour_selection);
		AddLines(converter, result_doc, route, colour_selection);
		++colour_selection;
		}
	}
	colour_selection = 0;
	for (const location::Bus& route : buses) { //make bus names
		if (route.route_stops.size() != 0) {
			if (route.is_circular) {
				AddText(result_doc, converter((*route.route_stops.begin())->coordinates), settings_.color_palette[color_reminder[colour_selection]], route.route_number);
			} else {
				if ((*(route.route_stops.begin()))->name != (*(route.route_stops.end() - 1))->name) {
					AddText(result_doc, converter((*route.route_stops.begin())->coordinates), settings_.color_palette[color_reminder[colour_selection]], route.route_number);
					AddText(result_doc, converter((*(route.route_stops.end() - 1))->coordinates), settings_.color_palette[color_reminder[colour_selection]], route.route_number);
				} else {
					AddText(result_doc, converter((*route.route_stops.begin())->coordinates), settings_.color_palette[color_reminder[colour_selection]], route.route_number);
				}
			}
			++colour_selection;
		}
	}
	if (!uniq_stops_vect.empty()) { //make stop markers
		AddStopMarkers(converter, result_doc, uniq_stops_vect);
	}
	if (!uniq_stops_vect.empty()) { //make stop names
		for (const location::Stop* item : uniq_stops_vect) {
			AddStopName(result_doc, converter(item->coordinates), item->name);
		}
	}
}

json::Node MapRenderer::RenderMap(location::TransportCatalogue& transport_catalog) const {
	//--------------------------------------------- make sort buses names
	auto buses = transport_catalog.GetRoutes();
	std::sort(buses.begin(), buses.end(), [](const location::Bus& route_a, const location::Bus& route_b) {
		return std::lexicographical_compare(route_a.route_number.begin(), route_a.route_number.end(), route_b.route_number.begin(), route_b.route_number.end()) ;
		});
	//--------------------------------------------- make uniq stops list of all routes, and sort it
	std::set<const location::Stop*>  uniq_stops;
	for (const location::Bus& route : buses) {
		for (auto item : route.route_stops) {
			uniq_stops.insert(item);
		}
	}
	std::vector<const location::Stop*> uniq_stops_vect;
	for (auto item : uniq_stops) {
		uniq_stops_vect.push_back(item);
	}
	std::sort(uniq_stops_vect.begin(), uniq_stops_vect.end(), [](const location::Stop* stop_a, const location::Stop* stop_b) {
			return std::lexicographical_compare(stop_a->name.begin(), stop_a->name.end(), stop_b->name.begin(), stop_b->name.end()) ;
			});
	//--------------------------------------------- converter from Coordinates to Point
	std::vector<geo::Coordinates> collect_coordinates;
	collect_coordinates.reserve(transport_catalog.GetStops().size() + 1);
	for (const  location::Stop* stop : uniq_stops_vect) {
		collect_coordinates.push_back(stop->coordinates);
	}
	SphereProjector converter(collect_coordinates.begin(), collect_coordinates.end(), settings_.width, settings_.height, settings_.padding);
	//--------------------------------------------- make parts
	Document result_map;
	CreateMap(result_map, buses, converter, uniq_stops_vect);
	//--------------------------------------------- output(
	std::ostringstream out;
	result_map.Render(out);
	return {out.str()};
}

}// namespace output
}// namespace svg

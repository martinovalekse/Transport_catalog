#pragma once

#include "json.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <vector>

namespace svg {
namespace output {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
	template <typename PointInputIt>
	SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
					double max_height, double padding);

	svg::Point operator()(geo::Coordinates coords) const;

private:
	double padding_;
	double min_lon_ = 0;
	double max_lat_ = 0;
	double zoom_coeff_ = 0;
};

using RawColor = std::variant<std::string, std::vector<double>>;

struct RenderSettings {
	double width;
	double height;
	double padding;
	double line_width;
	double stop_radius;
	int bus_label_font_size;
	std::pair<double, double> bus_label_offset;
	int stop_label_font_size;
	std::pair<double, double> stop_label_offset;
	svg::Color underlayer_color;
	double underlayer_width;
	std::vector<svg::Color> color_palette;
};

svg::Color MakeColor(RawColor& raw_color);


class MapRenderer {
public:
	MapRenderer() = default;

	void InplacedSettings(RenderSettings& settings);

	const RenderSettings* GetSettings() const {
		return &settings_;
	}

	void AddText(svg::Document& doc, svg::Point position, svg::Color color, std::string data) const;
	void AddLines(SphereProjector& converter, svg::Document& result_doc, const location::Bus& route,  size_t colour_selection) const;
	void AddStopMarkers(SphereProjector& converter, svg::Document& result_doc, std::vector<const location::Stop*>& stop_list) const;
	void AddStopName(svg::Document& doc, svg::Point position, std::string name) const;

	void CreateMap(Document& result_doc, std::deque<location::Bus>& buses,  SphereProjector& converter,  std::vector<const location::Stop*>& uniq_stops_vect) const;

	json::Node RenderMap(location::TransportCatalogue& transport_catalog) const;

private:
	RenderSettings settings_;

};

//--------------------------------- SphereProjector templateted method ---------------------------------

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
					double max_height, double padding)
		: padding_(padding) {
	if (points_begin == points_end) {
		return;
	}
	const auto [left_it, right_it]
		= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
			  return lhs.lng < rhs.lng;
		  });
	min_lon_ = left_it->lng;
	const double max_lon = right_it->lng;
	const auto [bottom_it, top_it]
		= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
			  return lhs.lat < rhs.lat;
		  });
	const double min_lat = bottom_it->lat;
	max_lat_ = top_it->lat;
	std::optional<double> width_zoom;
	if (!IsZero(max_lon - min_lon_)) {
		width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
	}
	std::optional<double> height_zoom;
	if (!IsZero(max_lat_ - min_lat)) {
		height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
	}
	if (width_zoom && height_zoom) {
		zoom_coeff_ = std::min(*width_zoom, *height_zoom);
	} else if (width_zoom) {
		zoom_coeff_ = *width_zoom;
	} else if (height_zoom) {
		zoom_coeff_ = *height_zoom;
	}
}

}// namespace output
}// namespace svg

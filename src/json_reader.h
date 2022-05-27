#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace location {
namespace input {

using StopData = std::map<std::string_view, std::vector<std::pair<std::string_view, int>>>;

json::Document LoadJSON(const std::string& s);
std::string Print(const json::Node& node);

StopData ParseStop(TransportCatalogue& transport_catalog, const json::Node* stop_node);
void ParseRoute(TransportCatalogue& transport_catalog, const json::Node*);
void ParseMap(TransportCatalogue& transport_catalog, const json::Node* settings_node);
void ParseRoutingSettings(TransportCatalogue& transport_catalog, const json::Node* bus_node);

std::variant<std::string, std::vector<double>>  DiscernColor(const json::Node* color_node);

void FormRequest(TransportCatalogue& transport_catalog, const json::Node*);

void RequestOutput(TransportCatalogue& transport_catalog, const json::Node*, std::ostream& output);
void FillData(TransportCatalogue& transport_catalog, svg::output::MapRenderer& render, RequestHandler& handler, std::istream& input);
void FillRequestsData(TransportCatalogue& transport_catalog, svg::output::MapRenderer& render, RequestHandler& handler, std::istream& input);

}// namespace input
}// namespace location

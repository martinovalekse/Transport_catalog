#pragma once

#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "router.h"
#include "transport_catalogue.h"
#include <transport_catalogue.pb.h>
#include "transport_router.h"
#include "serialization.h"

#include <optional>
#include <fstream>
#include <unordered_set>


namespace location {
namespace input {

class RequestHandler {
public:
	RequestHandler(TransportCatalogue& transport_catalog, svg::output::MapRenderer& renderer, graph::TransportRouter& transport_router)
		: transport_catalog_(transport_catalog), renderer_(renderer), transport_router_(transport_router) { }

	void AddRequest(int id, std::string_view type, std::string_view name, std::string_view opt_str);
	void AddSerializationFilename(std::string_view name);
	void AddDeserializationFilename(std::string_view name);

	graph::TransportRouter& GetTransportRouter() {
		return transport_router_;
	}

	void Save();
	void Load();

	json::Node Result() const;

private:
	location::TransportCatalogue& transport_catalog_;
	svg::output::MapRenderer& renderer_;
	graph::TransportRouter& transport_router_;
	std::vector<Request> stat_requests_;
	std::string serialization_filename;
	std::string deserialization_filename;

	void Serialization(std::ostream& out_str) const;

	transport_catalogue_serialize::StopsList StopListSerialization(std::map<std::string, int>& stops_pointer) const;
	transport_catalogue_serialize::BusesList BusesListSerialization(std::map<std::string, int>& stops_pointer) const;
	transport_catalogue_serialize::DistancesList DistancesListSerialization(std::map<std::string, int>& stops_pointer) const;
	transport_catalogue_serialize::CatalogData TransportCatalogSerialization() const;
	transport_catalogue_serialize::RoutingSettings RoutingSettingsSerialization() const;
	inline transport_catalogue_serialize::Color ColorSerialization(svg::Color& input_color) const;
	transport_catalogue_serialize::RenderSettings RenderSettingsSerialization() const;

	void Deserialization(std::istream& input_st);

	void DeserializationStopsList(transport_catalogue_serialize::StopsList stop_list, std::map<int, std::string>& stops_pointer);
	void DeserializationBusesList(transport_catalogue_serialize::BusesList buses_list, std::map<int, std::string>& stops_pointer);
	void DeserializationDistancesList(transport_catalogue_serialize::DistancesList distances_list, std::map<int, std::string>& stops_pointer);
	void DeserializationTransportCatalog(transport_catalogue_serialize::CatalogData catalog_data);
	void DeserializationRoutingSettings(transport_catalogue_serialize::RoutingSettings routing_settings);
	inline svg::Color DeserializationColor(transport_catalogue_serialize::Color color_serialized);
	void DeserializationRenderSettings(transport_catalogue_serialize::RenderSettings render_settings_serialized);

};

}// namespace input
}// namespace location


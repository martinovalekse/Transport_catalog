#pragma once

#include "json_builder.h"
#include "router.h"
#include "transport_catalogue.h"

#include <map>
#include <memory>
#include <set>

namespace graph {

struct IDList {
	std::map<std::string_view, size_t> stop_to_id;
	std::map<size_t, std::string_view> id_to_stop;
};

struct EdgeData {
	std::string_view route_name;
	int span_count;
	double time;
};

inline bool operator>(const EdgeData& A, const EdgeData& B) { return A.time > B.time; }
inline bool operator<(const EdgeData& A, const EdgeData& B) { return A.time < B.time; }
inline EdgeData operator+(const EdgeData& A, const EdgeData& B) { return {{}, 0, A.time + B.time}; }

class TransportRouter {
	struct Settings {
		int velocity = 0;
		int wait_time = 0;
	};

public:
	void SetupRouter(location::TransportCatalogue& transport_catalog, int velocity, int wait_time);

	void CalculateRoute(std::string_view from, std::string_view to, json::Builder& request_result);

	const IDList* GetIDList() const { return &id_list_;	}
	const Settings* GetSettings() const { return &settings;	}

private:
	std::unique_ptr<graph::Router<EdgeData>> router_ = nullptr;
	std::unique_ptr<graph::DirectedWeightedGraph<EdgeData>> graph_holder_ = nullptr;
	std::vector<graph::Edge<EdgeData>> edges_;
	IDList id_list_;
	Settings settings;

	void BuildPaths(location::TransportCatalogue& transport_catalog);
	inline double CalculateTime(location::TransportCatalogue& transport_catalog, const location::Bus& bus, size_t from, size_t to);
	void SetRoutingSettings(int velocity, int wait_time);
	void PrepareGraphAndRouter(location::TransportCatalogue& transport_catalog);
};

}// namespace graph

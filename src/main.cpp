
#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
	 if (argc != 2) {
		PrintUsage();
		return 1;
	}
	const std::string_view mode(argv[1]);
	location::TransportCatalogue transport_catalog;
	svg::output::MapRenderer map_renderer;
	graph::TransportRouter transport_router;
	location::input::RequestHandler request_hander(transport_catalog, map_renderer, transport_router);
	if (mode == "make_base"sv) {
		std::fstream fin("make_base.json", std::ios::in);
		location::input::FillData(transport_catalog, map_renderer, request_hander, fin);
		request_hander.Save();

		fin.close();
	 } else if (mode == "process_requests"sv) {
		std::fstream PR_fin("process_request.json", std::ios::in);
		location::input::FillRequestsData(transport_catalog, map_renderer, request_hander, PR_fin);
		PR_fin.close();
		request_hander.Load();
		json::Print(json::Document(request_hander.Result()), std::cout);
	 } else {
		 PrintUsage();
		 return 1;
	 }
	return 0;
}

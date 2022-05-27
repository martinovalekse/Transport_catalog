cmake_minimum_required(VERSION 3.11)

project(Transport_catalog CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)

set(TRANSPORT_CATALOG_FILES ./src/transport_catalogue.h ./src/transport_catalogue.cpp ./src/domain.h ./src/geo.h ./src/geo.cpp ./src/graph.h)
set(JSON_FILES ./src/json.h ./src/json.cpp ./src/json_builder.h ./src/json_builder.cpp ./src/json_reader.h ./src/json_reader.cpp)
set(ROUTER_FILES ./src/transport_router.h ./src/transport_router.cpp ./src/router.h ./src/graph.h ./src/ranges.h)
set(MAP_RENDER_FILES ./src/map_renderer.h ./src/map_renderer.cpp ./src/svg.h ./src/svg.cpp )
set(REQUEST_HANDLER_FILES ./src/request_handler.h ./src/request_handler.cpp)
set(SERIALIZATION_FILES ./src/serialization.h ./src/serialization.cpp transport_catalogue.proto)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_CATALOG_FILES} ${JSON_FILES} ${ROUTER_FILES} ${MAP_RENDER_FILES} ${REQUEST_HANDLER_FILES} ${SERIALIZATION_FILES} ./src/main.cpp)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads) 
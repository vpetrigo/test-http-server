cmake_minimum_required(VERSION 3.4)
project(new-http-server)

include_directories(deps/libuv/include)

add_executable(http_server server.cpp)
set_property(TARGET http_server PROPERTY CXX_STANDARD 11)
set_property(TARGET http_server PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
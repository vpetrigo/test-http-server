cmake_minimum_required(VERSION 3.4)
project(new-http-server)

include(ExternalProject)

ExternalProject_Add(
    libuv
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libuv
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND mingw32-make -f Makefile.mingw
    INSTALL_COMMAND ""
)

include_directories(deps/libuv/include)
link_directories(deps/libuv)

add_executable(http_server server.cpp)
target_link_libraries(http_server uv)
if(WIN32)
    target_link_libraries(http_server wsock32 ws2_32 userenv iphlpapi psapi)
endif()
set_property(TARGET http_server PROPERTY CXX_STANDARD 11)
set_property(TARGET http_server PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_dependencies(http_server libuv)
cmake_minimum_required(VERSION 2.8)
project(new-http-server)

include(ExternalProject)
find_package(Threads)

if(${MINGW})
    ExternalProject_Add(
        libuv
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libuv
        UPDATE_COMMAND git submodule update
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_COMMAND mingw32-make -f Makefile.mingw
        INSTALL_COMMAND ""
    )
elseif(${UNIX})
    ExternalProject_Add(
        libuv
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libuv
        DOWNLOAD_COMMAND git submodule update
        CONFIGURE_COMMAND ./autogen.sh
        BUILD_IN_SOURCE 1
        BUILD_COMMAND gmake 
        INSTALL_COMMAND gmake install
    )

ExternalProject_Add_Step(
        libuv conf
        COMMAND ./configure --prefix ${PROJECT_SOURCE_DIR}/deps/libuv/output
        DEPENDEES configure
        DEPENDERS build
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/libuv
    )

endif()

include_directories(deps/libuv/include)
link_directories(${PROJECT_SOURCE_DIR}/deps/libuv ${PROJECT_SOURCE_DIR}/ddeps/libuv/output/lib)

add_executable(http_server server.cpp)
target_link_libraries(http_server uv)
if(${WIN32})
    target_link_libraries(http_server wsock32 ws2_32 userenv iphlpapi psapi)
elseif(${UNIX})
    target_link_libraries(http_server ${CMAKE_THREAD_LIBS_INIT})
endif()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
    target_link_libraries(http_server kvm)
endif()
set_property(TARGET http_server PROPERTY CXX_STANDARD 11)
set_property(TARGET http_server PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_dependencies(http_server libuv)

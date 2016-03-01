cmake_minimum_required(VERSION 2.8)
project(new-http-server)

include(ExternalProject)
find_package(Threads)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(MAKE_CMD gmake)
else()
    set(MAKE_CMD make)
endif()

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
        DOWNLOAD_COMMAND git submodule init
        UPDATE_COMMAND git submodule update
        CONFIGURE_COMMAND ./autogen.sh
        BUILD_IN_SOURCE 1
        BUILD_COMMAND ${MAKE_CMD} 
        INSTALL_COMMAND ${MAKE_CMD} install
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

add_executable(final server.cpp)
target_link_libraries(final uv)
if(${WIN32})
    target_link_libraries(final wsock32 ws2_32 userenv iphlpapi psapi)
elseif(${UNIX})
    target_link_libraries(final ${CMAKE_THREAD_LIBS_INIT})
endif()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
    target_link_libraries(final kvm)
endif()
set_property(TARGET final PROPERTY CXX_STANDARD 11)
set_property(TARGET final PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_dependencies(final libuv)

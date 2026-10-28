cmake_minimum_required(VERSION 3.16)
project(diag_sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DIAG_BUILD_EXAMPLES "Build example programs" ON)
option(DIAG_BUILD_TESTS "Build tests" OFF)

find_package(Threads REQUIRED)

add_library(diag STATIC
    src/util/byte_buffer.cpp
    src/util/log.cpp
    src/net/socket.cpp
    src/uds/service_layer.cpp
    src/doip/codec.cpp
    src/doip/entity_server.cpp
    src/doip/tester_client.cpp
    src/node/ecu_node.cpp
    src/node/tester_node.cpp
)

target_include_directories(diag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(diag PUBLIC Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(diag PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DIAG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(DIAG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_test(NAME diag_tests COMMAND diag_tests)
endif()

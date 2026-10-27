cmake_minimum_required(VERSION 3.28)

project(tieqi LANGUAGES CXX)

option(TQ_DEV_TOOLS "编译 ImGui 开发工具" OFF)
option(TQ_BUILD_TESTS "编译测试" ON)
option(TQ_WARNINGS_AS_ERRORS "把警告当错误" OFF)

include(cmake/Dependencies.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Warnings.cmake)

add_library(tq_core STATIC
    src/core/core.cpp
    src/core/sim/Rng.cpp
    src/core/sim/Battle.cpp
    src/core/rules/Combat.cpp
    src/core/rules/Preview.cpp)
target_include_directories(tq_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
tq_apply_warnings(tq_core)

add_library(tq_data STATIC
    src/data/data.cpp
    src/data/Load.cpp
    src/data/Strings.cpp)
target_link_libraries(tq_data PUBLIC tq_core nlohmann_json::nlohmann_json)
tq_apply_warnings(tq_data)

add_executable(tieqi src/app/main.cpp)
target_link_libraries(tieqi PRIVATE tq_core tq_data SFML::Graphics SFML::Audio)
if(TQ_DEV_TOOLS)
    target_link_libraries(tieqi PRIVATE ImGui-SFML::ImGui-SFML)
endif()
tq_apply_warnings(tieqi)

add_executable(tq_headless tools/headless/main.cpp)
target_link_libraries(tq_headless PRIVATE tq_core tq_data)
tq_apply_warnings(tq_headless)

if(TQ_BUILD_TESTS)
    enable_testing()
    add_executable(tq_tests
        tests/placeholder.cpp
        tests/core/geometry.cpp
        tests/core/terrain.cpp
        tests/data/load.cpp
        tests/core/sim.cpp
        tests/core/combat.cpp)
    target_link_libraries(tq_tests PRIVATE tq_core tq_data doctest::doctest)
    file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" TQ_SOURCE_DIR_FWD)
    target_compile_definitions(tq_tests PRIVATE "TQ_SOURCE_DIR=\"${TQ_SOURCE_DIR_FWD}\"")
    tq_apply_warnings(tq_tests)
    add_test(NAME tq_tests COMMAND tq_tests)
endif()

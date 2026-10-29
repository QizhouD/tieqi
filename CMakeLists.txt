cmake_minimum_required(VERSION 3.28)

project(tieqi LANGUAGES CXX)

option(TQ_DEV_TOOLS "Build the ImGui dev tools" OFF)
option(TQ_BUILD_TESTS "Build tests" ON)
option(TQ_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

include(cmake/Dependencies.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Warnings.cmake)

add_library(tq_core STATIC
    src/core/core.cpp
    src/core/sim/Rng.cpp
    src/core/sim/Battle.cpp
    src/core/ai/Ai.cpp
    src/core/rules/Combat.cpp
    src/core/rules/Preview.cpp)
target_include_directories(tq_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
tq_apply_warnings(tq_core)

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE TQ_BUILD_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT TQ_BUILD_ID)
    set(TQ_BUILD_ID "unknown")
endif()

add_library(tq_data STATIC
    src/data/data.cpp
    src/data/Load.cpp
    src/data/Strings.cpp
    src/data/Replay.cpp)
target_compile_definitions(tq_data PRIVATE "TQ_BUILD_ID=\"${TQ_BUILD_ID}\"")
target_link_libraries(tq_data PUBLIC tq_core nlohmann_json::nlohmann_json)
tq_apply_warnings(tq_data)

add_executable(tieqi
    src/app/main.cpp
    src/app/App.cpp
    src/app/Window.cpp
    src/app/ResourceManager.cpp
    src/app/AudioManager.cpp
    src/app/ParticleSystem.cpp
    src/app/Launch.cpp
    src/app/SaveData.cpp
    src/app/Script.cpp
    src/app/Synth.cpp
    src/app/MapView.cpp
    src/app/UnitView.cpp
    src/app/Orders.cpp
    src/app/Report.cpp
    src/app/Hud.cpp
    src/app/TextCache.cpp
    src/app/Effects.cpp)
target_link_libraries(tieqi PRIVATE tq_core tq_data SFML::Graphics SFML::Audio)
add_custom_command(TARGET tieqi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:tieqi>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:tieqi>/assets)
if(TQ_DEV_TOOLS)
    # The constants tab lists every rule constant with its compiled value.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/core/rules/Constants.h)
    file(STRINGS ${CMAKE_SOURCE_DIR}/src/core/rules/Constants.h TQ_CONSTANT_LINES
         REGEX "^inline constexpr float k[A-Za-z0-9_]+ = ")
    set(TQ_CONSTANT_TABLE "")
    foreach(line IN LISTS TQ_CONSTANT_LINES)
        string(REGEX REPLACE "^inline constexpr float (k[A-Za-z0-9_]+) = .*$" "\\1" name "${line}")
        string(APPEND TQ_CONSTANT_TABLE "    {\"${name}\", ${name}},\n")
    endforeach()
    file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/generated/app/dev/Constants.inc CONTENT "${TQ_CONSTANT_TABLE}")
    target_sources(tieqi PRIVATE src/app/dev/DevTools.cpp src/app/dev/Intent.cpp)
    target_include_directories(tieqi PRIVATE ${CMAKE_BINARY_DIR}/generated)
    target_compile_definitions(tieqi PRIVATE TQ_DEV_TOOLS=1)
    target_link_libraries(tieqi PRIVATE ImGui-SFML::ImGui-SFML)
endif()
tq_apply_warnings(tieqi)

add_executable(tq_headless
    tools/headless/main.cpp
    tools/headless/Policy.cpp
    tools/headless/Run.cpp)
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
        tests/core/combat.cpp
        tests/core/melee.cpp
        tests/core/formation.cpp
        tests/core/flow.cpp
        tests/core/ai.cpp
        tests/replay/replay.cpp
        tests/parity/duel.cpp
        tests/parity/baseline.cpp
        tests/app/shell.cpp
        tests/app/pieces.cpp
        tests/app/orders.cpp
        tests/app/report.cpp
        tests/app/effects.cpp
        tests/app/glyphs.cpp
        tests/app/dev.cpp
        src/app/dev/Intent.cpp
        src/app/Launch.cpp
        src/app/Orders.cpp
        src/app/Report.cpp
        src/app/SaveData.cpp
        src/app/Script.cpp
        src/app/Synth.cpp
        tools/headless/Policy.cpp
        tools/headless/Run.cpp)
    target_link_libraries(tq_tests PRIVATE tq_core tq_data doctest::doctest SFML::Graphics)
    file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" TQ_SOURCE_DIR_FWD)
    target_compile_definitions(tq_tests PRIVATE "TQ_SOURCE_DIR=\"${TQ_SOURCE_DIR_FWD}\"")
    tq_apply_warnings(tq_tests)
    add_test(NAME tq_tests COMMAND tq_tests --test-case-exclude=*slow*)
    add_test(NAME tq_tests_slow COMMAND tq_tests --test-case=*slow*)
    set_tests_properties(tq_tests_slow PROPERTIES LABELS slow)
    if(NOT TQ_DEV_TOOLS)
        add_test(NAME tieqi_without_imgui
                 COMMAND ${CMAKE_COMMAND} -DBINARY=$<TARGET_FILE:tieqi> -P ${CMAKE_SOURCE_DIR}/cmake/CheckNoImGui.cmake)
    endif()
endif()

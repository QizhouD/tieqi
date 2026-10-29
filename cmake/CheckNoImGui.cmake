# Fails when the given binary carries any trace of Dear ImGui (symbols, debug info, or its built-in strings).
# Usage: cmake -DBINARY=<file> -P CheckNoImGui.cmake
if(NOT BINARY OR NOT EXISTS "${BINARY}")
    message(FATAL_ERROR "BINARY not found: ${BINARY}")
endif()
file(STRINGS "${BINARY}" hits REGEX "ImGui|imgui|IMGUI")
list(LENGTH hits count)
if(count GREATER 0)
    list(SUBLIST hits 0 5 sample)
    message(FATAL_ERROR "${BINARY} contains ${count} ImGui strings, for example: ${sample}")
endif()
message(STATUS "${BINARY}: no ImGui strings")

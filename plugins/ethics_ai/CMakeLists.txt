cmake_minimum_required(VERSION 3.20)

# Compatibility shim: implementation has been integrated under src/ethics_ai.
# Keep this CMake entry so older workflows that reference plugins/ethics_ai
# still resolve to the canonical source location.

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

set(_ethics_ai_src_dir "${THEMIS_ROOT_DIR}/src/ethics_ai")
set(_ethics_ai_private_dir "${THEMIS_ROOT_DIR}/plugins/private/ethics_ai")

if(EXISTS "${_ethics_ai_src_dir}/CMakeLists.txt")
    message(STATUS "plugins/ethics_ai: delegating build to src/ethics_ai")
    if(NOT TARGET ethics_ai_plugin)
        add_subdirectory("${_ethics_ai_src_dir}" "${CMAKE_CURRENT_BINARY_DIR}/src_ethics_ai")
    endif()
elseif(WITH_PRIVATE_ETHICS_AI AND EXISTS "${_ethics_ai_private_dir}/CMakeLists.txt")
    message(STATUS "plugins/ethics_ai: delegating build to private plugin path")
    if(NOT TARGET ethics_ai_plugin)
        add_subdirectory("${_ethics_ai_private_dir}" "${CMAKE_CURRENT_BINARY_DIR}/private_ethics_ai")
    endif()
else()
    message(STATUS "plugins/ethics_ai: optional source not available; skipping compatibility entry point")
endif()

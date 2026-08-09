cmake_minimum_required(VERSION 3.20)

# ============================================================================
# ThemisDB Whisper Plugin
# Audio transcription plugin backed by whisper.cpp (optional).
# Without whisper.cpp the plugin compiles a stub that returns empty results,
# allowing all unit-tests to run without a model file.
# ============================================================================

project(whisper_plugin VERSION 2.3.0 LANGUAGES CXX)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
set(WHISPER_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_chunk_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_transcriber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_plugin.cpp
)

add_library(whisper_plugin STATIC ${WHISPER_PLUGIN_SOURCES})

target_include_directories(whisper_plugin PUBLIC
    "${THEMIS_ROOT_DIR}/include"
)

target_compile_features(whisper_plugin PUBLIC cxx_std_17)

# ── Optional: whisper.cpp ────────────────────────────────────────────────────
option(THEMIS_ENABLE_WHISPER "Enable whisper.cpp for real transcription" OFF)
if(THEMIS_ENABLE_WHISPER)
    set(WHISPER_ROOT "${THEMIS_ROOT_DIR}/whisper.cpp" CACHE PATH "Path to local whisper.cpp checkout")
    find_library(WHISPER_LIB NAMES whisper)
    if(WHISPER_LIB)
        target_link_libraries(whisper_plugin PUBLIC ${WHISPER_LIB})
        target_compile_definitions(whisper_plugin PUBLIC THEMIS_ENABLE_WHISPER=1)
        message(STATUS "  whisper_plugin: whisper.cpp found – real transcription enabled")
    else()
        message(WARNING "  whisper_plugin: THEMIS_ENABLE_WHISPER=ON but libwhisper not found – stub mode")
    endif()
else()
    message(STATUS "  whisper_plugin: whisper.cpp disabled – stub/test mode")
endif()

# ── nlohmann/json ─────────────────────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(whisper_plugin PUBLIC nlohmann_json::nlohmann_json)
else()
    if(EXISTS "${THEMIS_ROOT_DIR}/third_party/nlohmann/json.hpp")
        target_include_directories(whisper_plugin PUBLIC
            "${THEMIS_ROOT_DIR}/third_party/nlohmann")
    endif()
endif()

# ── Plugin manifest ───────────────────────────────────────────────────────────
configure_file(
    "${THEMIS_ROOT_DIR}/plugins/whisper/plugin.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
    @ONLY
)

message(STATUS "whisper_plugin v${PROJECT_VERSION} configured")

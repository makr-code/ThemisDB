cmake_minimum_required(VERSION 3.20)

# ============================================================================
# ThemisDB llama_cpp Plugin
# Standalone LLM plugin that wraps the existing LlamaWrapper/ILLMPlugin
# and exposes it for dynamic loading via THEMIS_LLM_PLUGIN().
# v2.1.0: streaming token output, batch inference, PluginManager registrar
# ============================================================================

project(llama_cpp_plugin VERSION 2.1.0 LANGUAGES CXX)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

add_library(llama_cpp_plugin STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/llama_cpp_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llama_cpp_registrar.cpp
)

target_include_directories(llama_cpp_plugin PUBLIC
    "${THEMIS_ROOT_DIR}/include"
)

target_compile_features(llama_cpp_plugin PUBLIC cxx_std_17)

# ── nlohmann/json ─────────────────────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(llama_cpp_plugin PUBLIC nlohmann_json::nlohmann_json)
else()
    if(EXISTS "${THEMIS_ROOT_DIR}/third_party/nlohmann/json.hpp")
        target_include_directories(llama_cpp_plugin PUBLIC
            "${THEMIS_ROOT_DIR}/third_party/nlohmann")
    endif()
endif()

configure_file(
    "${THEMIS_ROOT_DIR}/plugins/llama_cpp/plugin.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
    @ONLY
)

message(STATUS "llama_cpp_plugin v${PROJECT_VERSION} configured")

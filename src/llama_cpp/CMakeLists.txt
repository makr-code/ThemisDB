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

# ── Real LlamaWrapper backend (optional) ──────────────────────────────────────
# When THEMIS_ENABLE_LLM is ON (set by the parent build or edition cmake),
# compile in the LlamaWrapper and propagate THEMIS_LLM_ENABLED so the plugin
# activates real inference instead of stub mode.
if(THEMIS_ENABLE_LLM)
    target_compile_definitions(llama_cpp_plugin PUBLIC THEMIS_LLM_ENABLED)
    # Link against the aggregated LLM library target when it exists (full build).
    if(TARGET themis_llm)
        target_link_libraries(llama_cpp_plugin PUBLIC themis_llm)
    elseif(TARGET themis_core)
        target_link_libraries(llama_cpp_plugin PUBLIC themis_core)
    endif()
    if(TARGET spdlog::spdlog)
        target_link_libraries(llama_cpp_plugin PUBLIC spdlog::spdlog)
    endif()
    if(TARGET fmt::fmt)
        target_link_libraries(llama_cpp_plugin PUBLIC fmt::fmt)
    endif()
endif()

message(STATUS "llama_cpp_plugin v${PROJECT_VERSION} configured")

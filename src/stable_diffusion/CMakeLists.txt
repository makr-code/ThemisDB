cmake_minimum_required(VERSION 3.20)

# ============================================================================
# ThemisDB Stable Diffusion Plugin
# Image generation plugin backed by stable-diffusion.cpp (optional).
# Without stable-diffusion.cpp the plugin compiles a stub that returns minimal
# PNG frames, allowing all unit-tests to run without a model file.
# ============================================================================

project(stable_diffusion_plugin VERSION 2.3.0 LANGUAGES CXX)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

set(SD_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sd_config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sd_prompt_sanitizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sd_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sd_plugin_registrar.cpp
    ${THEMIS_ROOT_DIR}/src/utils/checksum_utils.cpp
)

add_library(stable_diffusion_plugin STATIC ${SD_PLUGIN_SOURCES})

target_include_directories(stable_diffusion_plugin PUBLIC
    "${THEMIS_ROOT_DIR}/include"
)

target_compile_features(stable_diffusion_plugin PUBLIC cxx_std_17)

option(THEMIS_SD_ENABLE_SHA256 "Enable model SHA-256 integrity gate (requires OpenSSL)" ON)
if(THEMIS_SD_ENABLE_SHA256)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND OR OPENSSL_FOUND)
        target_link_libraries(stable_diffusion_plugin PRIVATE OpenSSL::Crypto)
        target_compile_definitions(stable_diffusion_plugin PRIVATE THEMIS_SD_ENABLE_SHA256=1)
        message(STATUS "  stable_diffusion_plugin: OpenSSL found – SHA-256 model integrity gate enabled")
    else()
        message(WARNING "  stable_diffusion_plugin: THEMIS_SD_ENABLE_SHA256=ON but OpenSSL not found – SHA-256 gate disabled")
    endif()
else()
    message(STATUS "  stable_diffusion_plugin: SHA-256 model integrity gate disabled (THEMIS_SD_ENABLE_SHA256=OFF)")
endif()

# ── Optional: stable-diffusion.cpp ──────────────────────────────────────────
option(THEMIS_ENABLE_STABLE_DIFFUSION "Enable stable-diffusion.cpp for real generation" OFF)
if(THEMIS_ENABLE_STABLE_DIFFUSION)
    set(STABLE_DIFFUSION_ROOT "${THEMIS_ROOT_DIR}/stable-diffusion.cpp" CACHE PATH "Path to local stable-diffusion.cpp checkout")
    find_library(SD_LIB
        NAMES stable-diffusion stable_diffusion
        PATHS
            ${STABLE_DIFFUSION_ROOT}/build
            ${STABLE_DIFFUSION_ROOT}/build/src
            ${STABLE_DIFFUSION_ROOT}/build/lib
            ${THEMIS_ROOT_DIR}/external/stable-diffusion.cpp/build
            /usr/local/lib
            /usr/lib
    )
    if(SD_LIB)
        target_link_libraries(stable_diffusion_plugin PUBLIC ${SD_LIB})
        target_compile_definitions(stable_diffusion_plugin PUBLIC THEMIS_ENABLE_STABLE_DIFFUSION=1)
        message(STATUS "  stable_diffusion_plugin: stable-diffusion.cpp found – real generation enabled")
    else()
        message(WARNING "  stable_diffusion_plugin: THEMIS_ENABLE_STABLE_DIFFUSION=ON but lib not found – stub mode")
    endif()
else()
    message(STATUS "  stable_diffusion_plugin: stable-diffusion.cpp disabled – stub/test mode")
endif()

# ── nlohmann/json ─────────────────────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(stable_diffusion_plugin PUBLIC nlohmann_json::nlohmann_json)
else()
    if(EXISTS "${THEMIS_ROOT_DIR}/third_party/nlohmann/json.hpp")
        target_include_directories(stable_diffusion_plugin PUBLIC
            "${THEMIS_ROOT_DIR}/third_party/nlohmann")
    endif()
endif()

configure_file(
    "${THEMIS_ROOT_DIR}/plugins/stable_diffusion/plugin.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
    @ONLY
)

message(STATUS "stable_diffusion_plugin v${PROJECT_VERSION} configured")

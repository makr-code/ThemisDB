# ThemisDB Enterprise Plugins – manifest / configuration directory
# Compiled plugin implementations have been migrated to src/plugins/ and
# include/plugins/.  This file handles only non-compiled artefacts (manifests,
# config examples, philosophy profiles, etc.) and legacy compatibility paths.
#
# Per-plugin feature flags (e.g. THEMIS_PLUGIN_ETHICS_AI) are defined in
# cmake/features/PluginFeatures.cmake and have already been applied before
# this file is included from cmake/CMakeLists.txt.

cmake_minimum_required(VERSION 3.20)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()

message(STATUS "Processing ThemisDB plugin manifests (plugins/)")

function(_themis_add_optional_public_plugin_dir label source_dir binary_dir)
    if(EXISTS "${source_dir}/CMakeLists.txt")
        message(STATUS "  - ${label}")
        add_subdirectory("${source_dir}" "${binary_dir}")
    else()
        message(STATUS "  - ${label} not available (optional public source missing: ${source_dir})")
    endif()
endfunction()

# Legacy compatibility entry points for integrated runtime plugins.
# The canonical implementations live under src/* and public APIs under include/*.
# These add_subdirectory calls resolve older workflows that still reference the
# plugins/ tree directly. Each delegated CMakeLists guards against duplicate
# target creation.

if(THEMIS_PLUGIN_ETHICS_AI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai/CMakeLists.txt" AND NOT TARGET ethics_ai_plugin)
    message(STATUS "  - Ethics AI compatibility entry point")
    add_subdirectory(ethics_ai)
endif()

if(THEMIS_PLUGIN_RPC_GRPC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/grpc/CMakeLists.txt")
    message(STATUS "  - gRPC RPC compatibility entry point")
    add_subdirectory(rpc/grpc)
endif()

if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/image_analysis/CMakeLists.txt" AND NOT TARGET themis_image_onnx_clip)
    message(STATUS "  - Image Analysis compatibility entry point")
    add_subdirectory(image_analysis)
endif()

if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user_storage_encrypted/CMakeLists.txt")
    message(STATUS "  - User Storage Encrypted compatibility entry point")
    add_subdirectory(user_storage_encrypted)
endif()

# Scraper Plugin – agentic gap-detection web scraper
option(THEMIS_PLUGIN_SCRAPER "Build the agentic Scraper Plugin" ON)
if(THEMIS_PLUGIN_SCRAPER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scraper/CMakeLists.txt" AND NOT TARGET scraper_plugin)
    message(STATUS "  - Scraper Plugin (agentic gap-detection scraper)")
    add_subdirectory(scraper)
endif()

# Whisper Plugin – audio transcription (whisper.cpp)
option(THEMIS_PLUGIN_WHISPER "Build the Whisper audio transcription plugin" ON)
if(THEMIS_PLUGIN_WHISPER AND EXISTS "${THEMIS_ROOT_DIR}/src/whisper/CMakeLists.txt")
    message(STATUS "  - Whisper Plugin (audio transcription)")
    add_subdirectory("${THEMIS_ROOT_DIR}/src/whisper" "${CMAKE_CURRENT_BINARY_DIR}/whisper")
endif()

# Stable Diffusion Plugin – image generation (stable-diffusion.cpp)
option(THEMIS_PLUGIN_STABLE_DIFFUSION "Build the Stable Diffusion image generation plugin" ON)
if(THEMIS_PLUGIN_STABLE_DIFFUSION AND EXISTS "${THEMIS_ROOT_DIR}/src/stable_diffusion/CMakeLists.txt")
    message(STATUS "  - Stable Diffusion Plugin (image generation)")
    add_subdirectory("${THEMIS_ROOT_DIR}/src/stable_diffusion" "${CMAKE_CURRENT_BINARY_DIR}/stable_diffusion")
endif()

# llama_cpp Plugin – LLM backend plugin for dynamic loading
option(THEMIS_PLUGIN_LLAMA_CPP "Build the llama_cpp LLM backend plugin" ON)
if(THEMIS_PLUGIN_LLAMA_CPP AND EXISTS "${THEMIS_ROOT_DIR}/src/llama_cpp/CMakeLists.txt")
    message(STATUS "  - llama_cpp Plugin (LLM backend)")
    add_subdirectory("${THEMIS_ROOT_DIR}/src/llama_cpp" "${CMAKE_CURRENT_BINARY_DIR}/llama_cpp")
endif()

# SAGA Orchestrator Plugin – transaction SAGA as DLL/plugin
option(THEMIS_PLUGIN_SAGA_ORCHESTRATOR "Build the SAGA orchestrator plugin" ON)
if(THEMIS_PLUGIN_SAGA_ORCHESTRATOR AND EXISTS "${THEMIS_ROOT_DIR}/src/transaction/saga_plugin/CMakeLists.txt" AND NOT TARGET saga_orchestrator_plugin)
    message(STATUS "  - SAGA Orchestrator Plugin (transaction workflow orchestration)")
    add_subdirectory("${THEMIS_ROOT_DIR}/src/transaction/saga_plugin" "${CMAKE_CURRENT_BINARY_DIR}/saga_plugin")
endif()

# Public optional submodule plugins for module externalization.
# These are no-hard-fail entry points and can be absent in community-only checkouts.
if(THEMIS_PLUGIN_GEO AND THEMIS_EXTERNALIZE_GEO_PLUGIN)
    _themis_add_optional_public_plugin_dir("Geo Plugin (public submodule: themisdb_geo)"
        "${CMAKE_CURRENT_SOURCE_DIR}/themisdb_geo"
        "${CMAKE_CURRENT_BINARY_DIR}/themisdb_geo")
endif()

if(THEMIS_PLUGIN_TIMESERIES AND THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN)
    _themis_add_optional_public_plugin_dir("TimeSeries Plugin (public submodule: themisdb_timeseries)"
        "${CMAKE_CURRENT_SOURCE_DIR}/themisdb_timeseries"
        "${CMAKE_CURRENT_BINARY_DIR}/themisdb_timeseries")
endif()

include("${THEMIS_ROOT_DIR}/cmake/PrivatePlugins.cmake" OPTIONAL)
if(COMMAND themis_register_private_plugins)
    themis_register_private_plugins("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

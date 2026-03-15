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

# Legacy compatibility entry points for integrated runtime plugins.
# The canonical implementations live under src/* and public APIs under include/*.
# These add_subdirectory calls resolve older workflows that still reference the
# plugins/ tree directly. Each delegated CMakeLists guards against duplicate
# target creation.

if(THEMIS_PLUGIN_ETHICS_AI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai/CMakeLists.txt")
    message(STATUS "  - Ethics AI compatibility entry point")
    add_subdirectory(ethics_ai)
endif()

if(THEMIS_PLUGIN_RPC_GRPC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/grpc/CMakeLists.txt")
    message(STATUS "  - gRPC RPC compatibility entry point")
    add_subdirectory(rpc/grpc)
endif()

if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/image_analysis/CMakeLists.txt")
    message(STATUS "  - Image Analysis compatibility entry point")
    add_subdirectory(image_analysis)
endif()

if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user_storage_encrypted/CMakeLists.txt")
    message(STATUS "  - User Storage Encrypted compatibility entry point")
    add_subdirectory(user_storage_encrypted)
endif()

# GPU Impact Analysis Plugin (enterprise/non-open source)
option(THEMIS_BUILD_GPU_IMPACT_PLUGIN "Build GPU Impact Analysis Plugin" ON)
if(THEMIS_BUILD_GPU_IMPACT_PLUGIN)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/enterprise/gpu_impact_analysis/CMakeLists.txt")
        message(STATUS "  - GPU Impact Analysis Plugin")
        add_subdirectory(enterprise/gpu_impact_analysis)
    else()
        message(WARNING "GPU Impact Analysis Plugin source not found at plugins/enterprise/gpu_impact_analysis")
    endif()
endif()


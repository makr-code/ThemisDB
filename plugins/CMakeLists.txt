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


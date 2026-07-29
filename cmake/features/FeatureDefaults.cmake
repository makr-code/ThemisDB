# ThemisDB Feature System
# Central registry of all optional features
# Edition defaults are already set by edition files; these are user-overridable

if(NOT THEMIS_EDITION_SELECTED)
    message(FATAL_ERROR "FeatureDefaults.cmake requires EditionDefaults.cmake to be included first")
endif()

message(STATUS "==========================================")
message(STATUS "Feature Configuration:")
message(STATUS "==========================================")

# Include feature modules
include(${CMAKE_CURRENT_LIST_DIR}/LLMFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/NetworkFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/GPUFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/SecurityFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ToolsFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/OptimizationFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PluginFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PrivatePluginFeatures.cmake)

# Set global flag for feature configuration completed
set(THEMIS_FEATURES_CONFIGURED TRUE CACHE INTERNAL "Feature configuration completed")

message(STATUS "==========================================")

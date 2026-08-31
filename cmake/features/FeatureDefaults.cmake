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
include(${CMAKE_CURRENT_LIST_DIR}/GeoFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/SecurityFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ToolsFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/OptimizationFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PluginFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PrivatePluginFeatures.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/BenchmarkRuntimeProfile.cmake)

function(themis_report_compact_switches)
    get_cmake_property(_cache_vars CACHE_VARIABLES)
    set(_entries "")

    foreach(_var IN LISTS _cache_vars)
        if(NOT _var MATCHES "^THEMIS_")
            continue()
        endif()

        # Ignore non-switch metadata; only output the actual build knobs and edition state.
        if(_var MATCHES "^(THEMIS_FEATURES_CONFIGURED|THEMIS_EDITION_SELECTED|THEMIS_PLATFORM_DETECTED|THEMIS_ROOT_DIR|THEMIS_PACKAGE_STORE|THEMIS_TRIPLET|THEMIS_TARGET_ARCH|THEMIS_VERSION|THEMIS_VERSION_STRING|THEMIS_BUILD_ID|THEMIS_BUILD_UUID|THEMIS_BUILD_VERSION_STRING|THEMIS_VCPKG_ROOT|.*(DIR|PATH|ROOT|STORE|UUID)$)$")
            continue()
        endif()

        set(_value "$CACHE{${_var}}")
        if(_value MATCHES "^(ON|OFF|TRUE|FALSE|0|1)$" OR _value MATCHES "^[A-Z_0-9]+$")
            list(APPEND _entries "${_var}=${_value}")
        endif()
    endforeach()

    list(SORT _entries)
    list(LENGTH _entries _count)
    if(_count GREATER 0)
        string(REPLACE ";" "  " _joined "${_entries}")
        message(STATUS "Themis switches (${_count}): ${_joined}")
    endif()
endfunction()

# Set global flag for feature configuration completed
set(THEMIS_FEATURES_CONFIGURED TRUE CACHE INTERNAL "Feature configuration completed")

themis_report_compact_switches()

message(STATUS "==========================================")

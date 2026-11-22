# src/tensor/CMakeLists.txt
#
# Library target: themis_tensor (Phase 4: Integration & Documentation)
# Version: Phase 4 (2026-07-01)
#
# Sources: 23 implementation files for the Tensor Mid-Layer Abstractions module.
# Responsibilities:
#   - Tensor-layer orchestration and mid-layer routing
#   - Compression strategies (TT-SVD, Quantization, Sampling, Hashing)
#   - Routing strategies (QualityBased, ShardAware, Adaptive)
#   - Prioritization and redundancy detection
#   - Error handling and recovery with RAII guards
#   - Adapter repository and fingerprint-based similarity search
#
# Dependencies: 
#   - themis_core (for ANN frontdoor and base functionality)
#   - themis_index (for ANN interfaces)
#   - Standard C++ libraries with C++17 support

cmake_minimum_required(VERSION 3.16)

# Tensor module source files
set(THEMIS_TENSOR_SOURCES
    adapter_repository.cpp
    compression_strategy.cpp
    hiss_structural_search.cpp
    hnsw_tt_bridge.cpp
    persistent_tensor_fingerprint_graph.cpp
    ht_index.cpp
    hyper_index_builder.cpp
    tensor_compression_routing_accelerator.cpp
    tensor_butterfly_operator.cpp
    tensor_core_bridge.cpp
    dream_orchestrator.cpp
    tensor_error_handling.cpp
    tensor_fingerprint_graph.cpp
    tensor_index.cpp
    tensor_index_manager.cpp
    tensor_ingestion_bridge.cpp
    tensor_mid_layer.cpp
    tensor_mmap_bridge.cpp
    tensor_redundancy_detection.cpp
    tensor_routing_strategy.cpp
    tensor_summary_types.cpp
    tensor_workflow_observability.cpp
    tnsr_task.cpp
    utr_converter.cpp
)

# Avoid compiling the same tensor translation units twice. The monolithic
# themis_core (StorageEnhancements.cmake / cmake/CMakeLists.txt) and the
# modular themis_storage (ModularBuild.cmake) already own a subset of these
# sources. themis_tensor drops overlapping sources only when the owner exports
# its symbols: static/object owners always qualify; shared owners must enable
# Windows auto-export or use default visibility on other platforms.
option(THEMIS_TENSOR_DEDUP_OWNER_SOURCES
       "Exclude tensor sources already compiled into linkable core/storage owners" ON)

function(_themis_tensor_owner_exports_sources _target _result)
    get_target_property(_owner_type ${_target} TYPE)
    set(_exports_sources FALSE)
    if(_owner_type STREQUAL "STATIC_LIBRARY" OR
       _owner_type STREQUAL "OBJECT_LIBRARY")
        set(_exports_sources TRUE)
    elseif(_owner_type STREQUAL "SHARED_LIBRARY")
        if(WIN32)
            get_target_property(_auto_export ${_target} WINDOWS_EXPORT_ALL_SYMBOLS)
            if(_auto_export)
                set(_exports_sources TRUE)
            endif()
        else()
            get_target_property(_visibility ${_target} CXX_VISIBILITY_PRESET)
            if(NOT "${_visibility}" STREQUAL "hidden")
                set(_exports_sources TRUE)
            endif()
        endif()
    endif()
    set(${_result} ${_exports_sources} PARENT_SCOPE)
endfunction()

set(_themis_tensor_source_owners "")
if(THEMIS_TENSOR_DEDUP_OWNER_SOURCES)
    foreach(_owner IN ITEMS themis_core themis_storage)
        if(NOT TARGET ${_owner})
            continue()
        endif()
        _themis_tensor_owner_exports_sources(${_owner} _owner_exports_sources)
        if(NOT _owner_exports_sources)
            continue()
        endif()
        get_target_property(_owner_dir ${_owner} SOURCE_DIR)
        get_target_property(_owner_sources ${_owner} SOURCES)
        if(NOT _owner_sources)
            continue()
        endif()
        set(_owner_abs_sources "")
        foreach(_src IN LISTS _owner_sources)
            if(_src MATCHES "\\$<")
                continue()
            endif()
            get_filename_component(_abs "${_src}" ABSOLUTE BASE_DIR "${_owner_dir}")
            list(APPEND _owner_abs_sources "${_abs}")
        endforeach()

        set(_owned_here FALSE)
        set(_remaining_sources "")
        foreach(_src IN LISTS THEMIS_TENSOR_SOURCES)
            get_filename_component(_abs "${_src}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
            if("${_abs}" IN_LIST _owner_abs_sources)
                set(_owned_here TRUE)
            else()
                list(APPEND _remaining_sources "${_src}")
            endif()
        endforeach()

        if(_owned_here)
            list(LENGTH THEMIS_TENSOR_SOURCES _before)
            list(LENGTH _remaining_sources _after)
            math(EXPR _dropped "${_before} - ${_after}")
            message(STATUS "Tensor module: ${_dropped} source(s) provided by ${_owner}; not recompiled in themis_tensor")
            set(THEMIS_TENSOR_SOURCES ${_remaining_sources})
            list(APPEND _themis_tensor_source_owners ${_owner})
        endif()
    endforeach()
endif()

# Create the themis_tensor library
add_library(themis_tensor STATIC ${THEMIS_TENSOR_SOURCES})

# Owners of de-duplicated sources must be linked so consumers of themis_tensor
# still resolve those symbols. Static owners do not link back to themis_tensor,
# so this introduces no link cycle.
foreach(_owner IN LISTS _themis_tensor_source_owners)
    target_link_libraries(themis_tensor PRIVATE ${_owner})
endforeach()

# Public include directories
target_include_directories(themis_tensor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link required core libraries
target_link_libraries(themis_tensor
    PUBLIC
        Threads::Threads
    PRIVATE
        spdlog::spdlog
)

# Link optional core module if available
if(TARGET themis_core)
    target_link_libraries(themis_tensor PRIVATE themis_core)
endif()

# Link optional index module if available
if(TARGET themis_index)
    target_link_libraries(themis_tensor PRIVATE themis_index)
endif()

# Link optional RAG module if available
if(TARGET themis_rag)
    target_link_libraries(themis_tensor PRIVATE themis_rag)
endif()

# Link optional query module if available
if(TARGET themis_query)
    target_link_libraries(themis_tensor PRIVATE themis_query)
endif()

# C++ standard and feature requirements
target_compile_features(themis_tensor PUBLIC cxx_std_17)

set_target_properties(themis_tensor PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED ON
    VERSION               1.0.0
    SOVERSION             1
)

# Compiler warning handling
if(MSVC)
    target_compile_options(themis_tensor PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(themis_tensor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add library to exports only when explicitly enabled
# Exporting targets during an in-source configure can fail if dependent
# targets are not part of the same export set. Make this opt-in so
# consuming build systems enable it during install/export steps.
if(DEFINED THEMIS_ENABLE_EXPORTS AND THEMIS_ENABLE_EXPORTS)
    export(TARGETS themis_tensor FILE ThemisThensorTargets.cmake)
endif()

list(LENGTH THEMIS_TENSOR_SOURCES _themis_tensor_source_count)
message(STATUS "Tensor module: themis_tensor library configured (${_themis_tensor_source_count} local sources, C++17)")

# ── Optional: MAP_SHARED zero-copy path (always available on POSIX — no extra lib) ──
# buildFromFd() uses mmap(MAP_SHARED) which is part of POSIX; no extra CMake
# find_package is required.  The THEMIS_HAS_MMAP define is set by the platform
# guard already present in tensor_mmap_bridge.cpp.

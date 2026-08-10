# Access Model module — cache↔storage tier coordination
#
# Sources are included in themis_core via cmake/CMakeLists.txt.
# This file documents the module sources for IDE tooling and standalone builds.
#
# Phase 2 Q3 2026 — CMakeLists.txt module registration (ROADMAP item completed)

cmake_minimum_required(VERSION 3.20)

set(ACCESS_MODEL_SOURCES
    access_coordinator.cpp
    age_based_policy.cpp
    access_metrics.cpp
    promotion_demotion.cpp
)

# Standalone library target (used by tests when themis_core is not available)
if(NOT TARGET themis_access_model)
    add_library(themis_access_model STATIC ${ACCESS_MODEL_SOURCES})

    target_include_directories(themis_access_model
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(themis_access_model
        PUBLIC
            spdlog::spdlog
            Threads::Threads
    )

    target_compile_features(themis_access_model PUBLIC cxx_std_17)
endif()

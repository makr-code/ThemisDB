<<<<<<< HEAD
# EPIC 2 — Evaluation & Benchmarking: query planner (EPIC 2.5) implementation.
# Phases 2-4 of docs/EPIC2_QUERY_PLANNER.md.
#
# Enable the library target and test target once the build environment provides
# GTest (via vcpkg). All other evaluation sub-issues (2.1-2.4, 2.6-2.7) remain
# as documentation-first scaffolds until their implementation PRs land.

# Source and header lists for query_planner (EPIC 2.5 only)
set(EVALUATION_QUERY_PLANNER_HEADERS
    include/query_planner.h
)
set(EVALUATION_QUERY_PLANNER_SOURCES
    src/query_planner.cc
)

# Placeholder — source and header lists for remaining sub-issues will be added
# with their implementation PRs:
# set(EVALUATION_PUBLIC_HEADERS ...)
# set(EVALUATION_SOURCES ...)

# Uncomment when build targets are approved for activation:
# add_library(themis_evaluation STATIC ${EVALUATION_SOURCES})
# target_include_directories(themis_evaluation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# query_planner static library (EPIC 2.5)
# Activated when the parent CMakeLists confirms vcpkg and include paths are ready.
if(TARGET GTest::gtest OR DEFINED THEMIS_BUILD_EPIC2)
    add_library(epic2_evaluation_lib STATIC
        ${EVALUATION_QUERY_PLANNER_SOURCES}
    )
    target_include_directories(epic2_evaluation_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_features(epic2_evaluation_lib PUBLIC cxx_std_17)
=======
# EPIC 2 — Evaluation & Benchmarking module.
# Includes benchmark_matrix (EPIC 2.2, Issue #5438) and hardware_profile sources.

# ============================================================================
# themis_evaluation — EPIC 2 evaluation library
# ============================================================================

set(EVALUATION_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/benchmark_matrix.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hardware_profile.h"
)

set(EVALUATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_matrix.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware_profile.cc"
)

if(NOT TARGET themis_evaluation)
    add_library(themis_evaluation STATIC ${EVALUATION_SOURCES})

    target_include_directories(themis_evaluation
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include/evaluation>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(themis_evaluation PUBLIC cxx_std_17)

    # No external dependencies: evaluation library is self-contained.
    target_compile_definitions(themis_evaluation PRIVATE THEMIS_EVALUATION_BUILD=1)

    install(TARGETS themis_evaluation
        ARCHIVE DESTINATION lib
        COMPONENT evaluation
    )
    install(FILES ${EVALUATION_PUBLIC_HEADERS}
        DESTINATION include/evaluation
        COMPONENT evaluation
    )
>>>>>>> origin/develop
endif()

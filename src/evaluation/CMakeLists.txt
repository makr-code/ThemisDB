# EPIC 2 — Evaluation & Benchmarking module.
# Includes benchmark_matrix (EPIC 2.2, Issue #5438), hardware_profile sources,
# the query planner (EPIC 2.5), retrieval metrics and ablation framework
# (Issue #5439), and artifact lifecycle management (EPIC 2.6, Issue #5442)
# implementations.

# ============================================================================
# themis_evaluation — EPIC 2.2 evaluation library (benchmark_matrix, hardware_profile)
#                   + EPIC 2 Phase 2 retrieval metrics and ablation framework
#                   + EPIC 2.6 artifact lifecycle management
# ============================================================================

set(EVALUATION_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/benchmark_matrix.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hardware_profile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/retrieval_metrics.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ablation_framework.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/approximation_rules.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/query_planner.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/artifact_lifecycle.h"
)

set(EVALUATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_matrix.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware_profile.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/retrieval_metrics.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ablation_framework.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/artifact_lifecycle.cc"
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
endif()

# ============================================================================
# epic2_evaluation_lib — EPIC 2.5 query planner library
# ============================================================================

set(EVALUATION_QUERY_PLANNER_HEADERS
    include/query_planner.h
)
set(EVALUATION_QUERY_PLANNER_SOURCES
    src/query_planner.cc
)

# Activated when the build environment provides GTest (via vcpkg) or
# THEMIS_BUILD_EPIC2 is explicitly set.
if(TARGET GTest::gtest OR DEFINED THEMIS_BUILD_EPIC2)
    add_library(epic2_evaluation_lib STATIC
        ${EVALUATION_QUERY_PLANNER_SOURCES}
    )
    target_include_directories(epic2_evaluation_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(epic2_evaluation_lib PUBLIC cxx_std_17)
endif()

# ============================================================================
# epic2_approximation_lib — EPIC 2.4 approximation boundary rule engine
# ============================================================================

set(EVALUATION_APPROXIMATION_HEADERS
    include/approximation_rules.h
)
set(EVALUATION_APPROXIMATION_SOURCES
    src/approximation_rules.cc
)

if(TARGET GTest::gtest OR DEFINED THEMIS_BUILD_EPIC2)
    add_library(epic2_approximation_lib STATIC
        ${EVALUATION_APPROXIMATION_SOURCES}
    )
    target_include_directories(epic2_approximation_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(epic2_approximation_lib PUBLIC cxx_std_17)
    # approximation_rules.cc includes query_planner.h; link the planner lib.
    if(TARGET epic2_evaluation_lib)
        target_link_libraries(epic2_approximation_lib PUBLIC epic2_evaluation_lib)
    endif()
endif()

# ============================================================================
# epic2_artifact_lifecycle_lib — EPIC 2.6 artifact lifecycle management
# ============================================================================

set(EVALUATION_ARTIFACT_LIFECYCLE_HEADERS
    include/artifact_lifecycle.h
)
set(EVALUATION_ARTIFACT_LIFECYCLE_SOURCES
    src/artifact_lifecycle.cc
)

if(TARGET GTest::gtest OR DEFINED THEMIS_BUILD_EPIC2)
    add_library(epic2_artifact_lifecycle_lib STATIC
        ${EVALUATION_ARTIFACT_LIFECYCLE_SOURCES}
    )
    target_include_directories(epic2_artifact_lifecycle_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(epic2_artifact_lifecycle_lib PUBLIC cxx_std_17)
endif()


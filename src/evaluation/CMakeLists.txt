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
endif()

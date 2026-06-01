# EPIC 2 — Evaluation & Benchmarking: skeleton headers and sources are in place.
# Enable library targets after interface review and acceptance-test sign-off.

set(EVALUATION_PUBLIC_HEADERS
    include/hardware_profile.h
    include/benchmark_matrix.h
    include/evaluation_metrics.h
    include/approximation_rules.h
    include/query_planner.h
    include/artifact_lifecycle.h
    include/storage_strategy.h
)

set(EVALUATION_SOURCES
    src/hardware_profile.cc
    src/benchmark_matrix.cc
    src/evaluation_metrics.cc
    src/approximation_rules.cc
    src/query_planner.cc
    src/artifact_lifecycle.cc
    src/storage_strategy.cc
)

# Uncomment when build targets are approved for activation:
# add_library(themis_evaluation STATIC ${EVALUATION_SOURCES})
# target_include_directories(themis_evaluation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

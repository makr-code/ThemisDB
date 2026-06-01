# Documentation-first CMake scaffold for planned epic work.

set(EVALUATION_PLANNED_PUBLIC_HEADERS
    include/hardware_profile.h
    include/benchmark_matrix.h
    include/evaluation_metrics.h
    include/approximation_rules.h
    include/query_planner.h
    include/artifact_lifecycle.h
    include/storage_strategy.h
)

set(EVALUATION_PLANNED_SOURCES
    src/hardware_profile.cc
    src/benchmark_matrix.cc
    src/evaluation_metrics.cc
    src/approximation_rules.cc
    src/query_planner.cc
    src/artifact_lifecycle.cc
    src/storage_strategy.cc
)

# Targets intentionally remain disabled until interface review and implementation approval are complete.

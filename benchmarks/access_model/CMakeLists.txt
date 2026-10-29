if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: access_model")

set(ACCESS_MODEL_IMPL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/access_model/access_coordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/access_model/age_based_policy.cpp
    ${CMAKE_SOURCE_DIR}/src/access_model/access_metrics.cpp
    ${CMAKE_SOURCE_DIR}/src/access_model/promotion_demotion.cpp
    ${CMAKE_SOURCE_DIR}/src/access_model/access_model_logging.cpp
    ${CMAKE_SOURCE_DIR}/src/access_model/access_model_trace.cpp
)

# The benchmark consumes the public access-model API, but the final link step
# needs the full module implementation closure as well. Linking only the factory
# object leaves unresolved support symbols from metrics/logging/trace helpers.
# Add the implementation files explicitly after target creation to guarantee the
# build graph includes the factory and all support objects for the benchmark.
themis_add_standard_benchmark(
    bench_access_coordinator_gates
    bench_access_coordinator_gates.cpp
)

if(TARGET bench_access_coordinator_gates)
    target_sources(bench_access_coordinator_gates PRIVATE
        ${ACCESS_MODEL_IMPL_SOURCES}
    )
endif()

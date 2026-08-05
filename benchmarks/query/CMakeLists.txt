if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: query")

themis_add_standard_benchmark(bench_query bench_query.cpp)
themis_add_standard_benchmark(bench_phase4_performance bench_phase4_performance.cpp)
themis_add_standard_benchmark(bench_geospatial_phase2 bench_geospatial_phase2.cpp)


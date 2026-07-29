if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: projects")

themis_add_standard_benchmark(bench_projects_release_gates bench_projects_release_gates.cpp)

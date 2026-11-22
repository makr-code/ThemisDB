if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: projects")

themis_add_standard_benchmark(bench_projects_release_gates bench_projects_release_gates.cpp)
themis_add_standard_benchmark(bench_projects_workflows bench_projects_workflows.cpp)

themis_register_benchmark_ctest(
    NAME    bench_projects_workflows
    TARGET  bench_projects_workflows
    MODULE  projects
    TIMEOUT 120
    MIN_TIME "0.01s"
)

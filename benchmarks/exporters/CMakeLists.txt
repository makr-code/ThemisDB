if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: exporters")

themis_add_standard_benchmark(bench_exporters              bench_exporters.cpp)
themis_add_standard_benchmark(bench_exporters_release_gates bench_exporters_release_gates.cpp)

# Wave D: Dedicated operability gates (EX-BM-01..04)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_exporters_dedicated_gates.cpp")
    if(NOT TARGET bench_exporters_dedicated_gates)
        themis_add_standard_benchmark(bench_exporters_dedicated_gates
            bench_exporters_dedicated_gates.cpp)
    endif()
endif()

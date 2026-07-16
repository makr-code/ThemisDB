if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic1_retrieval")

themis_add_standard_benchmark(bench_ann_distance_cpu_vs_flat bench_ann_distance_cpu_vs_flat.cpp)
themis_add_standard_benchmark(bench_multishard_exact bench_multishard_exact.cpp)

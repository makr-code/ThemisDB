# EPIC 1 Retrieval benchmarks
if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic1_retrieval")

# lora_loading_bench is now implemented; enable it when benchmark::benchmark is available.
if(TARGET benchmark::benchmark AND TARGET themis_retrieval)
    add_executable(epic1_lora_loading_bench
        "${CMAKE_CURRENT_SOURCE_DIR}/lora_loading_bench.cc"
    )
    target_include_directories(epic1_lora_loading_bench PRIVATE
        ${THEMIS_ROOT_DIR}/src/retrieval/include
        ${THEMIS_ROOT_DIR}/include
    )
    target_link_libraries(epic1_lora_loading_bench PRIVATE
        themis_retrieval
        benchmark::benchmark
        Threads::Threads
    )
    target_compile_features(epic1_lora_loading_bench PRIVATE cxx_std_17)
endif()

# Remaining benchmark targets will be enabled incrementally alongside implementation work.
themis_add_standard_benchmark(bench_ann_distance_cpu_vs_flat bench_ann_distance_cpu_vs_flat.cpp)
themis_add_standard_benchmark(bench_multishard_exact bench_multishard_exact.cpp)

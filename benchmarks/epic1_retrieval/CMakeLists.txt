# EPIC 1 Retrieval benchmarks

set(EPIC1_RETRIEVAL_PLANNED_BENCHMARKS
    ann_retrieval_bench.cc
    tensor_routing_bench.cc
    graph_validation_bench.cc
    lora_loading_bench.cc
    federated_bench.cc
)

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

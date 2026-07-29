if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: distributed_knowledge")

themis_add_standard_benchmark(bench_distributed_knowledge    bench_distributed_knowledge.cpp)
themis_add_standard_benchmark(bench_distributed_knowledge_or bench_distributed_knowledge_or.cpp)
themis_add_standard_benchmark(bench_dk_release_gates         bench_dk_release_gates.cpp)

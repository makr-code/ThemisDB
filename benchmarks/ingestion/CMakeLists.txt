if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: ingestion")

themis_add_standard_benchmark(bench_ingestion_extraction     bench_ingestion_extraction.cpp)
themis_add_standard_benchmark(bench_ingestion_kv             bench_ingestion_kv.cpp)
themis_add_standard_benchmark(bench_ingestion_quality_judge  bench_ingestion_quality_judge.cpp)
themis_add_standard_benchmark(bench_ingestion_release_gates  bench_ingestion_release_gates.cpp)

# Wave D dedicated gates — IN-BM-01..04
# Labels: wave_d;benchmark;not_release_critical
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_ingestion_dedicated_gates.cpp")
    themis_add_standard_benchmark(bench_ingestion_dedicated_gates bench_ingestion_dedicated_gates.cpp)
endif()

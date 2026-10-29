cmake_minimum_required(VERSION 3.20)
# benchmarks/retrieval — Wave D dedicated gate benchmarks

# RT-BM-01..04: retrieval engine dedicated benchmark gates
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_retrieval_dedicated_gates.cpp")
    add_executable(bench_retrieval_dedicated_gates
        bench_retrieval_dedicated_gates.cpp
    )
    target_include_directories(bench_retrieval_dedicated_gates PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(bench_retrieval_dedicated_gates PRIVATE
        benchmark::benchmark
        Threads::Threads
    )
    target_compile_definitions(bench_retrieval_dedicated_gates PRIVATE
        THEMIS_TEST_BUILD=1
    )
endif()

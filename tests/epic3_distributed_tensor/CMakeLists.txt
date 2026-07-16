# EPIC 3 Distributed Tensor contract tests

set(EPIC3_DISTRIBUTED_TENSOR_TESTS
    integrity_verification_test.cc
)

set(EPIC3_DISTRIBUTED_TENSOR_BENCHMARKS
    integrity_verification_bench.cc
)

# Wired when GTest and the distributed_tensor scaffold sources are available.
# Each test executable is linked with epic3_distributed_tensor_lib and GTest::gtest_main

foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    if(TARGET epic3_distributed_tensor_lib)
        target_link_libraries(${test_name} PRIVATE epic3_distributed_tensor_lib GTest::gtest_main)
        add_test(NAME ${test_name} COMMAND ${test_name})
    else()
        message(STATUS "Skipping test ${test_name}: epic3_distributed_tensor_lib target not available")
    endif()
endforeach()

# Benchmark executables (require Google Benchmark)
foreach(bench_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_BENCHMARKS)
    get_filename_component(bench_name ${bench_src} NAME_WE)
    if(TARGET benchmark::benchmark)
        add_executable(${bench_name} ${bench_src})
        if(TARGET epic3_distributed_tensor_lib)
            target_link_libraries(${bench_name} PRIVATE epic3_distributed_tensor_lib benchmark::benchmark benchmark::benchmark_main)
        else()
            message(STATUS "Skipping benchmark ${bench_name}: epic3_distributed_tensor_lib target not available")
        endif()
    else()
        message(STATUS "Skipping benchmark ${bench_name}: benchmark::benchmark target not available")
    endif()
endforeach()


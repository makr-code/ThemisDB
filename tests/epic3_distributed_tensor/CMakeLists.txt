<<<<<<< HEAD
# EPIC 3 Distributed Tensor — Phase A ManifestStore tests
# Registered as a module-focused test once themis_core and GTest are available.

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_manifest_store_phase_a.cpp")
    message(STATUS "Adding EPIC3: Phase A ManifestStore tests (MS-01..12)")

    add_executable(test_manifest_store_phase_a
        test_manifest_store_phase_a.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
    )

    target_include_directories(test_manifest_store_phase_a PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_manifest_store_phase_a PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_manifest_store_phase_a PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE distributed_tensor
            NAME ManifestStorePhaseATests
            TARGET test_manifest_store_phase_a
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor manifest phase-a advisory-only freshness
        )
    else()
        add_test(NAME ManifestStorePhaseATests COMMAND test_manifest_store_phase_a)
    endif()

    message(STATUS "  ManifestStore: store/get/list/evict, freshness, metrics, concurrency (MS-01..12)")
else()
    message(STATUS "EPIC3 ManifestStore tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_manifest_store_phase_a.cpp")
endif()
=======
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
>>>>>>> origin/develop


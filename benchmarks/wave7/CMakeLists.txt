# Wave 7 Benchmark Suite — CMakeLists.txt
# Registers all four W7 benchmark targets with the ThemisDB build system.

cmake_minimum_required(VERSION 3.20)

find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Wave 7 benchmarks skipped: Google Benchmark not found.")
    return()
endif()

# ---------------------------------------------------------------------------
# Helper: add a single wave-7 benchmark executable
# ---------------------------------------------------------------------------
function(add_w7_benchmark TARGET_NAME SOURCE_FILE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
        return()
    endif()
    if(TARGET ${TARGET_NAME})
        return()
    endif()

    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/benchmarks
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        benchmark::benchmark
        themis_core
        Threads::Threads
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(${TARGET_NAME})

    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin/benchmarks/wave7
        COMPONENT benchmarks
    )

    message(STATUS "  [Wave 7] Registered: ${TARGET_NAME}")
endfunction()

# ---------------------------------------------------------------------------
# W7-A: Final Release-Critical Workload Sign-Off
# ---------------------------------------------------------------------------
add_w7_benchmark(
    bench_w7a_release_critical_signoff
    bench_w7a_release_critical_signoff.cpp
)

# ---------------------------------------------------------------------------
# W7-B: Endurance / Soak & Peak Transition Validation
# ---------------------------------------------------------------------------
add_w7_benchmark(
    bench_w7b_endurance_soak
    bench_w7b_endurance_soak.cpp
)

# ---------------------------------------------------------------------------
# W7-C: Degradation, Fault & Recovery Performance Characterization
# ---------------------------------------------------------------------------
add_w7_benchmark(
    bench_w7c_degradation_fault_recovery
    bench_w7c_degradation_fault_recovery.cpp
)

# ---------------------------------------------------------------------------
# W7-D: Final Guardrails, Variance Zeroing & Operability
# ---------------------------------------------------------------------------
add_w7_benchmark(
    bench_w7d_guardrails_variance_operability
    bench_w7d_guardrails_variance_operability.cpp
)

# ---------------------------------------------------------------------------
# W7-P2D05: Phase 2 D05 Runtime Integration (Compression + State Store)
# ---------------------------------------------------------------------------
add_w7_benchmark(
    bench_w7_p2d05_compression_state_store
    bench_p2_d05_compression_state_store.cpp
)

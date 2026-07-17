# Wave 8 Benchmark Suite — CMakeLists.txt
# Registers all four W8 benchmark targets with the ThemisDB build system.

cmake_minimum_required(VERSION 3.20)

find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Wave 8 benchmarks skipped: Google Benchmark not found.")
    return()
endif()

# ---------------------------------------------------------------------------
# Helper: add a single wave-8 benchmark executable
# ---------------------------------------------------------------------------
function(add_w8_benchmark TARGET_NAME SOURCE_FILE)
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
        RUNTIME DESTINATION bin/benchmarks/wave8
        COMPONENT benchmarks
    )

    message(STATUS "  [Wave 8] Registered: ${TARGET_NAME}")
endfunction()

# ---------------------------------------------------------------------------
# W8-A: Incident Regression Shielding
# ---------------------------------------------------------------------------
add_w8_benchmark(
    bench_w8a_incident_regression_shielding
    bench_w8a_incident_regression_shielding.cpp
)

# ---------------------------------------------------------------------------
# W8-B: Threshold Hardening & Drift Detection
# ---------------------------------------------------------------------------
add_w8_benchmark(
    bench_w8b_threshold_hardening_drift_detection
    bench_w8b_threshold_hardening_drift_detection.cpp
)

# ---------------------------------------------------------------------------
# W8-C: Deterministic CI Harness
# ---------------------------------------------------------------------------
add_w8_benchmark(
    bench_w8c_deterministic_ci_harness
    bench_w8c_deterministic_ci_harness.cpp
)

# ---------------------------------------------------------------------------
# W8-D: Operability, Runbooks & Ownership
# ---------------------------------------------------------------------------
add_w8_benchmark(
    bench_w8d_operability_runbooks_ownership
    bench_w8d_operability_runbooks_ownership.cpp
)

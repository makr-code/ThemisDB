# Wave 9 Benchmark Suite — CMakeLists.txt
# Registers all four W9 benchmark targets with the ThemisDB build system.

cmake_minimum_required(VERSION 3.20)

find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Wave 9 benchmarks skipped: Google Benchmark not found.")
    return()
endif()

# ---------------------------------------------------------------------------
# Helper: add a single wave-9 benchmark executable
# ---------------------------------------------------------------------------
function(add_w9_benchmark TARGET_NAME SOURCE_FILE)
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
        RUNTIME DESTINATION bin/benchmarks/wave9
        COMPONENT benchmarks
    )

    message(STATUS "  [Wave 9] Registered: ${TARGET_NAME}")
endfunction()

# ---------------------------------------------------------------------------
# W9-A: Security Overhead & Audit
# ---------------------------------------------------------------------------
add_w9_benchmark(
    bench_w9a_security_overhead_audit
    bench_w9a_security_overhead_audit.cpp
)

# ---------------------------------------------------------------------------
# W9-B: SLA Measurement & Compliance
# ---------------------------------------------------------------------------
add_w9_benchmark(
    bench_w9b_sla_measurement_compliance
    bench_w9b_sla_measurement_compliance.cpp
)

# ---------------------------------------------------------------------------
# W9-C: Chaos & Fault Recovery
# ---------------------------------------------------------------------------
add_w9_benchmark(
    bench_w9c_chaos_fault_recovery
    bench_w9c_chaos_fault_recovery.cpp
)

# ---------------------------------------------------------------------------
# W9-D: Multi-Tenant Isolation
# ---------------------------------------------------------------------------
add_w9_benchmark(
    bench_w9d_multi_tenant_isolation
    bench_w9d_multi_tenant_isolation.cpp
)

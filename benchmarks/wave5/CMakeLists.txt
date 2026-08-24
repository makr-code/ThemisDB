# Wave 5 Benchmark Suite — CMakeLists.txt
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 ThemisDB Contributors
#
# Registers four benchmark targets for Wave 5 (Pre-Production Confidence):
#   bench_w5a_production_workloads  — B5-A: production-critical E2E workloads
#   bench_w5b_degradation           — B5-B: failure/degradation performance validation
#   bench_w5c_determinism           — B5-C: determinism, variance control & reproducibility
#   bench_w5d_governance            — B5-D: governance, diagnostics & maintainability
#
# Prerequisites (inherited from parent CMakeLists.txt):
#   - benchmark::benchmark already found
#   - themis_add_standard_benchmark() function defined
#   - THEMIS_BUILD_BENCHMARKS == ON (enforced by parent)
#
# CTest labels:  wave5;w5a / wave5;w5b / wave5;w5c / wave5;w5d
# CI output:     --benchmark_out=<target>.json --benchmark_out_format=json
# Baseline dir:  benchmarks/baselines/wave5/
# Runbook:       benchmarks/wave5/RUNBOOK_W5.md

cmake_minimum_required(VERSION 3.20)

# Guard: only build when invoked from the benchmarks root
if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

# Guard: Google Benchmark required
if(NOT TARGET benchmark::benchmark)
    message(STATUS "[wave5] Google Benchmark not found — skipping Wave 5 targets")
    return()
endif()

message(STATUS "[wave5] Configuring Wave 5 benchmark targets")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bench_results")

# ---------------------------------------------------------------------------
# Helper — adds a Wave 5 benchmark target with standard link deps and CTest
# ---------------------------------------------------------------------------
function(wave5_add_benchmark target_name source_file label)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
        message(STATUS "[wave5] Source not found: ${source_file} — skipping ${target_name}")
        return()
    endif()

    if(TARGET ${target_name})
        return()
    endif()

    add_executable(${target_name} "${source_file}")

    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src"
    )

    target_link_libraries(${target_name} PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(${target_name} PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(${target_name})

    install(TARGETS ${target_name}
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    # CTest registration: use canonical benchmark helper
    themis_register_benchmark_ctest(
        NAME         "${target_name}"
        TARGET       "${target_name}"
        MODULE       "wave5"
        TIMEOUT      120
        EXTRA_LABELS "${label}"
    )

    message(STATUS "[wave5] Registered ${target_name} (label: wave5;${label})")
endfunction()

# ---------------------------------------------------------------------------
# B5-A: Production-Critical Workload Coverage
# ---------------------------------------------------------------------------
wave5_add_benchmark(
    bench_w5a_production_workloads
    bench_w5a_production_workloads.cpp
    w5a
)

# ---------------------------------------------------------------------------
# B5-B: Failure/Degradation Performance Validation
# ---------------------------------------------------------------------------
wave5_add_benchmark(
    bench_w5b_degradation
    bench_w5b_degradation.cpp
    w5b
)

# ---------------------------------------------------------------------------
# B5-C: Determinism, Variance Control & Reproducibility
# ---------------------------------------------------------------------------
wave5_add_benchmark(
    bench_w5c_determinism
    bench_w5c_determinism.cpp
    w5c
)

# ---------------------------------------------------------------------------
# B5-D: Governance, Diagnostics & Maintainability
# ---------------------------------------------------------------------------
wave5_add_benchmark(
    bench_w5d_governance
    bench_w5d_governance.cpp
    w5d
)

message(STATUS "[wave5] Wave 5 benchmark suite configured (4 targets)")

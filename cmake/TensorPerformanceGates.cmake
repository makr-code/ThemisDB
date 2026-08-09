# ThemisDB Tensor Module Performance Gates (Stream A Block A3)
# Locked baseline measurements for release validation
# 
# Lock Date: 2026-08-07 (from TENSOR_Q3_BENCHMARK_BASELINE.md)
# Hardware: Reference CI Linux x64 (8-core, 3.5 GHz)
# Profile: Release (CMAKE_BUILD_TYPE=Release)
#
# Gate IDs: GATE-TN-A3-01 through GATE-TN-A3-06
# All gates feed into release candidate validation pipeline
#
# Usage:
#   - In CI/CD: include(cmake/TensorPerformanceGates.cmake)
#   - Then: add_tensor_performance_gate_tests()

if(NOT THEMIS_BUILD_BENCHMARKS)
    message(STATUS "Tensor performance gates disabled (benchmarks not enabled)")
    return()
endif()

message(STATUS "Configuring Tensor Module Performance Gates (Stream A Block A3)")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-01: Fingerprint Graph Insert Throughput
# 
# Requirement: Batch insert operation must sustain ≥1,500 ops/sec
# Benchmark: BM_TFG_Insert_Throughput with 100-node batches
# Source: FUTURE_ENHANCEMENTS.md §45 (insert ≤ 10 ms for 100K node graph)
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_01_ID "GATE-TN-A3-01")
set(GATE_TN_A3_01_NAME "FG Insert Throughput")
set(GATE_TN_A3_01_TARGET 1500)
set(GATE_TN_A3_01_UNIT "ops/sec")
set(GATE_TN_A3_01_OPERATOR ">=")
set(GATE_TN_A3_01_DESCRIPTION 
    "Fingerprint Graph batch insert throughput >= 1500 ops/sec (100-node batches)")
set(GATE_TN_A3_01_BENCHMARK "BM_TFG_Insert_Throughput/100")
set(GATE_TN_A3_01_NOTES 
    "Tests amortized insert cost; directly measures compliance with 10ms per-node target")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-02: Fingerprint Graph findSimilar p95 Latency
#
# Requirement: Query latency p95 ≤ 80 ms for 10,000 candidate adapters
# Benchmark: BM_TFG_FindSimilar with 10k prefill, top_k=10, 5 repetitions
# Source: FUTURE_ENHANCEMENTS.md §45-46
# Note: This is the "exact TT cosine" path (full path, not fingerprint-only fast path)
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_02_ID "GATE-TN-A3-02")
set(GATE_TN_A3_02_NAME "FG findSimilar p95")
set(GATE_TN_A3_02_TARGET 80)
set(GATE_TN_A3_02_UNIT "ms")
set(GATE_TN_A3_02_OPERATOR "<=")
set(GATE_TN_A3_02_DESCRIPTION 
    "Fingerprint Graph findSimilar() p95 latency <= 80 ms (10k candidates, top_k=10)")
set(GATE_TN_A3_02_BENCHMARK "BM_TFG_FindSimilar/10000_10")
set(GATE_TN_A3_02_NOTES 
    "p95 extracted from 5 repetitions; LSH reduces 10k candidates to ~200, then cosine scoring")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-03: Fingerprint Graph findSimilar p99 Latency
#
# Requirement: Query latency p99 ≤ 140 ms for 10,000 candidate adapters
# Benchmark: BM_TFG_FindSimilar with 10k prefill, top_k=10, 5 repetitions
# Source: FUTURE_ENHANCEMENTS.md §45-46
# Note: Tail latency; allows rare outliers but must not exceed 140 ms
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_03_ID "GATE-TN-A3-03")
set(GATE_TN_A3_03_NAME "FG findSimilar p99")
set(GATE_TN_A3_03_TARGET 140)
set(GATE_TN_A3_03_UNIT "ms")
set(GATE_TN_A3_03_OPERATOR "<=")
set(GATE_TN_A3_03_DESCRIPTION 
    "Fingerprint Graph findSimilar() p99 latency <= 140 ms (10k candidates, top_k=10)")
set(GATE_TN_A3_03_BENCHMARK "BM_TFG_FindSimilar/10000_10")
set(GATE_TN_A3_03_NOTES 
    "p99 extracted from 5 repetitions; tail latency; allows 2-3 standard deviations above mean")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-04: Deduplication Manager Mixed Workload Throughput
#
# Requirement: Sustained throughput ≥ 2,000 ops/sec for 90% read / 10% write mix
# Benchmark: BM_TDM_MixedReadHeavyWorkload with 1k ops/iteration
# Source: FUTURE_ENHANCEMENTS.md §51-52
# Note: Realistic production workload (read-heavy); tests memory stability
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_04_ID "GATE-TN-A3-04")
set(GATE_TN_A3_04_NAME "Dedup Mixed Ops Throughput")
set(GATE_TN_A3_04_TARGET 2000)
set(GATE_TN_A3_04_UNIT "ops/sec")
set(GATE_TN_A3_04_OPERATOR ">=")
set(GATE_TN_A3_04_DESCRIPTION 
    "Deduplication Manager sustained throughput >= 2000 ops/sec (90% reads, 10% writes)")
set(GATE_TN_A3_04_BENCHMARK "BM_TDM_MixedReadHeavyWorkload/1000")
set(GATE_TN_A3_04_NOTES 
    "Mixed workload reflects production patterns; measured via items_processed / time")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-05: Deduplication Manager Per-Operation p95 Latency
#
# Requirement: Per-operation p95 latency ≤ 2.5 ms in mixed workload
# Benchmark: BM_TDM_MixedReadHeavyWorkload with 3 repetitions
# Source: FUTURE_ENHANCEMENTS.md §51-52
# Note: Average operation latency; ensures individual ops don't create tail latencies
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_05_ID "GATE-TN-A3-05")
set(GATE_TN_A3_05_NAME "Dedup Per-Op p95 Latency")
set(GATE_TN_A3_05_TARGET 2.5)
set(GATE_TN_A3_05_UNIT "ms")
set(GATE_TN_A3_05_OPERATOR "<=")
set(GATE_TN_A3_05_DESCRIPTION 
    "Deduplication Manager per-operation p95 latency <= 2.5 ms (mixed workload)")
set(GATE_TN_A3_05_BENCHMARK "BM_TDM_MixedReadHeavyWorkload/1000")
set(GATE_TN_A3_05_NOTES 
    "Derived from (2000 ops/sec) → 0.5 ms/op avg; p95 ~2.5 ms allows variance")

# ═══════════════════════════════════════════════════════════════════════════════
# GATE-TN-A3-06: Deduplication Manager Memory Growth
#
# Requirement: Memory efficiency ≤ 20 bytes/operation in sustained workload
# Benchmark: BM_TDM_MixedReadHeavyWorkload with 10k ops/iteration over 3 reps
# Source: FUTURE_ENHANCEMENTS.md §51-52 (bounded growth requirement)
# Note: Prevents unbounded heap growth; O(n) memory is acceptable if constant is small
# ═══════════════════════════════════════════════════════════════════════════════

set(GATE_TN_A3_06_ID "GATE-TN-A3-06")
set(GATE_TN_A3_06_NAME "Dedup Memory Growth Rate")
set(GATE_TN_A3_06_TARGET 20)
set(GATE_TN_A3_06_UNIT "bytes/op")
set(GATE_TN_A3_06_OPERATOR "<=")
set(GATE_TN_A3_06_DESCRIPTION 
    "Deduplication Manager memory growth rate <= 20 bytes/operation (bounded)")
set(GATE_TN_A3_06_BENCHMARK "BM_TDM_MixedReadHeavyWorkload/10000")
set(GATE_TN_A3_06_NOTES 
    "Measured as (final_bytes - initial_bytes) / total_ops; ensures linear, not exponential growth")

# ═══════════════════════════════════════════════════════════════════════════════
# Gate Validation Test Function
# ═══════════════════════════════════════════════════════════════════════════════

function(add_tensor_performance_gate_tests)
    # Create individual test targets for each gate
    # Each test runs the corresponding benchmark and validates measurement
    
    if(NOT TARGET bench_tensor_fingerprint_graph OR NOT TARGET bench_tensor_deduplication_manager)
        message(WARNING "Tensor benchmark targets not found; skipping gate tests")
        return()
    endif()
    
    # Helper function to create a gate test
    function(add_gate_test gate_id gate_name target operator benchmark_name)
        # Create test that:
        # 1. Runs the benchmark with JSON output
        # 2. Extracts the measurement
        # 3. Compares against target
        # 4. Sets PASS/FAIL status
        
        set(test_name "tensor_performance_gate_${gate_id}")
        
        add_test(
            NAME ${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DGATE_ID=${gate_id}
                -DGATE_NAME="${gate_name}"
                -DGATE_TARGET=${target}
                -DGATE_OPERATOR=${operator}
                -DBENCHMARK_NAME=${benchmark_name}
                -DBUILD_DIR=${CMAKE_BINARY_DIR}
                -P ${CMAKE_CURRENT_LIST_DIR}/TensorGateValidator.cmake
        )
        
        set_tests_properties(${test_name} PROPERTIES
            LABELS "tensor;performance;gates"
            TIMEOUT 300
        )
    endfunction()
    
    # Register all six gates
    add_gate_test("TN-A3-01" "FG Insert Throughput" 
        ${GATE_TN_A3_01_TARGET} ${GATE_TN_A3_01_OPERATOR} "BM_TFG_Insert_Throughput/100")
    
    add_gate_test("TN-A3-02" "FG findSimilar p95" 
        ${GATE_TN_A3_02_TARGET} ${GATE_TN_A3_02_OPERATOR} "BM_TFG_FindSimilar/10000_10")
    
    add_gate_test("TN-A3-03" "FG findSimilar p99" 
        ${GATE_TN_A3_03_TARGET} ${GATE_TN_A3_03_OPERATOR} "BM_TFG_FindSimilar/10000_10")
    
    add_gate_test("TN-A3-04" "Dedup Mixed Ops Throughput" 
        ${GATE_TN_A3_04_TARGET} ${GATE_TN_A3_04_OPERATOR} "BM_TDM_MixedReadHeavyWorkload/1000")
    
    add_gate_test("TN-A3-05" "Dedup Per-Op p95 Latency" 
        ${GATE_TN_A3_05_TARGET} ${GATE_TN_A3_05_OPERATOR} "BM_TDM_MixedReadHeavyWorkload/1000")
    
    add_gate_test("TN-A3-06" "Dedup Memory Growth Rate" 
        ${GATE_TN_A3_06_TARGET} ${GATE_TN_A3_06_OPERATOR} "BM_TDM_MixedReadHeavyWorkload/10000")
    
    message(STATUS "Added 6 Tensor Performance Gate tests")
    message(STATUS "  Run with: ctest -R 'tensor_performance_gate' -V")
    
endfunction()

# ═══════════════════════════════════════════════════════════════════════════════
# Summary Table for Documentation
# ═══════════════════════════════════════════════════════════════════════════════

set(TENSOR_GATES_SUMMARY 
    "
Tensor Module Stream A Block A3 - Performance Gates (Locked 2026-08-07)

┌─────────────┬──────────────────────────────┬────────┬──────┬──────────────────┐
│ Gate ID     │ Name                         │ Target │ Unit │ Benchmark        │
├─────────────┼──────────────────────────────┼────────┼──────┼──────────────────┤
│ TN-A3-01    │ FG Insert Throughput         │ ≥1500  │ o/s  │ BM_TFG_Insert_*  │
│ TN-A3-02    │ FG findSimilar p95 Latency   │ ≤80    │ ms   │ BM_TFG_FindSim*  │
│ TN-A3-03    │ FG findSimilar p99 Latency   │ ≤140   │ ms   │ BM_TFG_FindSim*  │
│ TN-A3-04    │ Dedup Mixed Ops Throughput   │ ≥2000  │ o/s  │ BM_TDM_Mixed*    │
│ TN-A3-05    │ Dedup Per-Op p95 Latency     │ ≤2.5   │ ms   │ BM_TDM_Mixed*    │
│ TN-A3-06    │ Dedup Memory Growth Rate     │ ≤20    │ B/op │ BM_TDM_Mixed*    │
└─────────────┴──────────────────────────────┴────────┴──────┴──────────────────┘

All gates validate against locked baseline (TENSOR_Q3_BENCHMARK_BASELINE.md)
Gates feed into release candidate validation pipeline (GitHub Actions CI)
")

message(STATUS ${TENSOR_GATES_SUMMARY})

# Make gates available to parent CMakeLists
set(THEMIS_TENSOR_GATES_AVAILABLE ON PARENT_SCOPE)

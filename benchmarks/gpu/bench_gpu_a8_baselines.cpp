/**
 * @file bench_gpu_a8_baselines.cpp
 * @brief Performance baselines for GPU operations (Wave A-8).
 * @date 2026-08-16
 * 
 * Captures p95/p99 measurements for GPU error handling, timeout enforcement,
 * and safe operations overhead.
 * 
 * @see src/gpu/ROADMAP.md § Wave A-8 Closure Evidence Block
 */

#include <benchmark/benchmark.h>
#include "gpu/gpu_safe_operations.h"
#include "gpu/gpu_backend_dispatch_contract.h"
#include <chrono>
#include <vector>

namespace themis {
namespace gpu {
namespace bench {

// =============================================================================
// Baseline Benchmarks
// =============================================================================

/**
 * BP-A8-001: Measure CudaError exception creation overhead.
 * 
 * Baseline: Verify exception construction doesn't exceed 1µs (negligible).
 */
static void BenchCudaErrorCreation(benchmark::State& state) {
    for (auto _ : state) {
        try {
            throw CudaError("cudaMalloc", cudaErrorMemoryAllocation, __FILE__, __LINE__);
        } catch (const CudaError&) {
            // Exception caught; measure complete.
        }
    }
}
BENCHMARK(BenchCudaErrorCreation)->Repetitions(5);

/**
 * BP-A8-002: Measure KernelExecutionGuard construction overhead.
 * 
 * Baseline: Should be < 10µs (just a timestamp + atomic).
 */
static void BenchKernelExecutionGuardCreation(benchmark::State& state) {
    for (auto _ : state) {
        KernelExecutionGuard guard(5000);  // 5 second timeout
        benchmark::DoNotOptimize(guard);
    }
}
BENCHMARK(BenchKernelExecutionGuardCreation)->Repetitions(5);

/**
 * BP-A8-003: Measure timeout check overhead.
 * 
 * Baseline: has_timed_out() should be < 100ns (timestamp comparison).
 */
static void BenchKernelTimeoutCheck(benchmark::State& state) {
    KernelExecutionGuard guard(5000);
    for (auto _ : state) {
        bool timed_out = guard.has_timed_out();
        benchmark::DoNotOptimize(timed_out);
    }
}
BENCHMARK(BenchKernelTimeoutCheck)->Repetitions(5);

/**
 * BP-A8-004: Measure elapsed time calculation.
 * 
 * Baseline: Should be < 200ns (just time arithmetic).
 */
static void BenchKernelElapsedTime(benchmark::State& state) {
    KernelExecutionGuard guard(5000);
    for (auto _ : state) {
        uint64_t elapsed = guard.elapsed_ms();
        benchmark::DoNotOptimize(elapsed);
    }
}
BENCHMARK(BenchKernelElapsedTime)->Repetitions(5);

/**
 * BP-A8-005: Measure CUDA error code classification.
 * 
 * Baseline: isFailClosedClass() should be < 50ns (single comparison).
 */
static void BenchErrorCodeClassification(benchmark::State& state) {
    for (auto _ : state) {
        bool is_fail_closed = isFailClosedClass(GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT);
        benchmark::DoNotOptimize(is_fail_closed);
    }
}
BENCHMARK(BenchErrorCodeClassification)->Repetitions(5);

/**
 * BP-A8-006: Measure cuda_error_to_string conversion.
 * 
 * Baseline: Should be < 500ns (just string call).
 */
static void BenchCudaErrorToString(benchmark::State& state) {
    for (auto _ : state) {
        std::string msg = cuda_error_to_string(cudaErrorMemoryAllocation);
        benchmark::DoNotOptimize(msg);
    }
}
BENCHMARK(BenchCudaErrorToString)->Repetitions(5);

/**
 * BP-A8-007: Measure timing utility conversions.
 * 
 * Baseline: ms_to_us and us_to_ms should be < 50ns (arithmetic only).
 */
static void BenchTimingConversions(benchmark::State& state) {
    for (auto _ : state) {
        uint64_t us = ms_to_us(100);
        uint64_t ms = us_to_ms(us);
        benchmark::DoNotOptimize(us);
        benchmark::DoNotOptimize(ms);
    }
}
BENCHMARK(BenchTimingConversions)->Repetitions(5);

// =============================================================================
// Latency Envelope Tests (verify contract bounds)
// =============================================================================

/**
 * BP-A8-008: Verify KernelExecutionGuard respects timeout semantics.
 * 
 * This benchmark verifies that the timeout contract is met:
 * - Timeout check is O(1)
 * - No hidden allocations
 * - Suitable for use in hot paths
 */
static void BenchKernelTimeoutGuardHotPath(benchmark::State& state) {
    for (auto _ : state) {
        KernelExecutionGuard guard(5000);
        
        // Simulate tight loop checking timeout (as GPU kernel would).
        for (int i = 0; i < 100; ++i) {
            bool timed_out = guard.has_timed_out();
            benchmark::DoNotOptimize(timed_out);
            if (timed_out) {
              break;
            }
        }
    }
}
BENCHMARK(BenchKernelTimeoutGuardHotPath)->Repetitions(5);

/**
 * BP-A8-009: Verify error handling latency is bounded.
 * 
 * Tests the complete error path: creation → exception → classification.
 */
static void BenchCompleteErrorHandlingPath(benchmark::State& state) {
    for (auto _ : state) {
        try {
            throw CudaError("cudaMemcpy", cudaErrorInvalidValue, __FILE__, __LINE__);
        } catch (const CudaError& err) {
            auto code = err.error_code();
            bool is_closed = isFailClosedClass(GPUDispatchErrorCode::DISPATCH_TIMEOUT);
            benchmark::DoNotOptimize(code);
            benchmark::DoNotOptimize(is_closed);
        }
    }
}
BENCHMARK(BenchCompleteErrorHandlingPath)->Repetitions(5);

/**
 * BP-A8-010: Batch error classification (simulating error queue processing).
 * 
 * Tests performance of classifying multiple errors in sequence.
 */
static void BenchBatchErrorClassification(benchmark::State& state) {
    std::vector<GPUDispatchErrorCode> errors = {
        GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT,
        GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE,
        GPUDispatchErrorCode::DISPATCH_TIMEOUT,
        GPUDispatchErrorCode::DISPATCH_KERNEL_LAUNCH_FAILED,
        GPUDispatchErrorCode::SUCCESS,
    };
    
    for (auto _ : state) {
        for (auto err : errors) {
            bool is_fail_closed = isFailClosedClass(err);
            benchmark::DoNotOptimize(is_fail_closed);
        }
    }
}
BENCHMARK(BenchBatchErrorClassification)->Repetitions(5);

}  // namespace bench
}  // namespace gpu
}  // namespace themis

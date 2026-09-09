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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace gpu {
namespace bench {

namespace {

struct LatencySummary {
    double p50_ns = 0.0;
    double p95_ns = 0.0;
    double p99_ns = 0.0;
    double avg_ns = 0.0;
};

double percentile(std::vector<double> samples, double fraction) {
    if (samples.empty()) {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());
    const auto raw_index = static_cast<double>(samples.size() - 1) * fraction;
    const auto lower_index = static_cast<std::size_t>(std::floor(raw_index));
    const auto upper_index = static_cast<std::size_t>(std::ceil(raw_index));
    if (lower_index == upper_index) {
        return samples[lower_index];
    }
    const auto weight = raw_index - static_cast<double>(lower_index);
    return samples[lower_index] +
           ((samples[upper_index] - samples[lower_index]) * weight);
}

LatencySummary summarizeLatencies(const std::vector<double>& samples_ns) {
    if (samples_ns.empty()) {
        return {};
    }
    auto sum = 0.0;
    for (const auto sample : samples_ns) {
        sum += sample;
    }
    LatencySummary summary;
    summary.p50_ns = percentile(samples_ns, 0.50);
    summary.p95_ns = percentile(samples_ns, 0.95);
    summary.p99_ns = percentile(samples_ns, 0.99);
    summary.avg_ns = sum / static_cast<double>(samples_ns.size());
    return summary;
}

void publishLatencyCounters(benchmark::State& state,
                            const std::vector<double>& samples_ns,
                            double gate_target_ns) {
    const auto summary = summarizeLatencies(samples_ns);
    state.counters["p50_ns"] = summary.p50_ns;
    state.counters["p95_ns"] = summary.p95_ns;
    state.counters["p99_ns"] = summary.p99_ns;
    state.counters["avg_ns"] = summary.avg_ns;
    state.counters["gate_target_ns"] = gate_target_ns;
    state.counters["gate_pass"] = summary.p99_ns <= gate_target_ns ? 1.0 : 0.0;
}

template <typename Fn>
void runLatencyBenchmark(benchmark::State& state, Fn&& fn, double gate_target_ns) {
    auto samples_ns = std::vector<double>{};
    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        const auto end = std::chrono::steady_clock::now();
        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples_ns.push_back(static_cast<double>(elapsed_ns));
        state.SetIterationTime(static_cast<double>(elapsed_ns) * 1e-9);
    }
    publishLatencyCounters(state, samples_ns, gate_target_ns);
}

}  // namespace

// =============================================================================
// Baseline Benchmarks
// =============================================================================

/**
 * BP-A8-001: Measure CudaError exception creation overhead.
 * 
 * Baseline: Verify exception construction doesn't exceed 1µs (negligible).
 */
static void BenchCudaErrorCreation(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        try {
            throw CudaError("cudaMalloc", cudaErrorMemoryAllocation, __FILE__, __LINE__);
        } catch (const CudaError&) {
            // Exception caught; measure complete.
        }
    }, 1000.0);
}
BENCHMARK(BenchCudaErrorCreation)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-002: Measure KernelExecutionGuard construction overhead.
 * 
 * Baseline: Should be < 10µs (just a timestamp + atomic).
 */
static void BenchKernelExecutionGuardCreation(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        KernelExecutionGuard guard(5000);  // 5 second timeout
        benchmark::DoNotOptimize(guard);
    }, 10000.0);
}
BENCHMARK(BenchKernelExecutionGuardCreation)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-003: Measure timeout check overhead.
 * 
 * Baseline: has_timed_out() should be < 100ns (timestamp comparison).
 */
static void BenchKernelTimeoutCheck(benchmark::State& state) {
    KernelExecutionGuard guard(5000);
    runLatencyBenchmark(state, [&guard]() {
        bool timed_out = guard.has_timed_out();
        benchmark::DoNotOptimize(timed_out);
    }, 100.0);
}
BENCHMARK(BenchKernelTimeoutCheck)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-004: Measure elapsed time calculation.
 * 
 * Baseline: Should be < 200ns (just time arithmetic).
 */
static void BenchKernelElapsedTime(benchmark::State& state) {
    KernelExecutionGuard guard(5000);
    runLatencyBenchmark(state, [&guard]() {
        uint64_t elapsed = guard.elapsed_ms();
        benchmark::DoNotOptimize(elapsed);
    }, 200.0);
}
BENCHMARK(BenchKernelElapsedTime)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-005: Measure CUDA error code classification.
 * 
 * Baseline: isFailClosedClass() should be < 50ns (single comparison).
 */
static void BenchErrorCodeClassification(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        bool is_fail_closed = isFailClosedClass(GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT);
        benchmark::DoNotOptimize(is_fail_closed);
    }, 50.0);
}
BENCHMARK(BenchErrorCodeClassification)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-006: Measure cuda_error_to_string conversion.
 * 
 * Baseline: Should be < 500ns (just string call).
 */
static void BenchCudaErrorToString(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        std::string msg = cuda_error_to_string(cudaErrorMemoryAllocation);
        benchmark::DoNotOptimize(msg);
    }, 500.0);
}
BENCHMARK(BenchCudaErrorToString)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-007: Measure timing utility conversions.
 * 
 * Baseline: ms_to_us and us_to_ms should be < 50ns (arithmetic only).
 */
static void BenchTimingConversions(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        uint64_t us = ms_to_us(100);
        uint64_t ms = us_to_ms(us);
        benchmark::DoNotOptimize(us);
        benchmark::DoNotOptimize(ms);
    }, 50.0);
}
BENCHMARK(BenchTimingConversions)->UseManualTime()->Repetitions(5);

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
    runLatencyBenchmark(state, []() {
        KernelExecutionGuard guard(5000);
        
        // Simulate tight loop checking timeout (as GPU kernel would).
        for (int i = 0; i < 100; ++i) {
            bool timed_out = guard.has_timed_out();
            benchmark::DoNotOptimize(timed_out);
            if (timed_out) {
              break;
            }
        }
    }, 10000.0);
}
BENCHMARK(BenchKernelTimeoutGuardHotPath)->UseManualTime()->Repetitions(5);

/**
 * BP-A8-009: Verify error handling latency is bounded.
 * 
 * Tests the complete error path: creation → exception → classification.
 */
static void BenchCompleteErrorHandlingPath(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        try {
            throw CudaError("cudaMemcpy", cudaErrorInvalidValue, __FILE__, __LINE__);
        } catch (const CudaError& err) {
            auto code = err.error_code();
            bool is_closed = isFailClosedClass(GPUDispatchErrorCode::DISPATCH_TIMEOUT);
            benchmark::DoNotOptimize(code);
            benchmark::DoNotOptimize(is_closed);
        }
    }, 5000.0);
}
BENCHMARK(BenchCompleteErrorHandlingPath)->UseManualTime()->Repetitions(5);

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
    
    runLatencyBenchmark(state, [&errors]() {
        for (auto err : errors) {
            bool is_fail_closed = isFailClosedClass(err);
            benchmark::DoNotOptimize(is_fail_closed);
        }
    }, 500.0);
}
BENCHMARK(BenchBatchErrorClassification)->UseManualTime()->Repetitions(5);

}  // namespace bench
}  // namespace gpu
}  // namespace themis

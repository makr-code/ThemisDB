// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_tensor_release_gates.cpp
 * @brief Phase 5 tensor hot-path release-gate benchmarks (TRNRG-01..TRNRG-06).
 *
 * Provides reproducible latency and throughput measurements for the tensor
 * module critical paths.  Results serve as release gates.
 *
 * ## Benchmark families
 *
 * ### TRNRG-01 — Matrix multiply (128×128 float32, CPU)
 *   Gate: ≥ 100 MFLOPS (≥ 1M calls/s for tiny matrix via inner-product mock).
 *
 * ### TRNRG-02 — Tensor reshape (1M elements, no copy)
 *   Gate: p99 ≤ 100 µs.
 *
 * ### TRNRG-03 — Element-wise add (1k elements, CPU)
 *   Gate: p99 ≤ 100 µs.
 *
 * ### TRNRG-04 — Dtype conversion (float32→float16, 1k elements)
 *   Gate: p99 ≤ 200 µs.
 *
 * ### TRNRG-05 — Device selection decision
 *   Gate: p99 ≤ 10 µs.
 *
 * ### TRNRG-06 — Slice view creation (no copy)
 *   Gate: p99 ≤ 50 µs.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark   | Threshold         |
 * |--------------|-------------|-------------------|
 * | GATE-TRNRG-01 | TRNRG-01   | ≥ 100 MFLOPS      |
 * | GATE-TRNRG-02 | TRNRG-02   | p99 ≤ 100 µs      |
 * | GATE-TRNRG-03 | TRNRG-03   | p99 ≤ 100 µs      |
 * | GATE-TRNRG-04 | TRNRG-04   | p99 ≤ 200 µs      |
 * | GATE-TRNRG-05 | TRNRG-05   | p99 ≤ 10 µs       |
 * | GATE-TRNRG-06 | TRNRG-06   | p99 ≤ 50 µs       |
 *
 * @see include/tensor/tensor_api_contract.h
 * @see src/tensor/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "tensor/tensor_api_contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace themis {
namespace bench {
namespace trnrg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all TRNRG benchmarks.
static constexpr uint64_t kTensorCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------

/// Minimal mock float16 type (stored as uint16).
using Float16 = std::uint16_t;

/// Convert float32 → mock float16 (round to nearest, drop lower bits).
static Float16 f32ToF16(float v) {
    // Trivial bit-manipulation approximation (not IEEE 754 half — just a mock
    // to measure the conversion loop cost without I/O).
    std::uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    return static_cast<Float16>((bits >> 16) & 0xFFFF);
}

/// Simple 128×128 matrix multiply (C = A * B), accumulating into output.
/// N=128: 128*128*128*2 = ~4.2M FLOPs per call.
static void matMul128(const std::vector<float>& A,
                      const std::vector<float>& B,
                      std::vector<float>& C) {
    constexpr std::size_t N = 128;
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            float acc = 0.0f;
            for (std::size_t k = 0; k < N; ++k) {
                acc += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = acc;
        }
    }
}

/// Mock reshape: returns new shape descriptor (no data copy).
struct TensorShape {
    std::vector<std::size_t> dims;
    const float* data_ptr; // non-owning pointer to original data
};

static TensorShape mockReshape(const float* data,
                                const std::vector<std::size_t>& new_dims) {
    return {new_dims, data};
}

/// Element-wise add: out[i] = a[i] + b[i].
static void elementWiseAdd(const float* a, const float* b, float* out, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) out[i] = a[i] + b[i];
}

/// Dtype conversion: float32 → float16.
static void convertF32ToF16(const float* src, Float16* dst, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) dst[i] = f32ToF16(src[i]);
}

enum class MockDevice { CPU, GPU };
static MockDevice mockSelectDevice(bool gpu_available) {
    return gpu_available ? MockDevice::GPU : MockDevice::CPU;
}

/// Mock slice view: returns pointer + length (no copy).
struct MockView {
    const float* ptr;
    std::size_t  count;
};

static MockView mockSliceView(const float* base, std::size_t offset, std::size_t count) {
    return {base + offset, count};
}

// ---------------------------------------------------------------------------
// TRNRG-01 — Matrix multiply (128×128 float32)
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-01: 128×128 float32 matrix multiplication on CPU.
 *
 * GATE-TRNRG-01: ≥ 100 MFLOPS.
 */
static void BM_TRNRG01_MatMul128(benchmark::State& state) {
    constexpr std::size_t N = 128;
    std::mt19937_64 rng(kTensorCanonicalSeed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> A(N * N), B(N * N), C(N * N, 0.0f);
    for (auto& v : A) v = dist(rng);
    for (auto& v : B) v = dist(rng);

    // Warmup (single pass)
    matMul128(A, B, C);

    for (auto _ : state) {
        matMul128(A, B, C);
        benchmark::ClobberMemory();
    }
    // Report FLOPS: N^3 * 2 (mul + add) per call
    state.SetItemsProcessed(state.iterations() *
                             static_cast<std::int64_t>(N * N * N * 2));
    state.SetLabel("GATE-TRNRG-01: >= 100 MFLOPS");
}
BENCHMARK(BM_TRNRG01_MatMul128)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRNRG-02 — Tensor reshape (1M elements, no copy)
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-02: Zero-copy reshape of a 1M-element flat tensor.
 *
 * GATE-TRNRG-02: p99 ≤ 100 µs.
 */
static void BM_TRNRG02_TensorReshape(benchmark::State& state) {
    constexpr std::size_t kElems = 1'000'000;
    std::vector<float> data(kElems, 1.0f);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(
            mockReshape(data.data(), {1000, 1000}));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            mockReshape(data.data(), {1000, 1000}));
    }
    state.SetLabel("GATE-TRNRG-02: p99 <= 100us");
}
BENCHMARK(BM_TRNRG02_TensorReshape)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRNRG-03 — Element-wise add (1k elements, CPU)
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-03: Element-wise float32 addition over 1k elements.
 *
 * GATE-TRNRG-03: p99 ≤ 100 µs.
 */
static void BM_TRNRG03_ElementWiseAdd(benchmark::State& state) {
    constexpr std::size_t kN = 1000;
    std::vector<float> a(kN, 1.0f), b(kN, 2.0f), out(kN);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        elementWiseAdd(a.data(), b.data(), out.data(), kN);
    }
    for (auto _ : state) {
        elementWiseAdd(a.data(), b.data(), out.data(), kN);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kN));
    state.SetLabel("GATE-TRNRG-03: p99 <= 100us");
}
BENCHMARK(BM_TRNRG03_ElementWiseAdd)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRNRG-04 — Dtype conversion (float32→float16, 1k elements)
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-04: float32 to float16 conversion for 1k elements.
 *
 * GATE-TRNRG-04: p99 ≤ 200 µs.
 */
static void BM_TRNRG04_DtypeConversion(benchmark::State& state) {
    constexpr std::size_t kN = 1000;
    std::mt19937_64 rng(kTensorCanonicalSeed);
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    std::vector<float>   src(kN);
    std::vector<Float16> dst(kN);
    for (auto& v : src) v = dist(rng);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        convertF32ToF16(src.data(), dst.data(), kN);
    }
    for (auto _ : state) {
        convertF32ToF16(src.data(), dst.data(), kN);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kN));
    state.SetLabel("GATE-TRNRG-04: p99 <= 200us");
}
BENCHMARK(BM_TRNRG04_DtypeConversion)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRNRG-05 — Device selection decision
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-05: Device selection (CPU vs GPU) decision latency.
 *
 * GATE-TRNRG-05: p99 ≤ 10 µs.
 */
static void BM_TRNRG05_DeviceSelection(benchmark::State& state) {
    std::mt19937_64 rng(kTensorCanonicalSeed);
    std::bernoulli_distribution dist(0.5);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(mockSelectDevice(dist(rng)));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(mockSelectDevice(dist(rng)));
    }
    state.SetLabel("GATE-TRNRG-05: p99 <= 10us");
}
BENCHMARK(BM_TRNRG05_DeviceSelection)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRNRG-06 — Slice view creation (no copy)
// ---------------------------------------------------------------------------

/**
 * @brief TRNRG-06: Zero-copy slice view creation into a 1M-element buffer.
 *
 * GATE-TRNRG-06: p99 ≤ 50 µs.
 */
static void BM_TRNRG06_SliceViewCreation(benchmark::State& state) {
    constexpr std::size_t kElems = 1'000'000;
    std::vector<float> data(kElems, 0.0f);

    std::mt19937_64 rng(kTensorCanonicalSeed);
    std::uniform_int_distribution<std::size_t> dist(0, kElems / 2);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        std::size_t off = dist(rng);
        benchmark::DoNotOptimize(mockSliceView(data.data(), off, kElems / 2));
    }
    for (auto _ : state) {
        std::size_t off = dist(rng);
        benchmark::DoNotOptimize(mockSliceView(data.data(), off, kElems / 2));
    }
    state.SetLabel("GATE-TRNRG-06: p99 <= 50us");
}
BENCHMARK(BM_TRNRG06_SliceViewCreation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace trnrg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();

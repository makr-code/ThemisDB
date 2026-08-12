#include <benchmark/benchmark.h>

#include "acceleration/cpu_backend.h"
#include "acceleration/kernel_invocation.h"
#include "themis/gpu/query_accelerator.h"
#include "utils/simd_distance.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace themis::acceleration;
using namespace themis::gpu;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Returns a float buffer of @p count elements drawn from N(0,1).
static std::vector<float> makeNormal(std::size_t count, std::uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> v(count);
    for (auto& x : v) {
        x = dist(rng);
    }
    return v;
}

/// Row-normalises @p rows vectors of dimension @p dim in-place.
static void rowNormalize(float* data, std::size_t rows, std::size_t dim) {
    for (std::size_t r = 0; r < rows; ++r) {
        float* base = data + r * dim;
        float sq_sum = 0.0f;
        for (std::size_t d = 0; d < dim; ++d) {
            sq_sum += base[d] * base[d];
        }
        const float inv = 1.0f / std::sqrt(std::max(sq_sum, 1e-12f));
        for (std::size_t d = 0; d < dim; ++d) {
            base[d] *= inv;
        }
    }
}

/// Scalar cosine distance between two unit-normalised vectors.
static float scalarCosine(const float* a, const float* b, std::size_t dim) {
    float dot = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
    }
    return 1.0f - dot;
}

/// Scalar Frobenius norm (sum of squares) of a vector interpreted as a flat tensor.
static float frobeniusNorm(const float* data, std::size_t n) {
    float sq = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        sq += data[i] * data[i];
    }
    return std::sqrt(sq);
}

/// Naive matrix multiply C = A(M×K) × B(K×N) stored row-major.
static void scalarGemm(const float* A, const float* B, float* C,
                        std::size_t M, std::size_t K, std::size_t N) {
    std::fill(C, C + M * N, 0.0f);
    for (std::size_t m = 0; m < M; ++m) {
        for (std::size_t k = 0; k < K; ++k) {
            const float a = A[m * K + k];
            for (std::size_t n = 0; n < N; ++n) {
                C[m * N + n] += a * B[k * N + n];
            }
        }
    }
}

/// Returns a singleton (default-initialized) CPUVectorBackend.
static CPUVectorBackend& cpuVecBackend() {
    static CPUVectorBackend backend;
    static const bool init = backend.initialize();
    (void)init;
    return backend;
}

/// Returns a singleton CPUMatrixBackend.
static CPUMatrixBackend& cpuMatBackend() {
    static CPUMatrixBackend backend;
    static const bool init = backend.initialize();
    (void)init;
    return backend;
}

} // namespace

// ============================================================================
// TEN-S1 — Cosine Similarity: CPU Scalar vs CPU SIMD (CPUVectorBackend)
//
// Measures the scalar reference path vs the CPUVectorBackend batch path for
// cosine similarity.  Sweep: rank (vector dimension) ∈ {4, 8, 16, 32, 64}.
// ============================================================================

static void BM_TEN_S1_CosineScalar(benchmark::State& state) {
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const std::size_t batch = static_cast<std::size_t>(state.range(1));

    auto queries = makeNormal(batch * dim, 1001);
    auto database = makeNormal(batch * dim, 1002);
    rowNormalize(queries.data(), batch, dim);
    rowNormalize(database.data(), batch, dim);

    for (auto _ : state) {
        float sum = 0.0f;
        for (std::size_t qi = 0; qi < batch; ++qi) {
            for (std::size_t di = 0; di < batch; ++di) {
                sum += scalarCosine(queries.data() + qi * dim, database.data() + di * dim, dim);
            }
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch * batch));
    state.counters["dim"] = static_cast<double>(dim);
    state.counters["path"] = 0; // scalar
}

BENCHMARK(BM_TEN_S1_CosineScalar)
    ->Args({4,   32})
    ->Args({8,   32})
    ->Args({16,  32})
    ->Args({32,  32})
    ->Args({64,  32})
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_S1_CosineSIMD(benchmark::State& state) {
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const std::size_t batch = static_cast<std::size_t>(state.range(1));

    auto queries = makeNormal(batch * dim, 2001);
    auto database = makeNormal(batch * dim, 2002);
    rowNormalize(queries.data(), batch, dim);
    rowNormalize(database.data(), batch, dim);

    auto& backend = cpuVecBackend();

    for (auto _ : state) {
        // batchKnnSearch with k=1 measures the distance computation path.
        auto result = backend.batchKnnSearch(
            queries.data(), batch, dim,
            database.data(), batch, /*k=*/1, /*useL2=*/false);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch * batch));
    state.counters["dim"] = static_cast<double>(dim);
    state.counters["path"] = 1; // SIMD
}

BENCHMARK(BM_TEN_S1_CosineSIMD)
    ->Args({4,   32})
    ->Args({8,   32})
    ->Args({16,  32})
    ->Args({32,  32})
    ->Args({64,  32})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-S2 — Frobenius Norm: CPU Scalar vs GPU Dispatch
//
// Frobenius norm of a flat tensor core.  GPU path uses GPUQueryAccelerator
// dotProduct(v, v) as a proxy (sqrt of the result = Frobenius norm).
// Sweep: rank × dim combinations.
// ============================================================================

static void BM_TEN_S2_FrobeniusScalar(benchmark::State& state) {
    const std::size_t elems = static_cast<std::size_t>(state.range(0));
    const auto data = makeNormal(elems, 3001);

    for (auto _ : state) {
        auto norm = frobeniusNorm(data.data(), elems);
        benchmark::DoNotOptimize(norm);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["elems"] = static_cast<double>(elems);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_S2_FrobeniusScalar)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_S2_FrobeniusGPUDispatch(benchmark::State& state) {
    const std::size_t elems = static_cast<std::size_t>(state.range(0));

    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 4096;
    GPUQueryAccelerator accel(cfg);

    const auto data = makeNormal(elems, 3002);

    bool last_used_gpu = false;
    for (auto _ : state) {
        // dotProduct(v, v) = ||v||^2; sqrt gives Frobenius norm.
        auto result = accel.dotProduct(data, data);
        const float norm = std::sqrt(result.value);
        last_used_gpu = result.used_gpu;
        benchmark::DoNotOptimize(norm);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["elems"] = static_cast<double>(elems);
    state.counters["path"] = 1;
    state.counters["used_gpu"] = last_used_gpu ? 1.0 : 0.0;
}

BENCHMARK(BM_TEN_S2_FrobeniusGPUDispatch)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-S3 — Dot-Product Contraction: CPU SIMD vs GPU
//
// Inner-product of two flat vectors as a proxy for a rank-1 tensor contraction.
// Sweep: elems (= rank × dim).
// ============================================================================

static void BM_TEN_S3_DotProductCPU(benchmark::State& state) {
    const std::size_t elems = static_cast<std::size_t>(state.range(0));
    const auto a = makeNormal(elems, 4001);
    const auto b = makeNormal(elems, 4002);

    for (auto _ : state) {
        const float result = themis::simd::inner_product(a.data(), b.data(), elems);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["elems"] = static_cast<double>(elems);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_S3_DotProductCPU)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_S3_DotProductGPU(benchmark::State& state) {
    const std::size_t elems = static_cast<std::size_t>(state.range(0));

    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 4096;
    GPUQueryAccelerator accel(cfg);

    const auto a = makeNormal(elems, 5001);
    const auto b = makeNormal(elems, 5002);

    bool last_used_gpu = false;
    for (auto _ : state) {
        auto result = accel.dotProduct(a, b);
        last_used_gpu = result.used_gpu;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["elems"] = static_cast<double>(elems);
    state.counters["path"] = 1;
    state.counters["used_gpu"] = last_used_gpu ? 1.0 : 0.0;
}

BENCHMARK(BM_TEN_S3_DotProductGPU)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-S4 — Batch Similarity Scoring: CPU SIMD vs GPU
//
// Batch ANN search (top-k) as proxy for batch tensor similarity scoring.
// Sweep: num_vectors × dim × batch_size.
// ============================================================================

static void BM_TEN_S4_BatchSimilarityCPU(benchmark::State& state) {
    const std::size_t num_vectors = static_cast<std::size_t>(state.range(0));
    const std::size_t dim         = static_cast<std::size_t>(state.range(1));
    const std::size_t batch       = static_cast<std::size_t>(state.range(2));
    const std::size_t top_k       = 5;

    auto queries  = makeNormal(batch * dim, 6001);
    auto database = makeNormal(num_vectors * dim, 6002);
    rowNormalize(queries.data(), batch, dim);
    rowNormalize(database.data(), num_vectors, dim);

    auto& backend = cpuVecBackend();

    for (auto _ : state) {
        auto result = backend.batchKnnSearch(
            queries.data(), batch, dim,
            database.data(), num_vectors, top_k, /*useL2=*/false);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch * num_vectors));
    state.counters["num_vectors"] = static_cast<double>(num_vectors);
    state.counters["batch"] = static_cast<double>(batch);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_S4_BatchSimilarityCPU)
    ->Args({256,  64,  1})
    ->Args({256,  64,  8})
    ->Args({1024, 64,  8})
    ->Args({1024, 64,  32})
    ->Args({4096, 128, 32})
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_S4_BatchSimilarityGPU(benchmark::State& state) {
    const std::size_t num_vectors = static_cast<std::size_t>(state.range(0));
    const std::size_t dim         = static_cast<std::size_t>(state.range(1));
    const std::size_t batch       = static_cast<std::size_t>(state.range(2));
    const std::size_t top_k       = 5;

    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 4096;
    GPUQueryAccelerator accel(cfg);

    auto queries  = makeNormal(batch * dim, 7001);
    auto database = makeNormal(num_vectors * dim, 7002);
    rowNormalize(queries.data(), batch, dim);
    rowNormalize(database.data(), num_vectors, dim);

    bool last_used_gpu = false;
    for (auto _ : state) {
        auto result = accel.annSearch(queries, batch, dim, database, num_vectors, top_k, false);
        last_used_gpu = result.used_gpu;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch * num_vectors));
    state.counters["num_vectors"] = static_cast<double>(num_vectors);
    state.counters["batch"] = static_cast<double>(batch);
    state.counters["path"] = 1;
    state.counters["used_gpu"] = last_used_gpu ? 1.0 : 0.0;
}

BENCHMARK(BM_TEN_S4_BatchSimilarityGPU)
    ->Args({256,  64,  1})
    ->Args({256,  64,  8})
    ->Args({1024, 64,  8})
    ->Args({1024, 64,  32})
    ->Args({4096, 128, 32})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-G1 — GEMM Contraction: CPUMatrixBackend vs GPU dotProduct chain
//
// Dense matrix multiply as proxy for full tensor contraction (core × core).
// Sweep: matrix_dim ∈ {64, 128, 256, 512}.
// ============================================================================

static void BM_TEN_G1_GemmScalar(benchmark::State& state) {
    const std::size_t M = static_cast<std::size_t>(state.range(0));
    const std::size_t K = M;
    const std::size_t N = M;

    auto A = makeNormal(M * K, 8001);
    auto B = makeNormal(K * N, 8002);
    std::vector<float> C(M * N, 0.0f);

    for (auto _ : state) {
        scalarGemm(A.data(), B.data(), C.data(), M, K, N);
        benchmark::DoNotOptimize(C);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(2) * M * K * N);
    state.counters["dim"] = static_cast<double>(M);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_G1_GemmScalar)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_G1_GemmCPUBackend(benchmark::State& state) {
    const std::size_t M = static_cast<std::size_t>(state.range(0));
    const std::size_t K = M;
    const std::size_t N = M;

    auto A = makeNormal(M * K, 9001);
    auto B = makeNormal(K * N, 9002);
    std::vector<float> C(M * N, 0.0f);

    auto& backend = cpuMatBackend();

    MatrixKernelParams params;
    params.A     = A.data();
    params.B     = B.data();
    params.C     = C.data();
    params.M     = M;
    params.K     = K;
    params.N     = N;
    params.alpha = 1.0f;
    params.beta  = 0.0f;
    params.precision = MatrixPrecision::FP32;

    for (auto _ : state) {
        (void)backend.matmul(params);
        benchmark::DoNotOptimize(C);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(2) * M * K * N);
    state.counters["dim"] = static_cast<double>(M);
    state.counters["path"] = 1;
}

BENCHMARK(BM_TEN_G1_GemmCPUBackend)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-G2 — Shard Relevance Scoring: Batch Dot-Product Sweep
//
// Each shard contributes a relevance vector; GPU accelerator scores all shards
// against a query in one batch call.  Sweep: shard_count × batch.
// ============================================================================

static void BM_TEN_G2_ShardScoringCPU(benchmark::State& state) {
    const std::size_t shard_count = static_cast<std::size_t>(state.range(0));
    const std::size_t dim         = 128;

    auto queries = makeNormal(dim, 10001);
    auto shards  = makeNormal(shard_count * dim, 10002);

    auto& backend = cpuVecBackend();

    for (auto _ : state) {
        auto result = backend.batchKnnSearch(
            queries.data(), 1, dim,
            shards.data(), shard_count, shard_count, /*useL2=*/false);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(shard_count));
    state.counters["shard_count"] = static_cast<double>(shard_count);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_G2_ShardScoringCPU)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_G2_ShardScoringGPU(benchmark::State& state) {
    const std::size_t shard_count = static_cast<std::size_t>(state.range(0));
    const std::size_t dim         = 128;

    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 4096;
    GPUQueryAccelerator accel(cfg);

    auto queries = makeNormal(dim, 11001);
    auto shards  = makeNormal(shard_count * dim, 11002);

    for (auto _ : state) {
        auto result = accel.annSearch(queries, 1, dim, shards, shard_count, shard_count, false);
        benchmark::DoNotOptimize(result);
    }

    const auto stats = accel.getStats();
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(shard_count));
    state.counters["shard_count"] = static_cast<double>(shard_count);
    state.counters["path"] = 1;
    state.counters["used_gpu"] = stats.gpu_ops > 0 ? 1.0 : 0.0;
}

BENCHMARK(BM_TEN_G2_ShardScoringGPU)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-G3 — Summary Generation (Mean-Pool): CPU Scalar
//
// Mean-pool of N tensors (each of dimension @p dim) as proxy for adapter
// summary generation.  GPU path would use a thrust::reduce; CPU is scalar.
// ============================================================================

static void BM_TEN_G3_MeanPoolScalar(benchmark::State& state) {
    const std::size_t tensor_count = static_cast<std::size_t>(state.range(0));
    const std::size_t dim          = 128;

    const auto data = makeNormal(tensor_count * dim, 12001);
    std::vector<float> mean(dim, 0.0f);

    for (auto _ : state) {
        std::fill(mean.begin(), mean.end(), 0.0f);
        for (std::size_t i = 0; i < tensor_count; ++i) {
            const float* src = data.data() + i * dim;
            for (std::size_t d = 0; d < dim; ++d) {
                mean[d] += src[d];
            }
        }
        const float inv = 1.0f / static_cast<float>(tensor_count);
        for (float& v : mean) {
            v *= inv;
        }
        benchmark::DoNotOptimize(mean);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(tensor_count * dim));
    state.counters["tensor_count"] = static_cast<double>(tensor_count);
}

BENCHMARK(BM_TEN_G3_MeanPoolScalar)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-G4 — Residual/Error Accumulation: CPU vs GPU
//
// Squared residual ||a - b||^2 as proxy for reconstruction error tracking in
// a TT decomposition pipeline.  Sweep: rank ∈ {4, 8, 16, 32, 64}.
// ============================================================================

static void BM_TEN_G4_ResidualCPU(benchmark::State& state) {
    const std::size_t rank = static_cast<std::size_t>(state.range(0));
    const std::size_t dim  = 128;
    const std::size_t elems = rank * dim;

    const auto a = makeNormal(elems, 13001);
    const auto b = makeNormal(elems, 13002);

    auto& backend = cpuVecBackend();

    for (auto _ : state) {
        // L2 distance = squared residual norm (without sqrt).
        auto result = backend.batchKnnSearch(
            a.data(), 1, elems,
            b.data(), 1, 1, /*useL2=*/true);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["rank"] = static_cast<double>(rank);
    state.counters["path"] = 0;
}

BENCHMARK(BM_TEN_G4_ResidualCPU)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_G4_ResidualGPU(benchmark::State& state) {
    const std::size_t rank = static_cast<std::size_t>(state.range(0));
    const std::size_t dim  = 128;
    const std::size_t elems = rank * dim;

    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 4096;
    GPUQueryAccelerator accel(cfg);

    const auto a = makeNormal(elems, 14001);
    const auto b = makeNormal(elems, 14002);
    // residual = ||a||^2 + ||b||^2 - 2*dot(a,b); approximate via dotProduct(diff, diff)
    std::vector<float> diff(elems);
    for (std::size_t i = 0; i < elems; ++i) {
        diff[i] = a[i] - b[i];
    }

    for (auto _ : state) {
        auto result = accel.dotProduct(diff, diff);
        benchmark::DoNotOptimize(result);
    }

    const auto stats = accel.getStats();
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elems));
    state.counters["rank"] = static_cast<double>(rank);
    state.counters["path"] = 1;
    state.counters["used_gpu"] = stats.gpu_ops > 0 ? 1.0 : 0.0;
}

BENCHMARK(BM_TEN_G4_ResidualGPU)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-M1 — mmap Cold vs Warm Page-Cache Simulation
//
// STUB/SIMULATION NOTE:
// Purpose: Provide stable CI-safe benchmark coverage for TEN-M1 without OS-specific mmap setup.
// Activation: Always active in benchmark-only binary bench_tensor_cpu_gpu_dispatch.
// Production Delta: Uses allocation/memcpy timing instead of true mmap/page-cache behavior.
// Removal Plan: Replace with real mmap-backed benchmark implementation in benchmark matrix phase 2.
//
// Simulates cold-cache latency (first-access allocation + copy) vs warm-cache
// latency (already-hot buffer read) for a tensor shard resident on disk.
// In production this would touch mmap'd pages; here we use std::vector
// allocation + memcpy to model the two scenarios.
// ============================================================================

static void BM_TEN_M1_MmapCold(benchmark::State& state) {
    const std::size_t bytes = static_cast<std::size_t>(state.range(0)) * 1024ULL;

    for (auto _ : state) {
        // Simulate cold page fault: allocate + zero (forces page mapping).
        std::vector<std::uint8_t> buf(bytes, 0);
        benchmark::DoNotOptimize(buf.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
    state.counters["kb"] = static_cast<double>(state.range(0));
    state.counters["cache"] = 0; // cold
}

BENCHMARK(BM_TEN_M1_MmapCold)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

static void BM_TEN_M1_MmapWarm(benchmark::State& state) {
    const std::size_t bytes = static_cast<std::size_t>(state.range(0)) * 1024ULL;

    // Pre-warm: allocate outside the loop.
    std::vector<std::uint8_t> warm_buf(bytes, 0xFF);
    std::vector<std::uint8_t> dst(bytes);

    for (auto _ : state) {
        // Warm access: buffer already resident.
        std::memcpy(dst.data(), warm_buf.data(), bytes);
        benchmark::DoNotOptimize(dst.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
    state.counters["kb"] = static_cast<double>(state.range(0));
    state.counters["cache"] = 1; // warm
}

BENCHMARK(BM_TEN_M1_MmapWarm)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TEN-M2 — mmap → CPU SIMD End-to-End Pipeline
//
// Models the full tensor retrieval pipeline assuming warm mmap access:
//   1. Load shard vectors from a warm buffer (memcpy)
//   2. Run CPU SIMD batch similarity scoring
// ============================================================================

static void BM_TEN_M2_MmapToCPUSIMD(benchmark::State& state) {
    const std::size_t shard_vecs = static_cast<std::size_t>(state.range(0));
    const std::size_t dim        = 128;

    // Simulate the mmap'd shard data (warm).
    const auto shard_source = makeNormal(shard_vecs * dim, 15001);
    const auto query        = makeNormal(dim, 15002);
    std::vector<float> loaded(shard_vecs * dim);

    auto& backend = cpuVecBackend();

    for (auto _ : state) {
        // Step 1: warm mmap read (memcpy models page-resident access).
        std::memcpy(loaded.data(), shard_source.data(), shard_vecs * dim * sizeof(float));
        benchmark::ClobberMemory();

        // Step 2: CPU SIMD similarity search.
        auto result = backend.batchKnnSearch(
            query.data(), 1, dim,
            loaded.data(), shard_vecs, 10, /*useL2=*/false);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(shard_vecs));
    state.counters["shard_vecs"] = static_cast<double>(shard_vecs);
}

BENCHMARK(BM_TEN_M2_MmapToCPUSIMD)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_Smoke_* — Fast sanity targets for CI (≤ 5 s combined, no GPU required)
// ============================================================================

static void BM_Smoke_TensorCosine(benchmark::State& state) {
    const auto a = makeNormal(64, 20001);
    const auto b = makeNormal(64, 20002);
    for (auto _ : state) {
        auto dist = scalarCosine(a.data(), b.data(), 64);
        benchmark::DoNotOptimize(dist);
    }
}
BENCHMARK(BM_Smoke_TensorCosine)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_TensorSIMDBatch(benchmark::State& state) {
    auto& backend = cpuVecBackend();
    const auto q = makeNormal(4 * 64, 20003);
    const auto d = makeNormal(256 * 64, 20004);
    for (auto _ : state) {
        auto result = backend.batchKnnSearch(q.data(), 4, 64, d.data(), 256, 5, false);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Smoke_TensorSIMDBatch)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_TensorGPUDispatch(benchmark::State& state) {
    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 128;
    GPUQueryAccelerator accel(cfg);
    const auto q = makeNormal(4 * 64, 20005);
    const auto d = makeNormal(256 * 64, 20006);
    for (auto _ : state) {
        auto result = accel.annSearch(q, 4, 64, d, 256, 5, false);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Smoke_TensorGPUDispatch)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_TensorGemm(benchmark::State& state) {
    auto& backend = cpuMatBackend();
    std::vector<float> A(32 * 32, 1.0f);
    std::vector<float> B(32 * 32, 1.0f);
    std::vector<float> C(32 * 32, 0.0f);
    MatrixKernelParams params;
    params.A = A.data();
    params.B = B.data();
    params.C = C.data();
    params.M = params.K = params.N = 32;
    params.alpha = 1.0f;
    params.beta  = 0.0f;
    params.precision = MatrixPrecision::FP32;
    for (auto _ : state) {
        (void)backend.matmul(params);
        benchmark::DoNotOptimize(C);
    }
}
BENCHMARK(BM_Smoke_TensorGemm)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_TensorMmap(benchmark::State& state) {
    const std::size_t bytes = 64 * 1024;
    std::vector<std::uint8_t> warm(bytes, 0xAA);
    std::vector<std::uint8_t> dst(bytes);
    for (auto _ : state) {
        std::memcpy(dst.data(), warm.data(), bytes);
        benchmark::DoNotOptimize(dst.data());
    }
}
BENCHMARK(BM_Smoke_TensorMmap)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

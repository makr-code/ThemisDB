#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <cmath>
#include <string>

#include "utils/simd_distance.h"

namespace {
struct RandGen {
    static std::vector<float> gen(size_t dim, std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::vector<float> v(dim);
        for (size_t i = 0; i < dim; ++i) v[i] = dist(rng);
        return v;
    }
};

float scalar_l2(const float* a, const float* b, size_t dim) {
    float s = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}
} // namespace

// Benchmark: SIMD vs. Scalar L2 (distance only; ordering semantics unchanged)
// Args: {dimension}
static void BM_SIMD_L2(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(42);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float d = themis::simd::l2_distance(q.data(), v.data(), dim);
        benchmark::DoNotOptimize(d);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

static void BM_Scalar_L2(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(1337);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float d = scalar_l2(q.data(), v.data(), dim);
        benchmark::DoNotOptimize(d);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

BENCHMARK(BM_SIMD_L2)->Args({64})->Args({128})->Args({256})->Args({512})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Scalar_L2)->Args({64})->Args({128})->Args({256})->Args({512})->Unit(benchmark::kMicrosecond);

// ── Inner Product ────────────────────────────────────────────────────────────

static void BM_SIMD_InnerProduct(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(99);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float d = themis::simd::inner_product(q.data(), v.data(), dim);
        benchmark::DoNotOptimize(d);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

static void BM_Scalar_InnerProduct(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(999);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float acc = 0.0f;
        for (size_t i = 0; i < dim; ++i) acc += q[i] * v[i];
        benchmark::DoNotOptimize(acc);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

BENCHMARK(BM_SIMD_InnerProduct)->Args({64})->Args({128})->Args({256})->Args({512})->Args({1536})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Scalar_InnerProduct)->Args({64})->Args({128})->Args({256})->Args({512})->Args({1536})->Unit(benchmark::kMicrosecond);

// ── Cosine Distance ──────────────────────────────────────────────────────────

static void BM_SIMD_CosineDistance(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(77);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float d = themis::simd::cosine_distance(q.data(), v.data(), dim);
        benchmark::DoNotOptimize(d);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

static void BM_Scalar_CosineDistance(benchmark::State& state) {
    size_t dim = static_cast<size_t>(state.range(0));
    std::mt19937 rng(777);
    auto q = RandGen::gen(dim, rng);
    auto v = RandGen::gen(dim, rng);
    for (auto _ : state) {
        float dot = 0.0f, na = 0.0f, nb = 0.0f;
        for (size_t i = 0; i < dim; ++i) {
            dot += q[i] * v[i];
            na  += q[i] * q[i];
            nb  += v[i] * v[i];
        }
        float denom = std::sqrt(na * nb);
        float d = (denom < 1e-10f) ? 1.0f : (1.0f - dot / denom);
        benchmark::DoNotOptimize(d);
    }
    state.counters["dim"] = static_cast<double>(dim);
}

BENCHMARK(BM_SIMD_CosineDistance)->Args({64})->Args({128})->Args({256})->Args({512})->Args({1536})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Scalar_CosineDistance)->Args({64})->Args({128})->Args({256})->Args({512})->Args({1536})->Unit(benchmark::kMicrosecond);

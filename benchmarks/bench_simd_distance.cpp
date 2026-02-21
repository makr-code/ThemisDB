/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_simd_distance.cpp                            ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

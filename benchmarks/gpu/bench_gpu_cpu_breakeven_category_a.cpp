// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file bench_gpu_cpu_breakeven_category_a.cpp
/// @brief Phase D CPU baseline benchmarks for GPU break-even Category A kernels.
///
/// Category A covers distance and Top-K style kernels that require >= 1.5x GPU
/// speedup before the optional GPU path can be enabled. This benchmark records
/// deterministic CPU reference timings only; final GPU comparison remains gated
/// on 2027 hardware availability.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int kVectorDimension = 128;
constexpr int kTopK            = 32;
constexpr uint32_t kSeed       = 42u;

std::vector<float> makeVectorCorpus(int count) {
    std::mt19937 rng(kSeed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> corpus(static_cast<std::size_t>(count) * kVectorDimension);
    for (float& value : corpus) {
        value = dist(rng);
    }
    return corpus;
}

std::vector<float> makeQueryVector() {
    std::mt19937 rng(kSeed + 1u);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> query(kVectorDimension);
    for (float& value : query) {
        value = dist(rng);
    }
    return query;
}

static void BGPU_CategoryA_Distance_CPUBaseline(benchmark::State& state) {
    const int vector_count = static_cast<int>(state.range(0));
    const auto corpus      = makeVectorCorpus(vector_count);
    const auto query       = makeQueryVector();
    std::vector<float> distances(static_cast<std::size_t>(vector_count), 0.0f);

    for (auto _ : state) {
        for (int row = 0; row < vector_count; ++row) {
            const std::size_t offset =
                static_cast<std::size_t>(row) * kVectorDimension;
            float squared_l2 = 0.0f;
            for (int dim = 0; dim < kVectorDimension; ++dim) {
                const float delta = corpus[offset + static_cast<std::size_t>(dim)] -
                                    query[static_cast<std::size_t>(dim)];
                squared_l2 += delta * delta;
            }
            distances[static_cast<std::size_t>(row)] = squared_l2;
        }
        benchmark::DoNotOptimize(distances);
    }

    state.SetItemsProcessed(state.iterations() * vector_count);
    state.counters["gate_speedup_x100"] = 150;
    state.counters["dimension"]         = kVectorDimension;
    state.SetLabel(
        "Phase D CPU baseline | Category A distance | GPU review pending 2027");
}
BENCHMARK(BGPU_CategoryA_Distance_CPUBaseline)
    ->Arg(1'024)
    ->Arg(4'096)
    ->Arg(16'384)
    ->Arg(65'536)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

static void BGPU_CategoryA_TopK_CPUBaseline(benchmark::State& state) {
    const int score_count = static_cast<int>(state.range(0));
    std::mt19937 rng(kSeed + 2u);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<float> scores(static_cast<std::size_t>(score_count));
    for (float& score : scores) {
        score = dist(rng);
    }

    std::vector<float> working = {};

    working.reserve(scores.size());

    for (auto _ : state) {
        working = scores;
        std::partial_sort(working.begin(),
                          working.begin() + std::min<int>(kTopK, score_count),
                          working.end(),
                          std::greater<float>());
        benchmark::DoNotOptimize(working.data());
    }

    state.SetItemsProcessed(state.iterations() * score_count);
    state.counters["gate_speedup_x100"] = 150;
    state.counters["top_k"]             = kTopK;
    state.SetLabel(
        "Phase D CPU baseline | Category A topk | GPU review pending 2027");
}
BENCHMARK(BGPU_CategoryA_TopK_CPUBaseline)
    ->Arg(1'024)
    ->Arg(4'096)
    ->Arg(16'384)
    ->Arg(65'536)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

} // namespace

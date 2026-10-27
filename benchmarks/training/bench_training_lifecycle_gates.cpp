// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_training_lifecycle_gates.cpp
 * @brief Phase 5 release gates for training lifecycle hot paths.
 *
 * Provides named benchmark gates (TLG-01..TLG-08) that lock the acceptable
 * performance envelope for training module hot paths.  Gate violations are
 * reported to stderr with a standardized [PERF_GATE] prefix compatible with
 * the existing bench_lora_training.cpp convention.
 */

#include <benchmark/benchmark.h>
#include "training/lora_adapter.h"
#include "training/lora_adapter_merger.h"

#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::training;

namespace {

template <typename Fn>
double measureUs(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}

constexpr unsigned int kCanonicalRngSeed = 42u;

std::vector<float> makeWeights(size_t n, float scale, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-scale, scale);
    std::vector<float> v(n);
    for (float& x : v) {
        x = dist(rng);
    }
    return v;
}
}  // namespace

namespace gates {
constexpr double kAdapterConstructUs = 50.0;
constexpr double kApplyUpdateUs = 10.0;
constexpr double kBatchUpdateUs = 30.0;
constexpr double kLinearMergeMs = 5.0;
constexpr double kTIESMergeMs = 5.0;
constexpr double kExportWeightsUs = 200.0;
constexpr double kImportWeightsUs = 200.0;
constexpr double kThousandStepsMs = 100.0;

inline void reportViolation(const char* gate_name,
                           double measured,
                           double target,
                           const char* unit) {
    const double pct = ((measured - target) / target) * 100.0;
    fprintf(stderr,
            "[PERF_GATE] %s VIOLATION: measured=%.2f%s gate=%.2f%s (+%.1f%%)\n",
            gate_name, measured, unit, target, unit, pct);
}
}  // namespace gates

static void BM_TLG01_AdapterConstruct(benchmark::State& state) {
    const size_t in_dim = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank = 8;

    for (auto _ : state) {
        const double us = measureUs([&] {
            LoRAAdapter adapter(rank, 16.0f);
            adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);
            benchmark::DoNotOptimize(adapter);
        });
        state.counters["latency_us"] = benchmark::Counter(us, benchmark::Counter::kAvgIterations);
    }

    const double measured_us = state.counters["latency_us"].value;
    if (measured_us > gates::kAdapterConstructUs) {
        gates::reportViolation("TLG-01 AdapterConstruct", measured_us, gates::kAdapterConstructUs, "µs");
    }
    state.SetLabel("gate≤50µs");
}
BENCHMARK(BM_TLG01_AdapterConstruct)->Arg(64)->Arg(256)->Arg(768);

static void BM_TLG02_ApplyUpdate(benchmark::State& state) {
    const size_t in_dim = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank = 8;

    LoRAAdapter adapter(rank, 16.0f);
    adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);
    auto dB = makeWeights(in_dim * rank, 0.01f, kCanonicalRngSeed);
    auto dA = makeWeights(rank * out_dim, 0.01f, kCanonicalRngSeed + 1u);

    for (auto _ : state) {
        const double us = measureUs([&] {
            auto result = adapter.applyUpdate("fc", dB, dA);
            benchmark::DoNotOptimize(result);
        });
        state.counters["latency_us"] = benchmark::Counter(us, benchmark::Counter::kAvgIterations);
    }

    const double measured_us = state.counters["latency_us"].value;
    if (measured_us > gates::kApplyUpdateUs) {
        gates::reportViolation("TLG-02 ApplyUpdate", measured_us, gates::kApplyUpdateUs, "µs");
    }
    state.SetLabel("gate≤10µs");
}
BENCHMARK(BM_TLG02_ApplyUpdate)->Arg(64)->Arg(256)->Arg(768);

static void BM_TLG03_BatchUpdate(benchmark::State& state) {
    const size_t in_dim = 128;
    const size_t out_dim = 128;
    const size_t rank = 8;

    LoRAAdapter adapter(rank, 16.0f);
    for (const auto& name : {"q", "k", "v"}) {
        adapter.addLayer(name, in_dim, out_dim, rank, 16.0f);
    }

    WeightUpdateBatch batch;
    batch.layer_names = {"q", "k", "v"};
    for (size_t i = 0; i < 3; ++i) {
        batch.delta_B.push_back(makeWeights(in_dim * rank, 0.01f, kCanonicalRngSeed + static_cast<unsigned int>(i)));
        batch.delta_A.push_back(makeWeights(rank * out_dim, 0.01f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i)));
    }

    for (auto _ : state) {
        const double us = measureUs([&] {
            auto result = adapter.applyBatchUpdate(batch);
            benchmark::DoNotOptimize(result);
        });
        state.counters["latency_us"] = benchmark::Counter(us, benchmark::Counter::kAvgIterations);
    }

    const double measured_us = state.counters["latency_us"].value;
    if (measured_us > gates::kBatchUpdateUs) {
        gates::reportViolation("TLG-03 BatchUpdate", measured_us, gates::kBatchUpdateUs, "µs");
    }
    state.SetLabel("gate≤30µs");
}
BENCHMARK(BM_TLG03_BatchUpdate);

static void BM_TLG04_LinearMerge(benchmark::State& state) {
    const size_t in_dim = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank = 8;

    auto makeAdapter = [&](unsigned int seed) {
        auto a = std::make_unique<LoRAAdapter>(rank, 16.0f);
        a->addLayer("fc", in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank, 0.05f, seed);
        auto dA = makeWeights(rank * out_dim, 0.05f, seed + 500u);
        a->applyUpdate("fc", dB, dA);
        return a;
    };

    LoRAAdapterMerger merger;

    for (auto _ : state) {
        const double ms = measureUs([&] {
            auto a1 = makeAdapter(kCanonicalRngSeed);
            auto a2 = makeAdapter(kCanonicalRngSeed + 1u);
            auto result = merger.mergeLinearAll({a1.get(), a2.get()}, {0.5f, 0.5f}, rank);
            benchmark::DoNotOptimize(result);
        }) / 1000.0;
        state.counters["latency_ms"] = benchmark::Counter(ms, benchmark::Counter::kAvgIterations);
    }

    const double measured_ms = state.counters["latency_ms"].value * 1000.0;
    if (measured_ms > gates::kLinearMergeMs) {
        gates::reportViolation("TLG-04 LinearMerge", measured_ms, gates::kLinearMergeMs, "ms");
    }
    state.SetLabel("gate≤5ms");
}
BENCHMARK(BM_TLG04_LinearMerge)->Arg(64)->Arg(256);

static void BM_TLG05_TIESMerge(benchmark::State& state) {
    const size_t in_dim = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank = 8;

    auto makeAdapter = [&](unsigned int seed) {
        auto a = std::make_unique<LoRAAdapter>(rank, 16.0f);
        a->addLayer("fc", in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank, 0.05f, seed);
        auto dA = makeWeights(rank * out_dim, 0.05f, seed + 500u);
        a->applyUpdate("fc", dB, dA);
        return a;
    };

    LoRAAdapterMerger merger;

    for (auto _ : state) {
        const double ms = measureUs([&] {
            auto a1 = makeAdapter(kCanonicalRngSeed);
            auto a2 = makeAdapter(kCanonicalRngSeed + 1u);
            auto result = merger.mergeTIESAll({a1.get(), a2.get()}, rank, 0.2f);
            benchmark::DoNotOptimize(result);
        }) / 1000.0;
        state.counters["latency_ms"] = benchmark::Counter(ms, benchmark::Counter::kAvgIterations);
    }

    const double measured_ms = state.counters["latency_ms"].value * 1000.0;
    if (measured_ms > gates::kTIESMergeMs) {
        gates::reportViolation("TLG-05 TIESMerge", measured_ms, gates::kTIESMergeMs, "ms");
    }
    state.SetLabel("gate≤5ms");
}
BENCHMARK(BM_TLG05_TIESMerge)->Arg(64)->Arg(256);

static void BM_TLG06_ExportWeights(benchmark::State& state) {
    const size_t in_dim = 128, out_dim = 128, rank = 8;

    LoRAAdapter adapter(rank, 16.0f);
    for (int i = 0; i < 8; ++i) {
        std::string name = "layer_" + std::to_string(i);
        adapter.addLayer(name, in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank, 0.05f, kCanonicalRngSeed + static_cast<unsigned int>(i));
        auto dA = makeWeights(rank * out_dim, 0.05f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i));
        adapter.applyUpdate(name, dB, dA);
    }

    for (auto _ : state) {
        const double us = measureUs([&] {
            auto snap = adapter.exportWeights();
            benchmark::DoNotOptimize(snap);
        });
        state.counters["latency_us"] = benchmark::Counter(us, benchmark::Counter::kAvgIterations);
    }

    const double measured_us = state.counters["latency_us"].value;
    if (measured_us > gates::kExportWeightsUs) {
        gates::reportViolation("TLG-06 ExportWeights", measured_us, gates::kExportWeightsUs, "µs");
    }
    state.SetLabel("gate≤200µs");
}
BENCHMARK(BM_TLG06_ExportWeights);

static void BM_TLG07_ImportWeights(benchmark::State& state) {
    const size_t in_dim = 128, out_dim = 128, rank = 8;

    LoRAAdapter src(rank, 16.0f);
    for (int i = 0; i < 8; ++i) {
        std::string name = "layer_" + std::to_string(i);
        src.addLayer(name, in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank, 0.05f, kCanonicalRngSeed + static_cast<unsigned int>(i));
        auto dA = makeWeights(rank * out_dim, 0.05f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i));
        src.applyUpdate(name, dB, dA);
    }
    auto snapshot = src.exportWeights();

    for (auto _ : state) {
        const double us = measureUs([&] {
            LoRAAdapter dst(rank, 16.0f);
            dst.importWeights(snapshot);
            benchmark::DoNotOptimize(dst);
        });
        state.counters["latency_us"] = benchmark::Counter(us, benchmark::Counter::kAvgIterations);
    }

    const double measured_us = state.counters["latency_us"].value;
    if (measured_us > gates::kImportWeightsUs) {
        gates::reportViolation("TLG-07 ImportWeights", measured_us, gates::kImportWeightsUs, "µs");
    }
    state.SetLabel("gate≤200µs");
}
BENCHMARK(BM_TLG07_ImportWeights);

static void BM_TLG08_ThousandStepLifecycle(benchmark::State& state) {
    const size_t in_dim = 64, out_dim = 64, rank = 8;
    const int N_STEPS = 1000;

    auto dB = makeWeights(in_dim * rank, 0.001f, kCanonicalRngSeed);
    auto dA = makeWeights(rank * out_dim, 0.001f, kCanonicalRngSeed + 1u);

    for (auto _ : state) {
        const double ms = measureUs([&] {
            LoRAAdapter adapter(rank, 16.0f);
            adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);
            for (int step = 0; step < N_STEPS; ++step) {
                auto result = adapter.applyUpdate("fc", dB, dA);
                benchmark::DoNotOptimize(result);
            }
            benchmark::DoNotOptimize(adapter);
        }) / 1000.0;
        state.counters["latency_ms"] = benchmark::Counter(ms, benchmark::Counter::kAvgIterations);
    }

    const double measured_ms = state.counters["latency_ms"].value * 1000.0;
    if (measured_ms > gates::kThousandStepsMs) {
        gates::reportViolation("TLG-08 ThousandStepLifecycle", measured_ms, gates::kThousandStepsMs, "ms");
    }
    state.SetLabel("gate≤100ms/1000steps");
    state.SetItemsProcessed(state.iterations() * N_STEPS);
}
BENCHMARK(BM_TLG08_ThousandStepLifecycle);

BENCHMARK_MAIN();

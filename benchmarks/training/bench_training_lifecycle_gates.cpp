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
 *
 * Phase 5 Release Gates
 * ─────────────────────
 * TLG-01  LoRA adapter construction + addLayer               ≤ 50 µs  (per-layer)
 * TLG-02  Single-layer weight update (applyUpdate)           ≤ 10 µs  (per-call)
 * TLG-03  Batch weight update (3-layer WeightUpdateBatch)    ≤ 30 µs  (per-batch)
 * TLG-04  Linear merge of 2 adapters (single layer)         ≤ 5 ms   (per-merge)
 * TLG-05  TIES merge of 2 adapters (single layer)           ≤ 5 ms   (per-merge)
 * TLG-06  Export weights snapshot (8-layer adapter)         ≤ 200 µs (per-export)
 * TLG-07  Import weights snapshot (8-layer adapter)         ≤ 200 µs (per-import)
 * TLG-08  1000-step training lifecycle (adapter only)       ≤ 100 ms (total)
 *
 * Measurement hygiene:
 *  - Canonical RNG seed = 42 (bench_fixtures.h kCanonicalRngSeed convention)
 *  - CPU-only; no GPU or I/O
 *  - Benchmarks that do not depend on wall-clock I/O use CPU time (default)
 */

#include <benchmark/benchmark.h>
#include "training/lora_adapter.h"
#include "training/lora_adapter_merger.h"

#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::training;

// ─────────────────────────────────────────────────────────────────────────────
// Gate constants
// ─────────────────────────────────────────────────────────────────────────────
namespace gates {

constexpr double kAdapterConstructUs   =  50.0;   // TLG-01
constexpr double kApplyUpdateUs        =  10.0;   // TLG-02
constexpr double kBatchUpdateUs        =  30.0;   // TLG-03
constexpr double kLinearMergeMs        =   5.0;   // TLG-04
constexpr double kTIESMergeMs          =   5.0;   // TLG-05
constexpr double kExportWeightsUs      = 200.0;   // TLG-06
constexpr double kImportWeightsUs      = 200.0;   // TLG-07
constexpr double kThousandStepsMs      = 100.0;   // TLG-08

inline void reportViolation(const char* gate_name,
                             double measured,
                             double target,
                             const char* unit) {
    double pct = ((measured - target) / target) * 100.0;
    fprintf(stderr,
            "[PERF_GATE] %s VIOLATION: measured=%.2f%s gate=%.2f%s (+%.1f%%)\n",
            gate_name, measured, unit, target, unit, pct);
}

} // namespace gates

// ─────────────────────────────────────────────────────────────────────────────
// Helpers (canonical seed = 42)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

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

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// TLG-01: LoRA adapter construction + addLayer  ≤ 50 µs
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG01_AdapterConstruct(benchmark::State& state) {
    const size_t in_dim  = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank    = 8;

    for (auto _ : state) {
        LoRAAdapter adapter(rank, 16.0f);
        adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);
        benchmark::DoNotOptimize(adapter);
    }

    double us = state.iterations()
              ? state.elapsed_cpu_time() * 1e6 / static_cast<double>(state.iterations())
              : 0.0;
    if (us > gates::kAdapterConstructUs) {
        gates::reportViolation("TLG-01 AdapterConstruct", us, gates::kAdapterConstructUs, "µs");
    }
    state.SetLabel("gate≤50µs");
}
BENCHMARK(BM_TLG01_AdapterConstruct)->Arg(64)->Arg(256)->Arg(768);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-02: Single-layer applyUpdate  ≤ 10 µs
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG02_ApplyUpdate(benchmark::State& state) {
    const size_t in_dim  = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank    = 8;

    LoRAAdapter adapter(rank, 16.0f);
    adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);

    auto dB = makeWeights(in_dim * rank, 0.01f, kCanonicalRngSeed);
    auto dA = makeWeights(rank * out_dim, 0.01f, kCanonicalRngSeed + 1u);

    for (auto _ : state) {
        auto result = adapter.applyUpdate("fc", dB, dA);
        benchmark::DoNotOptimize(result);
    }

    double us = state.iterations()
              ? state.elapsed_cpu_time() * 1e6 / static_cast<double>(state.iterations())
              : 0.0;
    if (us > gates::kApplyUpdateUs) {
        gates::reportViolation("TLG-02 ApplyUpdate", us, gates::kApplyUpdateUs, "µs");
    }
    state.SetLabel("gate≤10µs");
}
BENCHMARK(BM_TLG02_ApplyUpdate)->Arg(64)->Arg(256)->Arg(768);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-03: Batch weight update (3-layer)  ≤ 30 µs
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG03_BatchUpdate(benchmark::State& state) {
    const size_t in_dim  = 128;
    const size_t out_dim = 128;
    const size_t rank    = 8;

    LoRAAdapter adapter(rank, 16.0f);
    for (const auto& name : {"q", "k", "v"}) {
        adapter.addLayer(name, in_dim, out_dim, rank, 16.0f);
    }

    WeightUpdateBatch batch;
    batch.layer_names = {"q", "k", "v"};
    for (size_t i = 0; i < 3; ++i) {
        batch.delta_B.push_back(makeWeights(in_dim * rank,  0.01f, kCanonicalRngSeed + static_cast<unsigned int>(i)));
        batch.delta_A.push_back(makeWeights(rank * out_dim, 0.01f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i)));
    }

    for (auto _ : state) {
        auto result = adapter.applyBatchUpdate(batch);
        benchmark::DoNotOptimize(result);
    }

    double us = state.iterations()
              ? state.elapsed_cpu_time() * 1e6 / static_cast<double>(state.iterations())
              : 0.0;
    if (us > gates::kBatchUpdateUs) {
        gates::reportViolation("TLG-03 BatchUpdate", us, gates::kBatchUpdateUs, "µs");
    }
    state.SetLabel("gate≤30µs");
}
BENCHMARK(BM_TLG03_BatchUpdate);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-04: Linear merge of 2 adapters (single layer)  ≤ 5 ms
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG04_LinearMerge(benchmark::State& state) {
    const size_t in_dim  = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank    = 8;

    auto makeAdapter = [&](unsigned int seed) {
        auto a = std::make_unique<LoRAAdapter>(rank, 16.0f);
        a->addLayer("fc", in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank,  0.05f, seed);
        auto dA = makeWeights(rank * out_dim, 0.05f, seed + 500u);
        a->applyUpdate("fc", dB, dA);
        return a;
    };

    LoRAAdapterMerger merger;

    for (auto _ : state) {
        auto a1 = makeAdapter(kCanonicalRngSeed);
        auto a2 = makeAdapter(kCanonicalRngSeed + 1u);
        auto result = merger.mergeLinearAll(
            {a1.get(), a2.get()}, {0.5f, 0.5f}, rank);
        benchmark::DoNotOptimize(result);
    }

    double ms = state.iterations()
              ? state.elapsed_cpu_time() * 1e3 / static_cast<double>(state.iterations())
              : 0.0;
    if (ms > gates::kLinearMergeMs) {
        gates::reportViolation("TLG-04 LinearMerge", ms, gates::kLinearMergeMs, "ms");
    }
    state.SetLabel("gate≤5ms");
}
BENCHMARK(BM_TLG04_LinearMerge)->Arg(64)->Arg(256);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-05: TIES merge of 2 adapters (single layer)  ≤ 5 ms
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG05_TIESMerge(benchmark::State& state) {
    const size_t in_dim  = static_cast<size_t>(state.range(0));
    const size_t out_dim = in_dim;
    const size_t rank    = 8;

    auto makeAdapter = [&](unsigned int seed) {
        auto a = std::make_unique<LoRAAdapter>(rank, 16.0f);
        a->addLayer("fc", in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank,  0.05f, seed);
        auto dA = makeWeights(rank * out_dim, 0.05f, seed + 500u);
        a->applyUpdate("fc", dB, dA);
        return a;
    };

    LoRAAdapterMerger merger;

    for (auto _ : state) {
        auto a1 = makeAdapter(kCanonicalRngSeed);
        auto a2 = makeAdapter(kCanonicalRngSeed + 1u);
        auto result = merger.mergeTIESAll({a1.get(), a2.get()}, rank, 0.2f);
        benchmark::DoNotOptimize(result);
    }

    double ms = state.iterations()
              ? state.elapsed_cpu_time() * 1e3 / static_cast<double>(state.iterations())
              : 0.0;
    if (ms > gates::kTIESMergeMs) {
        gates::reportViolation("TLG-05 TIESMerge", ms, gates::kTIESMergeMs, "ms");
    }
    state.SetLabel("gate≤5ms");
}
BENCHMARK(BM_TLG05_TIESMerge)->Arg(64)->Arg(256);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-06: Export weights snapshot (8-layer adapter)  ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG06_ExportWeights(benchmark::State& state) {
    const size_t in_dim = 128, out_dim = 128, rank = 8;

    LoRAAdapter adapter(rank, 16.0f);
    for (int i = 0; i < 8; ++i) {
        std::string name = "layer_" + std::to_string(i);
        adapter.addLayer(name, in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank,  0.05f, kCanonicalRngSeed + static_cast<unsigned int>(i));
        auto dA = makeWeights(rank * out_dim, 0.05f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i));
        adapter.applyUpdate(name, dB, dA);
    }

    for (auto _ : state) {
        auto snap = adapter.exportWeights();
        benchmark::DoNotOptimize(snap);
    }

    double us = state.iterations()
              ? state.elapsed_cpu_time() * 1e6 / static_cast<double>(state.iterations())
              : 0.0;
    if (us > gates::kExportWeightsUs) {
        gates::reportViolation("TLG-06 ExportWeights", us, gates::kExportWeightsUs, "µs");
    }
    state.SetLabel("gate≤200µs");
}
BENCHMARK(BM_TLG06_ExportWeights);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-07: Import weights snapshot (8-layer adapter)  ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG07_ImportWeights(benchmark::State& state) {
    const size_t in_dim = 128, out_dim = 128, rank = 8;

    LoRAAdapter src(rank, 16.0f);
    for (int i = 0; i < 8; ++i) {
        std::string name = "layer_" + std::to_string(i);
        src.addLayer(name, in_dim, out_dim, rank, 16.0f);
        auto dB = makeWeights(in_dim * rank,  0.05f, kCanonicalRngSeed + static_cast<unsigned int>(i));
        auto dA = makeWeights(rank * out_dim, 0.05f, kCanonicalRngSeed + 100u + static_cast<unsigned int>(i));
        src.applyUpdate(name, dB, dA);
    }
    auto snapshot = src.exportWeights();

    for (auto _ : state) {
        LoRAAdapter dst(rank, 16.0f);
        dst.importWeights(snapshot);
        benchmark::DoNotOptimize(dst);
    }

    double us = state.iterations()
              ? state.elapsed_cpu_time() * 1e6 / static_cast<double>(state.iterations())
              : 0.0;
    if (us > gates::kImportWeightsUs) {
        gates::reportViolation("TLG-07 ImportWeights", us, gates::kImportWeightsUs, "µs");
    }
    state.SetLabel("gate≤200µs");
}
BENCHMARK(BM_TLG07_ImportWeights);

// ─────────────────────────────────────────────────────────────────────────────
// TLG-08: 1000-step training lifecycle (adapter only)  ≤ 100 ms total
// ─────────────────────────────────────────────────────────────────────────────
static void BM_TLG08_ThousandStepLifecycle(benchmark::State& state) {
    const size_t in_dim = 64, out_dim = 64, rank = 8;
    const int    N_STEPS = 1000;

    auto dB = makeWeights(in_dim * rank,  0.001f, kCanonicalRngSeed);
    auto dA = makeWeights(rank * out_dim, 0.001f, kCanonicalRngSeed + 1u);

    for (auto _ : state) {
        LoRAAdapter adapter(rank, 16.0f);
        adapter.addLayer("fc", in_dim, out_dim, rank, 16.0f);

        for (int step = 0; step < N_STEPS; ++step) {
            auto result = adapter.applyUpdate("fc", dB, dA);
            benchmark::DoNotOptimize(result);
        }
        benchmark::DoNotOptimize(adapter);
    }

    double ms = state.iterations()
              ? state.elapsed_cpu_time() * 1e3 / static_cast<double>(state.iterations())
              : 0.0;
    if (ms > gates::kThousandStepsMs) {
        gates::reportViolation("TLG-08 ThousandStepLifecycle", ms, gates::kThousandStepsMs, "ms");
    }
    state.SetLabel("gate≤100ms/1000steps");
    state.SetItemsProcessed(state.iterations() * N_STEPS);
}
BENCHMARK(BM_TLG08_ThousandStepLifecycle);

BENCHMARK_MAIN();

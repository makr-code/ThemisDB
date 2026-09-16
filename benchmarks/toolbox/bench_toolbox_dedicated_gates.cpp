// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_toolbox_dedicated_gates.cpp
 * @brief Wave D — toolbox module dedicated benchmark gates.
 *
 * Provides reproducible latency measurements for the toolbox module under
 * Wave D operability requirements.
 *
 * ## Benchmark families
 *
 * ### TB-BM-01 — Tool dispatch throughput
 *   Measures per-dispatch cost for the stub composite router.
 *
 * ### TB-BM-02 — Content bridge throughput
 *   Measures per-bridge cost for the stub content bridge.
 *
 * ### TB-BM-03 — Extraction pipeline latency
 *   Measures per-document extraction cost including field count computation.
 *
 * ### TB-BM-04 — Composite routing batch (1 000 dispatches)
 *   Amortised per-dispatch cost across 1 000 composite route+bridge operations.
 *
 * ## Hard release gates
 *
 * | Gate ID  | Benchmark          | Threshold         |
 * |----------|--------------------|-------------------|
 * | TB-BM-01 | ToolDispatch       | p99 ≤ 200 ns      |
 * | TB-BM-02 | ContentBridge      | p99 ≤ 200 ns      |
 * | TB-BM-03 | ExtractionPipeline | p99 ≤ 500 ns      |
 * | TB-BM-04 | BatchComposite     | p99 ≤ 100 µs/batch|
 *
 * @see src/toolbox/ROADMAP.md — Wave D items
 * @see docs/operability/RUNBOOK_TOOLBOX.md
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace toolbox_dedicated {

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int      kRepetitions   = 5;

// ─── In-process stubs (SIMULATION NOTE: no external bridge or content required)

static bool stubDispatch(const std::string& tool, const std::string& content) noexcept {
    (void)tool; (void)content;
    return true;
}

static bool stubBridge(uint64_t content_id) noexcept {
    (void)content_id;
    return true;
}

static std::size_t stubExtract(const std::string& doc_id) noexcept {
    return (doc_id.size() % 16) + 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// TB-BM-01 — Tool dispatch throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Toolbox_ToolDispatch(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string tool    = "tool_bm_" + std::to_string(seed % 32);
        const std::string content = "content_" + std::to_string(seed);
        benchmark::DoNotOptimize(stubDispatch(tool, content));
        ++seed;
    }
}
BENCHMARK(BM_Toolbox_ToolDispatch)
    ->Name("TB-BM-01/ToolDispatch")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TB-BM-02 — Content bridge throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Toolbox_ContentBridge(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint64_t content_id = seed * 6364136223846793005ULL;
        benchmark::DoNotOptimize(stubBridge(content_id));
        ++seed;
    }
}
BENCHMARK(BM_Toolbox_ContentBridge)
    ->Name("TB-BM-02/ContentBridge")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TB-BM-03 — Extraction pipeline latency
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Toolbox_ExtractionPipeline(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string doc_id = "doc_bm_" + std::to_string(seed % 8000);
        benchmark::DoNotOptimize(stubExtract(doc_id));
        ++seed;
    }
}
BENCHMARK(BM_Toolbox_ExtractionPipeline)
    ->Name("TB-BM-03/ExtractionPipeline")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TB-BM-04 — Composite routing batch (1 000 dispatches)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Toolbox_BatchComposite(benchmark::State& state) {
    constexpr int kBatch = 1000;
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            const std::string tool    = "tool_batch_" + std::to_string(seed % 16);
            const std::string content = "content_" + std::to_string(seed);
            const uint64_t content_id = seed;
            benchmark::DoNotOptimize(stubDispatch(tool, content));
            benchmark::DoNotOptimize(stubBridge(content_id));
            ++seed;
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_Toolbox_BatchComposite)
    ->Name("TB-BM-04/BatchComposite1000")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace toolbox_dedicated
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();

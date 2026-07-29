// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_scraper_release_gates.cpp
 * @brief Phase 5 scraper module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the scraper module hot paths
 * identified in the scraper module roadmap (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-SCR-01 — Error enum cast throughput
 *   Measures the cost of casting ScraperError values from int32_t.
 *
 * ### GATE-SCR-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all ScraperError codes.
 *
 * ### GATE-SCR-03 — ScrapeRequest struct allocation
 *   Measures in-process heap allocation for ScrapeRequest; release gate for
 *   high-throughput crawl submission paths.
 *
 * ### GATE-SCR-04 — Batch error cast (1 000 iterations)
 *   Amortised error-cast cost across 1 000 mixed codes; simulates a
 *   high-frequency pipeline-stage error classification loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-SCR-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-SCR-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-SCR-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-SCR-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/scraper/ROADMAP.md — Phase 5 items
 * @see include/scraper/scraper_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "scraper/scraper_api_contract.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace scr {

/// Canonical PRNG seed for all SCR benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-SCR-01 — Error enum cast throughput
// ============================================================================

static void BM_SCR01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        scraper::ScraperError::kEvaluationFailed);
    for (auto _ : state) {
        auto e = static_cast<scraper::ScraperError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-SCR-01: p99 <= 5 ns");
}
BENCHMARK(BM_SCR01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCR-02 — Switch dispatch throughput
// ============================================================================

static void BM_SCR02_SwitchDispatch(benchmark::State& state) {
    const scraper::ScraperError codes[] = {
        scraper::ScraperError::kSuccess,
        scraper::ScraperError::kFetchFailed,
        scraper::ScraperError::kRenderTimeout,
        scraper::ScraperError::kParseError,
        scraper::ScraperError::kEvaluationFailed,
        scraper::ScraperError::kMetadataWriteFailed,
        scraper::ScraperError::kSourceNotFound,
        scraper::ScraperError::kPaginationLimit,
        scraper::ScraperError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case scraper::ScraperError::kSuccess:             label = "ok"; break;
            case scraper::ScraperError::kFetchFailed:         label = "fetch"; break;
            case scraper::ScraperError::kRenderTimeout:       label = "render"; break;
            case scraper::ScraperError::kParseError:          label = "parse"; break;
            case scraper::ScraperError::kEvaluationFailed:    label = "eval"; break;
            case scraper::ScraperError::kMetadataWriteFailed: label = "meta"; break;
            case scraper::ScraperError::kSourceNotFound:      label = "src"; break;
            case scraper::ScraperError::kPaginationLimit:     label = "page"; break;
            case scraper::ScraperError::kInternalError:       label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-SCR-02: p99 <= 10 ns");
}
BENCHMARK(BM_SCR02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCR-03 — ScrapeRequest struct allocation
// ============================================================================

static void BM_SCR03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        scraper::ScrapeRequest req;
        req.source_url           = "https://bench.example.com/page42";
        req.enable_js_render     = false;
        req.max_pagination_depth = 10;
        benchmark::DoNotOptimize(req);
    }
    state.SetLabel("GATE-SCR-03: p99 <= 500 ns");
}
BENCHMARK(BM_SCR03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCR-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_SCR04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8500, 8501, 8502, 8503, 8504, 8505, 8506, 8507
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<scraper::ScraperError>(kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-SCR-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_SCR04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace scr
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_scraper_pipeline_depth.cpp
 * @brief Scraper module pipeline-depth benchmarks (PIPE-01..PIPE-04).
 *
 * ## Benchmark families
 *
 * ### PIPE-01 — Batch-emit 1 000 DiagnosticEvents
 *   Gate: p99 ≤ 500 µs per batch
 *
 * ### PIPE-02 — ScraperRunSummary aggregation over 10 000 events
 *   Gate: total ≤ 5 ms
 *
 * ### PIPE-03 — ScrapeRequest batch allocation (1 000 requests)
 *   Gate: p99 ≤ 200 µs per batch
 *
 * ### PIPE-04 — faultClassOf() + defaultSeverityOf() loop 10 000×
 *   Gate: p99 ≤ 10 µs per batch
 *
 * @see benchmarks/scraper/bench_scraper_release_gates.cpp
 * @see include/scraper/scraper_diagnostics.h
 * @see include/scraper/scraper_run_summary.h
 */

#include <benchmark/benchmark.h>
#include "scraper/scraper_api_contract.h"
#include "scraper/scraper_diagnostics.h"
#include "scraper/scraper_run_summary.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace scr {

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int kRepetitions = 5;

// ============================================================================
// PIPE-01 — Batch-emit 1 000 DiagnosticEvents
// ============================================================================

static void BM_PIPE01_BatchEmit1000(benchmark::State& state) {
    static const scraper::ScraperError kErrors[] = {
        scraper::ScraperError::kFetchFailed,
        scraper::ScraperError::kParseError,
        scraper::ScraperError::kEvaluationFailed,
        scraper::ScraperError::kMetadataWriteFailed,
        scraper::ScraperError::kPaginationLimit,
    };
    static constexpr int kBatch = 1000;
    static const std::string kUrl = "https://bench.example.com/pipe01";
    static const std::string kMsg = "pipeline depth benchmark event";

    scraper::ListeningScraperDiagnosticSink sink;
    for (auto _ : state) {
        state.PauseTiming();
        sink.clear();
        state.ResumeTiming();
        for (int i = 0; i < kBatch; ++i) {
            auto ev = scraper::makeDiagnosticEvent(
                kErrors[i % 5], kUrl, kMsg);
            sink.emit(ev);
        }
        benchmark::DoNotOptimize(sink.size());
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.SetLabel("PIPE-01: p99 <= 500 us per batch");
}
BENCHMARK(BM_PIPE01_BatchEmit1000)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// PIPE-02 — ScraperRunSummary aggregation over 10 000 events
// ============================================================================

static void BM_PIPE02_SummaryAggregation10k(benchmark::State& state) {
    static const scraper::ScraperError kErrors[] = {
        scraper::ScraperError::kFetchFailed,
        scraper::ScraperError::kParseError,
        scraper::ScraperError::kEvaluationFailed,
        scraper::ScraperError::kMetadataWriteFailed,
        scraper::ScraperError::kPaginationLimit,
    };
    static constexpr int kEvents = 10000;
    static const std::string kUrl = "https://bench.example.com/pipe02";
    static const std::string kMsg = "aggregation bench";

    for (auto _ : state) {
        scraper::ListeningScraperDiagnosticSink sink;
        scraper::ScraperRunSummaryCollector collector;
        collector.attach(sink);
        for (int i = 0; i < kEvents; ++i) {
            sink.emit(scraper::makeDiagnosticEvent(kErrors[i % 5], kUrl, kMsg));
        }
        collector.setRunStats(static_cast<uint32_t>(kEvents), 10000);
        auto s = collector.summary();
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * kEvents);
    state.SetLabel("PIPE-02: total <= 5 ms");
}
BENCHMARK(BM_PIPE02_SummaryAggregation10k)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// PIPE-03 — ScrapeRequest batch allocation (1 000 requests)
// ============================================================================

static void BM_PIPE03_ScrapeRequestBatchAlloc(benchmark::State& state) {
    static constexpr int kBatch = 1000;
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            scraper::ScrapeRequest req;
            req.source_url           = "https://bench.example.com/pipe03/" + std::to_string(i);
            req.enable_js_render     = (i % 2 == 0);
            req.max_pagination_depth = 10;
            benchmark::DoNotOptimize(req);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.SetLabel("PIPE-03: p99 <= 200 us per batch");
}
BENCHMARK(BM_PIPE03_ScrapeRequestBatchAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// PIPE-04 — faultClassOf() + defaultSeverityOf() loop 10 000×
// ============================================================================

static void BM_PIPE04_FaultClassLoop10k(benchmark::State& state) {
    static const scraper::ScraperError kAll[] = {
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
    static constexpr int kLoop = 10000;
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        for (int i = 0; i < kLoop; ++i) {
            seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
            auto e = kAll[seed % 9];
            auto fc = scraper::faultClassOf(e);
            auto sv = scraper::defaultSeverityOf(e);
            benchmark::DoNotOptimize(fc);
            benchmark::DoNotOptimize(sv);
        }
    }
    state.SetItemsProcessed(state.iterations() * kLoop);
    state.SetLabel("PIPE-04: p99 <= 10 us per batch");
}
BENCHMARK(BM_PIPE04_FaultClassLoop10k)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace scr
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();

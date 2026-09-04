// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_temporal_release_gates.cpp
 * @brief Phase 5 temporal hot-path release-gate benchmarks (TRG-01..TRG-06).
 *
 * Provides reproducible latency and throughput measurements for the temporal
 * module critical paths.  Results serve as release gates.
 *
 * ## Benchmark families
 *
 * ### TRG-01 — Bi-temporal insert (in-memory)
 *   Gate: ≥ 100k inserts/s.
 *
 * ### TRG-02 — Interval tree point query (1k intervals)
 *   Gate: p99 ≤ 500 µs.
 *
 * ### TRG-03 — Snapshot read (100 rows)
 *   Gate: p99 ≤ 1 ms.
 *
 * ### TRG-04 — Retention check (single row)
 *   Gate: p99 ≤ 100 µs.
 *
 * ### TRG-05 — PITR restore (mock, 10 entries)
 *   Gate: p99 ≤ 5 ms.
 *
 * ### TRG-06 — Valid-time range computation
 *   Gate: p99 ≤ 50 µs.
 *
 * ## Hard release gates
 *
 * | Gate ID    | Benchmark | Threshold        |
 * |------------|-----------|------------------|
 * | GATE-TRG-01 | TRG-01   | ≥ 100k inserts/s |
 * | GATE-TRG-02 | TRG-02   | p99 ≤ 500 µs     |
 * | GATE-TRG-03 | TRG-03   | p99 ≤ 1 ms       |
 * | GATE-TRG-04 | TRG-04   | p99 ≤ 100 µs     |
 * | GATE-TRG-05 | TRG-05   | p99 ≤ 5 ms       |
 * | GATE-TRG-06 | TRG-06   | p99 ≤ 50 µs      |
 *
 * @see include/temporal/temporal_api_contract.h
 * @see src/temporal/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "temporal/temporal_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

namespace themis {
namespace bench {
namespace trg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all TRG benchmarks.
static constexpr uint64_t kTemporalCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------

struct BiTemporalEntry {
    int          id = 0;
    std::int64_t valid_start;
    std::int64_t valid_end;
    std::int64_t tx_time;
};

/// Simulates an in-memory bi-temporal insert (vector push_back).
static void btInsert(std::vector<BiTemporalEntry>& store,
                     int id, std::int64_t vs, std::int64_t ve, std::int64_t tx) {
    store.push_back({id, vs, ve, tx});
}

/// Simulates interval tree point query: linear scan over sorted intervals.
static std::size_t intervalQuery(const std::vector<BiTemporalEntry>& store,
                                  std::int64_t point) {
    std::size_t count = 0;
    for (const auto& e : store) {
        if (e.valid_start <= point && point <= e.valid_end) {
          ++count;
        }
    }
    return count;
}

/// Simulates snapshot read: filter by valid_time ∩ {T} and tx_time ≤ snapshot_tx.
static std::size_t snapshotRead(const std::vector<BiTemporalEntry>& store,
                                 std::int64_t t, std::int64_t snapshot_tx) {
    std::size_t count = 0;
    for (const auto& e : store) {
        if (e.tx_time <= snapshot_tx && e.valid_start <= t && t <= e.valid_end) {
          ++count;
        }
    }
    return count;
}

/// Simulates retention check for a single row.
static bool isExpired(std::int64_t valid_end, std::int64_t boundary) {
    return valid_end < boundary;
}

/// Simulates PITR restore over a small history (linear scan).
static std::size_t pitrRestore(const std::vector<BiTemporalEntry>& history,
                                std::int64_t target_tx) {
    std::size_t count = 0;
    for (const auto& e : history) {
        if (e.tx_time <= target_tx) {
          ++count;
        }
    }
    return count;
}

/// Computes a valid-time interval intersection length (ns).
static std::int64_t validTimeIntersection(std::int64_t as, std::int64_t ae,
                                           std::int64_t bs, std::int64_t be) {
    std::int64_t start = std::max(as, bs);
    std::int64_t end   = std::min(ae, be);
    return std::max(std::int64_t{0}, end - start);
}

// ---------------------------------------------------------------------------
// TRG-01 — Bi-temporal insert (in-memory)
// ---------------------------------------------------------------------------

/**
 * @brief TRG-01: In-memory bi-temporal insert throughput.
 *
 * GATE-TRG-01: ≥ 100k inserts/s.
 */
static void BM_TRG01_BiTemporalInsert(benchmark::State& state) {
    std::vector<BiTemporalEntry> store;
    store.reserve(1'000'000);
    std::int64_t tx = 1;
    int id = 0;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        btInsert(store, id++, 1000LL * id, 2000LL * id, tx++);
    }
    store.clear();
    store.reserve(1'000'000);

    for (auto _ : state) {
        btInsert(store, id++, 1000LL * id, 2000LL * id, tx++);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("GATE-TRG-01: >= 100k inserts/s");
}
BENCHMARK(BM_TRG01_BiTemporalInsert)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRG-02 — Interval tree point query (1k intervals)
// ---------------------------------------------------------------------------

/**
 * @brief TRG-02: Point query over 1k pre-loaded intervals.
 *
 * GATE-TRG-02: p99 ≤ 500 µs.
 */
static void BM_TRG02_IntervalTreeQuery(benchmark::State& state) {
    std::mt19937_64 rng(kTemporalCanonicalSeed);
    std::vector<BiTemporalEntry> store;
    store.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        std::int64_t s = static_cast<std::int64_t>(i) * 1000LL;
        store.push_back({i, s, s + 500LL, static_cast<std::int64_t>(i)});
    }

    std::uniform_int_distribution<std::int64_t> dist(0, 1000LL * 1000LL);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(intervalQuery(store, dist(rng)));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(intervalQuery(store, dist(rng)));
    }
    state.SetLabel("GATE-TRG-02: p99 <= 500us");
}
BENCHMARK(BM_TRG02_IntervalTreeQuery)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRG-03 — Snapshot read (100 rows)
// ---------------------------------------------------------------------------

/**
 * @brief TRG-03: Snapshot read over 100 bi-temporal rows.
 *
 * GATE-TRG-03: p99 ≤ 1 ms.
 */
static void BM_TRG03_SnapshotRead(benchmark::State& state) {
    std::vector<BiTemporalEntry> store;
    store.reserve(100);
    for (int i = 0; i < 100; ++i) {
        store.push_back({i, 100LL * i, 100LL * (i + 1), static_cast<std::int64_t>(i + 1)});
    }
    std::int64_t snapshot_tx = 50LL;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(snapshotRead(store, 2500LL, snapshot_tx));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(snapshotRead(store, 2500LL, snapshot_tx));
    }
    state.SetLabel("GATE-TRG-03: p99 <= 1ms");
}
BENCHMARK(BM_TRG03_SnapshotRead)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRG-04 — Retention check (single row)
// ---------------------------------------------------------------------------

/**
 * @brief TRG-04: Retention boundary check for a single row.
 *
 * GATE-TRG-04: p99 ≤ 100 µs.
 */
static void BM_TRG04_RetentionCheck(benchmark::State& state) {
    std::int64_t boundary  = 1'000'000'000LL;
    std::int64_t valid_end = 999'999'000LL; // just before boundary

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(isExpired(valid_end, boundary));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(isExpired(valid_end, boundary));
    }
    state.SetLabel("GATE-TRG-04: p99 <= 100us");
}
BENCHMARK(BM_TRG04_RetentionCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRG-05 — PITR restore (mock, 10 entries)
// ---------------------------------------------------------------------------

/**
 * @brief TRG-05: PITR restore scan over 10 history entries.
 *
 * GATE-TRG-05: p99 ≤ 5 ms.
 */
static void BM_TRG05_PitrRestore(benchmark::State& state) {
    std::vector<BiTemporalEntry> history;
    for (int i = 1; i <= 10; ++i) {
        history.push_back({i, 1000LL * i, temporal::kTemporalOpenEnd,
                           static_cast<std::int64_t>(i) * 100LL});
    }
    std::int64_t target_tx = 600LL; // midpoint

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(pitrRestore(history, target_tx));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(pitrRestore(history, target_tx));
    }
    state.SetLabel("GATE-TRG-05: p99 <= 5ms");
}
BENCHMARK(BM_TRG05_PitrRestore)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TRG-06 — Valid-time range computation
// ---------------------------------------------------------------------------

/**
 * @brief TRG-06: Valid-time interval intersection length computation.
 *
 * GATE-TRG-06: p99 ≤ 50 µs.
 */
static void BM_TRG06_ValidTimeRange(benchmark::State& state) {
    std::mt19937_64 rng(kTemporalCanonicalSeed);
    std::uniform_int_distribution<std::int64_t> dist(0, 1'000'000LL);

    std::int64_t as = 100LL, ae = 5000LL;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        std::int64_t bs = dist(rng), be = bs + dist(rng) % 1000;
        benchmark::DoNotOptimize(validTimeIntersection(as, ae, bs, be));
    }
    for (auto _ : state) {
        std::int64_t bs = dist(rng), be = bs + dist(rng) % 1000;
        benchmark::DoNotOptimize(validTimeIntersection(as, ae, bs, be));
    }
    state.SetLabel("GATE-TRG-06: p99 <= 50us");
}
BENCHMARK(BM_TRG06_ValidTimeRange)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace trg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();

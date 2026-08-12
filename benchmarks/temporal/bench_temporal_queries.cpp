/// @file bench_temporal_queries.cpp
/// @brief Performance benchmarks for the Temporal module.
///
/// Covers the following process lines:
///   - BiTemporalTable::insertWithValidTime()
///   - BiTemporalTable::queryBiTemporal()     (AS-OF both axes)
///   - BiTemporalTable::queryCurrentByValidTime()
///   - BiTemporalTable::updateForValidTime()
///   - BiTemporalTable::deleteForValidTime()
///   - BiTemporalTable::getHistory()
///
/// Performance targets (src/temporal/ROADMAP.md):
///   - insertWithValidTime single row:         < 1 µs
///   - queryBiTemporal   1000-version table:   < 50 µs
///   - queryCurrentByValidTime:                < 10 µs

#include <benchmark/benchmark.h>
#include "temporal/bi_temporal.h"
#include "temporal/temporal_types.h"
#include <memory>
#include <string>

using namespace themisdb::temporal;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a table pre-populated with @p n non-overlapping versions for key "k".
std::unique_ptr<BiTemporalTable> makeTableWithVersions(int n) {
    auto tbl = std::make_unique<BiTemporalTable>("bench_tbl", "node_a");
    for (int i = 0; i < n; ++i) {
        Timestamp vstart = 1000LL + static_cast<Timestamp>(i) * 100;
        Timestamp vend   = vstart + 100;
        tbl->insertWithValidTime("k", {{"v", i}}, {vstart, vend});
    }
    return tbl;
}

/// Build a table with @p keys, each with one version.
std::unique_ptr<BiTemporalTable> makeTableWithKeys(int keys) {
    auto tbl = std::make_unique<BiTemporalTable>("bench_tbl", "node_a");
    for (int i = 0; i < keys; ++i) {
        tbl->insertWithValidTime("k" + std::to_string(i), {{"v", i}},
                                 {1000, 9000});
    }
    return tbl;
}

} // anonymous namespace

// ============================================================================
// insertWithValidTime – write throughput
// ============================================================================

static void BM_BiTemporalTable_Insert(benchmark::State& state) {
    const int rows_per_key = static_cast<int>(state.range(0));
    BiTemporalTable tbl("bench_tbl", "node_a");
    std::atomic<int> counter{0};

    for (auto _ : state) {
        int idx        = counter.fetch_add(1, std::memory_order_relaxed);
        Timestamp vs   = 1000LL + static_cast<Timestamp>(idx) * 10;
        bool ok = tbl.insertWithValidTime(
            "k" + std::to_string(idx / rows_per_key),
            {{"v", idx}},
            {vs, vs + 10});
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_BiTemporalTable_Insert)
    ->Arg(1)
    ->Arg(100)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// queryBiTemporal – AS-OF read on growing version history
// ============================================================================

static void BM_BiTemporalTable_QueryBiTemporal(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto tbl    = makeTableWithVersions(n);

    // Query in the middle of the valid-time range
    Timestamp valid_at = 1000LL + static_cast<Timestamp>(n / 2) * 100 + 50;
    Timestamp sys_now  = now();

    for (auto _ : state) {
        auto rows = tbl->queryBiTemporal("k", sys_now, valid_at);
        benchmark::DoNotOptimize(rows);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("versions=" + std::to_string(n));
}

BENCHMARK(BM_BiTemporalTable_QueryBiTemporal)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// queryCurrentByValidTime – point-in-time lookup across many keys
// ============================================================================

static void BM_BiTemporalTable_QueryCurrentByValidTime(benchmark::State& state) {
    const int keys = static_cast<int>(state.range(0));
    auto tbl       = makeTableWithKeys(keys);

    for (auto _ : state) {
        for (int i = 0; i < keys; ++i) {
            auto rows = tbl->queryCurrentByValidTime(
                "k" + std::to_string(i), 5000);
            benchmark::DoNotOptimize(rows);
        }
    }

    state.SetItemsProcessed(state.iterations() * keys);
    state.SetLabel("keys=" + std::to_string(keys));
}

BENCHMARK(BM_BiTemporalTable_QueryCurrentByValidTime)
    ->Arg(10)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// updateForValidTime – update throughput
// ============================================================================

static void BM_BiTemporalTable_Update(benchmark::State& state) {
    const int versions = static_cast<int>(state.range(0));
    auto tbl           = makeTableWithVersions(versions);

    // Update at the midpoint of the first version
    Timestamp valid_at = 1050;

    for (auto _ : state) {
        bool ok = tbl->updateForValidTime("k", {{"updated", 1}}, valid_at);
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("versions=" + std::to_string(versions));
}

BENCHMARK(BM_BiTemporalTable_Update)
    ->Arg(1)
    ->Arg(100)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// deleteForValidTime
// ============================================================================

static void BM_BiTemporalTable_Delete(benchmark::State& state) {
    // Re-insert before each deletion iteration
    BiTemporalTable tbl("bench_tbl", "node_a");
    tbl.insertWithValidTime("k", {{"v", 1}}, {1000, 9000});
    const Timestamp valid_at = 5000;

    for (auto _ : state) {
        size_t n = tbl.deleteForValidTime("k", valid_at);
        benchmark::DoNotOptimize(n);
        // Re-insert so the benchmark remains valid across iterations
        tbl.insertWithValidTime("k", {{"v", 1}}, {1000, 9000});
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_BiTemporalTable_Delete)->Unit(benchmark::kNanosecond);

// ============================================================================
// getHistory – scan cost vs. number of versions
// ============================================================================

static void BM_BiTemporalTable_GetHistory(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto tbl    = makeTableWithVersions(n);

    for (auto _ : state) {
        auto hist = tbl->getHistory("k");
        benchmark::DoNotOptimize(hist);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("versions=" + std::to_string(n));
}

BENCHMARK(BM_BiTemporalTable_GetHistory)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

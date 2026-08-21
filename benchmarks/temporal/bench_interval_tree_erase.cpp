/**
 * @file bench_interval_tree_erase.cpp
 * @brief Benchmarks for IntervalTreeIndex::erase() vs. rebuild baseline.
 *
 * Addresses ROADMAP Phase 4 item:
 *   "Benchmark: IntervalTreeIndex::erase() vs. rebuild baseline"
 *
 * Scenarios benchmarked
 * ─────────────────────
 * ITEB-01  Single-key erase on a tree of varying sizes (100 … 10 000 keys).
 * ITEB-02  Bulk-erase all keys one-by-one (cumulative cost).
 * ITEB-03  Rebuild baseline — rebuild from scratch after clearing (comparison).
 * ITEB-04  Mixed read/erase workload (80 % read, 20 % erase) via concurrent
 *          threads.  Validates that the exclusive-write lock does not cause
 *          excessive reader stalls.
 * ITEB-05  Large-payload: each entry carries a 1 KiB JSON blob; measures
 *          memory-move overhead during erase+rebuild.
 *
 * Build:
 *   cmake --build build --target bench_interval_tree_erase --config Release
 * Run:
 *   ./build/bench_interval_tree_erase --benchmark_filter=.
 */

#include <benchmark/benchmark.h>

#include "temporal/interval_tree_index.h"
#include "temporal/temporal_types.h"

#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themisdb::temporal;

// ── Helpers ──────────────────────────────────────────────────────────────────

namespace {

constexpr Timestamp kStep = 1000; ///< ms between interval start times

/**
 * Populate @p tree with @p n entries using non-overlapping intervals.
 * Keys are "key_000000" … "key_N".
 */
void populateTree(IntervalTreeIndex& tree, int n) {
    for (int i = 0; i < n; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "key_%06d", i);
        IntervalEntry e;
        e.key       = buf;
        e.range     = {static_cast<Timestamp>(i * kStep),
                       static_cast<Timestamp>((i + 1) * kStep - 1)};
        e.payload   = {{"i", i}};
        tree.insert(e);
    }
}

/**
 * Return a deterministic key name for index @p i.
 */
std::string keyName(int i) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "key_%06d", i);
    return buf;
}

} // namespace

// ── ITEB-01: single-key erase on trees of varying size ───────────────────────

/**
 * Benchmark: erase a single, randomly chosen key from a tree pre-populated
 * with @p n entries.  The state setup re-inserts the erased key so each
 * iteration starts with a full tree.
 */
static void BM_ITEB01_SingleKeyErase(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    IntervalTreeIndex tree("bench_erase");
    populateTree(tree, n);

    // Pick a key in the middle of the tree.
    const std::string target = keyName(n / 2);

    for (auto _ : state) {
        // Erase the key.
        benchmark::DoNotOptimize(tree.erase(target));

        // Re-insert so the next iteration finds the key again.
        state.PauseTiming();
        IntervalEntry e;
        e.key       = target;
        e.range     = {static_cast<Timestamp>((n / 2) * kStep),
                       static_cast<Timestamp>((n / 2 + 1) * kStep - 1)};
        e.payload   = {{"i", n / 2}};
        tree.insert(e);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ITEB01_SingleKeyErase)->RangeMultiplier(10)->Range(100, 10000);

// ── ITEB-02: bulk erase all keys ─────────────────────────────────────────────

/**
 * Benchmark: erase every key in the tree sequentially.
 * Measures cumulative cost; tree is rebuilt in each iteration.
 */
static void BM_ITEB02_BulkErase(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        IntervalTreeIndex tree("bench_bulk_erase");
        populateTree(tree, n);
        state.ResumeTiming();

        for (int i = 0; i < n; ++i) {
            benchmark::DoNotOptimize(tree.erase(keyName(i)));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_ITEB02_BulkErase)->RangeMultiplier(10)->Range(100, 10000);

// ── ITEB-03: rebuild baseline ─────────────────────────────────────────────────

/**
 * Baseline benchmark: create a brand-new tree and insert @p n entries.
 * Use this to compare against ITEB-02 (erase vs. rebuild cost trade-off).
 */
static void BM_ITEB03_RebuildBaseline(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    for (auto _ : state) {
        IntervalTreeIndex tree("bench_rebuild");
        populateTree(tree, n);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_ITEB03_RebuildBaseline)->RangeMultiplier(10)->Range(100, 10000);

// ── ITEB-04: mixed read/erase under concurrency ───────────────────────────────

/**
 * Benchmark: 80 % reader threads / 20 % writer (erase+reinsert) threads.
 * Validates that the shared_mutex does not cause excessive reader stalls.
 *
 * Each thread runs its own micro-loop; the benchmark measures throughput
 * (ops/s) seen by the reader threads.
 */
static void BM_ITEB04_MixedReadErase(benchmark::State& state) {
    constexpr int kN = 1000;
    IntervalTreeIndex tree("bench_mixed");
    populateTree(tree, kN);

    std::atomic<bool> running{true};
    std::atomic<uint64_t> read_ops{0};

    // Spawn writer thread (20 % of CPU effort).
    std::thread writer([&] {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, kN - 1);
        while (running.load(std::memory_order_relaxed)) {
            const int idx    = dist(rng);
            const std::string k = keyName(idx);
            tree.erase(k);
            // Re-insert immediately so reads still find entries.
            IntervalEntry e;
            e.key       = k;
            e.range     = {static_cast<Timestamp>(idx * kStep),
                           static_cast<Timestamp>((idx + 1) * kStep - 1)};
            e.payload   = {{"i", idx}};
            tree.insert(e);
        }
    });

    for (auto _ : state) {
        // Reader: point query at a random timestamp.
        const Timestamp t = static_cast<Timestamp>((kN / 4) * kStep + 1);
        benchmark::DoNotOptimize(tree.queryPoint(t));
        ++read_ops;
    }

    running.store(false);
    writer.join();

    state.SetItemsProcessed(static_cast<int64_t>(read_ops.load()));
}
BENCHMARK(BM_ITEB04_MixedReadErase)->Threads(1)->Threads(4)->Threads(8);

// ── ITEB-05: large-payload erase ──────────────────────────────────────────────

/**
 * Benchmark: each entry carries a ~1 KiB JSON blob.
 * Measures the memory-move overhead incurred during erase + rebuild.
 */
static void BM_ITEB05_LargePayloadErase(benchmark::State& state) {
    constexpr int kN = 500;

    // Build a ~1 KiB payload.
    nlohmann::json blob;
    for (int i = 0; i < 40; ++i) {
        blob["field_" + std::to_string(i)] = std::string(24, 'x');
    }

    for (auto _ : state) {
        state.PauseTiming();
        IntervalTreeIndex tree("bench_large");
        for (int i = 0; i < kN; ++i) {
            IntervalEntry e;
            e.key       = keyName(i);
            e.range     = {static_cast<Timestamp>(i * kStep),
                           static_cast<Timestamp>((i + 1) * kStep - 1)};
            e.payload   = blob;
            tree.insert(e);
        }
        state.ResumeTiming();

        for (int i = 0; i < kN; ++i) {
            benchmark::DoNotOptimize(tree.erase(keyName(i)));
        }
    }
    state.SetItemsProcessed(state.iterations() * kN);
}
BENCHMARK(BM_ITEB05_LargePayloadErase);

BENCHMARK_MAIN();

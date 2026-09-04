// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_storage_release_gates.cpp
 * @brief Phase 5 storage module hot-path release-gate benchmarks (SGRG-01..SGRG-06).
 *
 * Provides reproducible throughput and latency measurements for the storage
 * hot paths identified in the storage module roadmap (Phase 5).  Hard release
 * gates — a regression beyond 10 % vs the baseline blocks promotion.
 *
 * ## Gate table
 *
 * | Gate     | Benchmark                              | Threshold          |
 * |----------|----------------------------------------|--------------------|
 * | SGRG-01  | WAL append throughput (in-memory mock) | ≥ 100 k ops/s      |
 * | SGRG-02  | MVCC read (warm cache, single key)     | p99 ≤ 100 µs       |
 * | SGRG-03  | MVCC write (WAL + index update, mock)  | p99 ≤ 500 µs       |
 * | SGRG-04  | Checkpoint overhead (1 k entries)      | p99 ≤ 10 ms        |
 * | SGRG-05  | Tiering decision (in-memory heuristic) | p99 ≤ 50 µs        |
 * | SGRG-06  | Compaction trigger check (no I/O)      | p99 ≤ 100 µs       |
 *
 * All benchmarks:
 *   - Use kStorageCanonicalSeed = 42.
 *   - Run with Repetitions(5).
 *   - No real RocksDB or disk I/O.
 *
 * @see include/storage/storage_api_contract.h — contract thresholds
 * @see src/storage/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "storage/storage_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace sgrg {

using namespace themis::storage;
using namespace std::chrono_literals;

// ============================================================================
// Constants
// ============================================================================

static constexpr std::uint64_t kStorageCanonicalSeed = 42;
static constexpr int           kRepetitions          = 5;
static constexpr int           kWarmupIterations     = 200;

// ============================================================================
// Mock helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Mock WAL ring buffer (no fsync, purely in-memory)
// ---------------------------------------------------------------------------
class BenchWal {
public:
    explicit BenchWal(std::size_t cap = 1 << 20) {
        ring_.resize(cap);
    }

    StorageErrorCode append(const std::string& key, const std::string& value) {
        std::size_t slot = seq_.fetch_add(1, std::memory_order_relaxed) % ring_.size();
        ring_[slot].key   = key;
        ring_[slot].value = value;
        return StorageErrorCode::OK;
    }

    std::uint64_t currentSeq() const { return seq_.load(std::memory_order_relaxed); }

private:
    struct Slot { std::string key, value; };
    std::vector<Slot>        ring_;
    std::atomic<std::uint64_t> seq_{1};
};

// ---------------------------------------------------------------------------
// Mock MVCC store (map from key → versioned value, warm in-memory)
// ---------------------------------------------------------------------------
class BenchMvccStore {
public:
    BenchMvccStore() {
        // Pre-populate with 10 k keys so reads hit the warm path.
        data_.reserve(10'000);
        for (int i = 0; i < 10'000; ++i) {
            data_["k" + std::to_string(i)] = "v" + std::to_string(i);
        }
    }

    std::optional<std::string> read(const std::string& key) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    StorageErrorCode write(BenchWal& wal, const std::string& key, const std::string& value) {
        // WAL append first (write-before-ack contract).
        auto code = wal.append(key, value);
        if (code != StorageErrorCode::OK) {
          return code;
        }
        // Index update after WAL.
        data_[key] = value;
        return StorageErrorCode::OK;
    }

    std::size_t size() const { return data_.size(); }

private:
    std::unordered_map<std::string, std::string> data_;
};

// ---------------------------------------------------------------------------
// Mock checkpoint: iterate entries and mark sequence position
// ---------------------------------------------------------------------------
class BenchCheckpointer {
public:
    StorageErrorCode checkpoint(const BenchMvccStore& store,
                                std::uint64_t         walSeq,
                                std::size_t           maxEntries = 1000) {
        std::size_t processed = 0;
        // Simulate scanning up to maxEntries (no real I/O).
        (void)store.size();
        (void)walSeq;
        for (; processed < maxEntries; ++processed) {
            benchmark::DoNotOptimize(processed);
        }
        return StorageErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Mock tiering decision (hot/warm/cold classification)
// ---------------------------------------------------------------------------
enum class Tier { HOT, WARM, COLD };

struct TierHeuristics {
    std::uint64_t accessCount = {};
    std::chrono::system_clock::time_point lastAccess;
};

static Tier classifyTier(const TierHeuristics& h,
                          std::uint64_t hotThreshold = 1000,
                          std::uint64_t warmThreshold = 100) {
    if (h.accessCount >= hotThreshold) {
      return Tier::HOT;
    }
    if (h.accessCount >= warmThreshold) {
      return Tier::WARM;
    }
    return Tier::COLD;
}

// ---------------------------------------------------------------------------
// Mock compaction trigger check (heuristic, no I/O)
// ---------------------------------------------------------------------------
struct CompactionState {
    std::size_t  pendingVersions{0};
    std::size_t  deletedRatio{0};   ///< Percentage of deleted entries [0..100].
    std::size_t  walLag{0};         ///< WAL entries since last compaction.
};

static bool shouldCompact(const CompactionState& s) {
    return s.pendingVersions > static_cast<std::size_t>(kMvccMaxVersionsPerKey * 0.8)
        || s.deletedRatio > 40
        || s.walLag > 50'000;
}

}  // anonymous namespace

// ============================================================================
// Shared fixtures
// ============================================================================

static BenchWal& sharedWal() {
    static BenchWal wal;
    return wal;
}

static BenchMvccStore& sharedStore() {
    static BenchMvccStore store;
    return store;
}

// ============================================================================
// SGRG-01 — WAL append throughput (in-memory)
// ============================================================================

/**
 * @brief SGRG-01: Append to in-memory ring-buffer WAL.
 *
 * Gate: ≥ 100 k ops/s.
 */
static void BM_SGRG01_WalAppendThroughput(benchmark::State& state) {
    BenchWal wal;
    std::string key("bench-key");
    std::string val("bench-value-payload-42");
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)wal.append(key, val);
    }
    std::int64_t ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(wal.append(key, val));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("SGRG-01: GATE >= 100k ops/s | WAL append throughput");
}
BENCHMARK(BM_SGRG01_WalAppendThroughput)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SGRG-02 — MVCC read (warm cache, single key)
// ============================================================================

/**
 * @brief SGRG-02: Hash-map read on warm MVCC store (10 k entries pre-loaded).
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_SGRG02_MvccReadWarm(benchmark::State& state) {
    const auto& store = sharedStore();
    std::mt19937_64 rng(kStorageCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)store.read("k" + std::to_string(rng() % 10000));
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(store.read("k" + std::to_string(counter % 10000)));
        ++counter;
    }
    state.SetLabel("SGRG-02: GATE p99 <= 100 us | MVCC read warm cache");
}
BENCHMARK(BM_SGRG02_MvccReadWarm)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SGRG-03 — MVCC write (WAL + index update, mock persistence)
// ============================================================================

/**
 * @brief SGRG-03: WAL append + in-memory index update (write-before-ack).
 *
 * Gate: p99 ≤ 500 µs.
 */
static void BM_SGRG03_MvccWrite(benchmark::State& state) {
    BenchWal   wal;
    BenchMvccStore store;
    std::mt19937_64 rng(kStorageCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i) {
        store.write(wal, "wk" + std::to_string(rng() % 1000), "wv");
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            store.write(wal, "wk" + std::to_string(counter % 1000), "wv"));
        ++counter;
    }
    state.SetLabel("SGRG-03: GATE p99 <= 500 us | MVCC write WAL+index");
}
BENCHMARK(BM_SGRG03_MvccWrite)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SGRG-04 — Checkpoint overhead (synthetic 1 k entries)
// ============================================================================

/**
 * @brief SGRG-04: Checkpoint scan of 1 000 entries (no real I/O).
 *
 * Gate: p99 ≤ 10 ms.
 */
static void BM_SGRG04_CheckpointOverhead(benchmark::State& state) {
    BenchCheckpointer cp;
    BenchWal wal;
    BenchMvccStore store;
    for (int i = 0; i < kWarmupIterations / 10; ++i) {
        cp.checkpoint(store, wal.currentSeq(), 1000);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(cp.checkpoint(store, wal.currentSeq(), 1000));
    }
    state.SetLabel("SGRG-04: GATE p99 <= 10 ms | checkpoint 1k entries");
}
BENCHMARK(BM_SGRG04_CheckpointOverhead)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SGRG-05 — Tiering decision (in-memory heuristic)
// ============================================================================

/**
 * @brief SGRG-05: Hot/warm/cold classification based on access-count heuristic.
 *
 * Gate: p99 ≤ 50 µs.
 */
static void BM_SGRG05_TieringDecision(benchmark::State& state) {
    std::mt19937_64 rng(kStorageCanonicalSeed);
    auto now = std::chrono::system_clock::now();
    TierHeuristics h{rng() % 2000, now};
    for (int i = 0; i < kWarmupIterations; ++i) {
        h.accessCount = rng() % 2000;
        (void)classifyTier(h);
    }
    std::uint64_t counter = 0;
    for (auto _ : state) {
        h.accessCount = counter++ % 2000;
        benchmark::DoNotOptimize(classifyTier(h));
    }
    state.SetLabel("SGRG-05: GATE p99 <= 50 us | tiering decision");
}
BENCHMARK(BM_SGRG05_TieringDecision)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SGRG-06 — Compaction trigger check (heuristic, no I/O)
// ============================================================================

/**
 * @brief SGRG-06: shouldCompact() heuristic evaluation (pure computation).
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_SGRG06_CompactionTriggerCheck(benchmark::State& state) {
    std::mt19937_64 rng(kStorageCanonicalSeed);
    CompactionState cs{rng() % 1500, rng() % 60, rng() % 80000};
    for (int i = 0; i < kWarmupIterations; ++i) {
        cs.pendingVersions = rng() % 1500;
        cs.deletedRatio    = rng() % 60;
        cs.walLag          = rng() % 80000;
        (void)shouldCompact(cs);
    }
    std::uint64_t counter = 0;
    for (auto _ : state) {
        cs.pendingVersions = counter % 1500;
        cs.deletedRatio    = counter % 60;
        cs.walLag          = counter % 80000;
        ++counter;
        benchmark::DoNotOptimize(shouldCompact(cs));
    }
    state.SetLabel("SGRG-06: GATE p99 <= 100 us | compaction trigger check");
}
BENCHMARK(BM_SGRG06_CompactionTriggerCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace sgrg
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();

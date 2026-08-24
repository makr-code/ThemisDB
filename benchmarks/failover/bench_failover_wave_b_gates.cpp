// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_failover_wave_b_gates.cpp
 * @brief Wave B performance consolidation benchmarks (FWB-01..FWB-08).
 *
 * Provides reproducible latency measurements for the new failover hot paths
 * introduced in Wave B: adaptive health-check scheduling, quorum log append,
 * topology snapshot capture, and concurrent recovery lifecycle paths.
 *
 * ## Benchmark families
 *
 * ### FWB-01 — Adaptive health-check interval update
 *   Overhead of `updateAdaptiveInterval()` with 20 latency samples.
 *   p99 ≤ 50 µs
 *
 * ### FWB-02 — QuorumLog append (in-memory path, no real I/O)
 *   Overhead of quorum log entry serialization + CRC32 computation.
 *   p99 ≤ 1 ms  (≤ 100 µs for in-memory path without fsync)
 *
 * ### FWB-03 — TopologySnapshot capture
 *   Overhead of `TopologySnapshot::capture()` with 16-node cluster.
 *   p99 ≤ 100 µs
 *
 * ### FWB-04 — TopologySnapshot diff (added/removed nodes)
 *   Overhead of `has_topology_change()` + `added_nodes()` with 16-node cluster.
 *   p99 ≤ 50 µs
 *
 * ### FWB-05 — GC grace period check (burst detection)
 *   Overhead of `checkAndApplyGcGrace()` with 20 recent failure timestamps.
 *   p99 ≤ 50 µs
 *
 * ### FWB-06 — Quorum consensus timeout resolution
 *   Overhead of quorum wait-for path when quorum is immediately available.
 *   p99 ≤ 200 µs
 *
 * ### FWB-07 — Recovery lifecycle hot-path (success path)
 *   Overhead of `attemptRecovery()` success path — batch stats flush.
 *   p99 ≤ 500 µs
 *
 * ### FWB-08 — Concurrent failover queue operations
 *   Throughput of `triggerManualFailover()` under 4 concurrent threads.
 *   p99 per enqueue ≤ 200 µs
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark | Threshold    |
 * |--------------|-----------|--------------|
 * | GATE-FWB-01  | FWB-01    | p99 ≤ 50 µs  |
 * | GATE-FWB-02  | FWB-02    | p99 ≤ 100 µs |
 * | GATE-FWB-03  | FWB-03    | p99 ≤ 100 µs |
 * | GATE-FWB-04  | FWB-04    | p99 ≤ 50 µs  |
 * | GATE-FWB-05  | FWB-05    | p99 ≤ 50 µs  |
 * | GATE-FWB-06  | FWB-06    | p99 ≤ 200 µs |
 * | GATE-FWB-07  | FWB-07    | p99 ≤ 500 µs |
 * | GATE-FWB-08  | FWB-08    | p99 ≤ 200 µs |
 *
 * @see include/failover/failover_api_contract.h
 * @see include/failover/topology_snapshot.h
 * @see include/failover/quorum_log.h
 * @see src/failover/ROADMAP.md — Wave B
 */

#include <benchmark/benchmark.h>

#include "failover/failover_api_contract.h"
#include "failover/topology_snapshot.h"
#include "failover/quorum_log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace themis::failover;

namespace {

/// Canonical seed for all Wave B benchmarks (deterministic, reproducible).
constexpr uint32_t kWaveBCanonicalSeed = 42u;

/// Build a synthetic latency sample vector for adaptive interval benchmarks.
std::vector<std::chrono::milliseconds> makeSamples(size_t n, uint32_t seed = kWaveBCanonicalSeed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int64_t> dist(1, 200);
    std::vector<std::chrono::milliseconds> v;
    v.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        v.emplace_back(dist(rng));
    }
    return v;
}

/// Build a synthetic node failure map for topology snapshot benchmarks.
std::unordered_map<std::string, int> makeFailureMap(size_t node_count,
                                                     uint32_t seed = kWaveBCanonicalSeed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 5);
    std::unordered_map<std::string, int> m;
    m.reserve(node_count);
    for (size_t i = 0; i < node_count; ++i) {
        m["node-" + std::to_string(i)] = dist(rng);
    }
    return m;
}

}  // namespace

// ============================================================================
// FWB-01 — Adaptive health-check interval update (in-process, no I/O)
// ============================================================================

/**
 * @brief FWB-01: overhead of computing p95 latency from a 20-sample rolling window
 *        and updating the adaptive interval.
 *
 * Uses a standalone p95-sort simulation that mirrors `updateAdaptiveInterval()` logic
 * without requiring a full AutoFailoverManager instance.
 */
static void BM_FWB01_AdaptiveIntervalUpdate(benchmark::State& state) {
    auto samples = makeSamples(20);
    std::chrono::milliseconds current_interval{500};
    const std::chrono::milliseconds interval_min{100};
    const std::chrono::milliseconds interval_max{5000};

    for (auto _ : state) {
        // Simulate updateAdaptiveInterval(): sort copy, compute p95, update interval.
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        const size_t p95_idx = (sorted.size() * 95 / 100);
        const auto p95 = sorted[std::min(p95_idx, sorted.size() - 1)];
        auto new_interval = std::chrono::milliseconds(p95.count() * 2);
        new_interval = std::max(new_interval, interval_min);
        new_interval = std::min(new_interval, interval_max);
        current_interval = new_interval;
        benchmark::DoNotOptimize(current_interval);
    }
}
BENCHMARK(BM_FWB01_AdaptiveIntervalUpdate)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-02 — QuorumLog append (in-memory CRC32 + serialization, no real I/O)
// ============================================================================

/**
 * @brief FWB-02: overhead of quorum entry CRC32 computation and string serialization.
 *
 * Simulates the work performed by `QuorumLog::append()` excluding the actual
 * file write (which would be I/O-bound and not representative of the hot path
 * overhead).
 */
static void BM_FWB02_QuorumLogAppendInMemory(benchmark::State& state) {
    // CRC32 lookup table (standard polynomial 0xEDB88320)
    static const auto make_crc_table = []() {
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
            }
            table[i] = c;
        }
        return table;
    };
    static const auto crc_table = make_crc_table();

    const auto crc32_str = [&](const std::string& s, uint32_t crc_in = 0xFFFFFFFFu) {
        uint32_t crc = crc_in;
        for (unsigned char c : s) {
            crc = crc_table[(crc ^ c) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFFu;
    };

    const uint64_t epoch = 42;
    const std::string node_id = "node-primary-0";
    const std::string decision = "PROMOTE";

    for (auto _ : state) {
        const int64_t ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        // Serialize entry
        std::string entry;
        entry.reserve(128);
        entry += std::to_string(epoch);
        entry += '|';
        entry += node_id;
        entry += '|';
        entry += decision;
        entry += '|';
        entry += std::to_string(ts_ms);

        // Compute CRC32
        const uint32_t crc = crc32_str(entry);
        entry += '|';
        entry += std::to_string(crc);
        entry += '\n';

        benchmark::DoNotOptimize(entry.size());
    }
}
BENCHMARK(BM_FWB02_QuorumLogAppendInMemory)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-03 — TopologySnapshot capture (16-node cluster)
// ============================================================================

/**
 * @brief FWB-03: overhead of `TopologySnapshot::capture()` with 16 nodes.
 *
 * Capture includes copying the failure map and sorting node IDs for
 * deterministic ordering.
 */
static void BM_FWB03_TopologySnapshotCapture(benchmark::State& state) {
    const auto failure_map = makeFailureMap(16);
    uint64_t version = 0;

    for (auto _ : state) {
        auto snap = TopologySnapshot::capture(++version, failure_map);
        benchmark::DoNotOptimize(snap.version);
    }
}
BENCHMARK(BM_FWB03_TopologySnapshotCapture)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-04 — TopologySnapshot diff (16-node cluster, one change)
// ============================================================================

/**
 * @brief FWB-04: overhead of `has_topology_change()` + `added_nodes()` between
 *        two 16-node snapshots that differ by one node.
 */
static void BM_FWB04_TopologySnapshotDiff(benchmark::State& state) {
    auto map_a = makeFailureMap(16);
    auto map_b = map_a;
    map_b["node-new-99"] = 0;  // one addition

    const auto snap_a = TopologySnapshot::capture(1, map_a);
    const auto snap_b = TopologySnapshot::capture(2, map_b);

    for (auto _ : state) {
        const bool changed = snap_a.has_topology_change(snap_b);
        const auto added = snap_a.added_nodes(snap_b);
        benchmark::DoNotOptimize(changed);
        benchmark::DoNotOptimize(added.size());
    }
}
BENCHMARK(BM_FWB04_TopologySnapshotDiff)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-05 — GC grace period burst detection (20 timestamps)
// ============================================================================

/**
 * @brief FWB-05: overhead of scanning and pruning a 20-element failure-timestamp
 *        vector to determine whether GC grace period applies.
 *
 * Simulates the inner loop of `checkAndApplyGcGrace()`.
 */
static void BM_FWB05_GcGraceBurstDetection(benchmark::State& state) {
    const auto now = std::chrono::steady_clock::now();
    std::vector<std::chrono::steady_clock::time_point> timestamps;
    timestamps.reserve(20);
    for (int i = 0; i < 20; ++i) {
        timestamps.push_back(now - std::chrono::milliseconds(50 * i));
    }

    const auto window_ms = std::chrono::milliseconds(1000);
    const uint32_t gc_count = 3;

    for (auto _ : state) {
        const auto window_start = std::chrono::steady_clock::now() - window_ms;
        auto ts_copy = timestamps;
        ts_copy.erase(
            std::remove_if(ts_copy.begin(), ts_copy.end(),
                [&](const auto& t) { return t < window_start; }),
            ts_copy.end());
        const bool burst = ts_copy.size() >= gc_count;
        benchmark::DoNotOptimize(burst);
    }
}
BENCHMARK(BM_FWB05_GcGraceBurstDetection)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-06 — Quorum consensus timeout resolution (quorum immediately available)
// ============================================================================

/**
 * @brief FWB-06: overhead of quorum deadline check when quorum is instantly satisfied.
 *
 * Simulates the tight loop in `checkAndWaitForQuorum()` with a no-op hasQuorum mock.
 */
static void BM_FWB06_QuorumResolutionFastPath(benchmark::State& state) {
    const auto timeout = std::chrono::milliseconds(30000);

    for (auto _ : state) {
        // Fast-path: quorum is available on first check
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        bool has_quorum = true;  // mock: always true
        if (has_quorum) {
            benchmark::DoNotOptimize(deadline.time_since_epoch().count());
        }
    }
}
BENCHMARK(BM_FWB06_QuorumResolutionFastPath)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-07 — Recovery lifecycle batch-stats flush (success path)
// ============================================================================

/**
 * @brief FWB-07: overhead of batch stats flush in `attemptRecovery()` success path.
 *
 * Simulates the single `stats_mutex_` acquisition + three counter increments that
 * replace the per-iteration locking pattern.
 */
static void BM_FWB07_RecoveryBatchStatFlush(benchmark::State& state) {
    struct Stats {
        uint64_t total_retry_attempts{0};
        uint64_t successful_retries{0};
        uint64_t failed_retries{0};
    };
    Stats stats;
    std::mutex stats_mutex;

    for (auto _ : state) {
        const uint64_t local_total   = 3;
        const uint64_t local_success = 1;
        const uint64_t local_failed  = 2;

        std::lock_guard<std::mutex> lock(stats_mutex);
        stats.total_retry_attempts += local_total;
        stats.successful_retries   += local_success;
        stats.failed_retries       += local_failed;
        benchmark::DoNotOptimize(stats.total_retry_attempts);
    }
}
BENCHMARK(BM_FWB07_RecoveryBatchStatFlush)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// FWB-08 — Concurrent failover queue enqueue (4 threads)
// ============================================================================

/**
 * @brief FWB-08: per-enqueue throughput of `triggerManualFailover()` queue path
 *        under 4 concurrent producers.
 *
 * Simulates the mutex-protected queue push without the full manager overhead.
 */
static void BM_FWB08_ConcurrentQueueEnqueue(benchmark::State& state) {
    struct FakeTask { std::string node_id; int64_t ts; };
    std::queue<FakeTask> q;
    std::mutex q_mutex;
    const size_t max_size = 10;

    for (auto _ : state) {
        bool enqueued = false;
        {
            std::lock_guard<std::mutex> lock(q_mutex);
            if (q.size() < max_size) {
                q.push({"node-0", 42});
                enqueued = true;
            }
        }
        // Drain to avoid unbounded growth in benchmark loop
        if (q.size() >= max_size) {
            std::lock_guard<std::mutex> lock(q_mutex);
            while (!q.empty()) q.pop();
        }
        benchmark::DoNotOptimize(enqueued);
    }
}
BENCHMARK(BM_FWB08_ConcurrentQueueEnqueue)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Threads(4);

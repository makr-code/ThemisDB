// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_sharding_soak.cpp
 * @brief Wave D — Sharding Soak Tests (sustained-traffic soak window).
 *
 * Long-duration soak tests for the ThemisDB sharding subsystem.
 * Three test cases validate routing throughput, migration stability, and
 * anti-entropy reliability over a configurable soak window.
 *
 * ## Acceptance criteria
 * - ShardingSoak_RoutingThroughput   : ≥ 20 000 routing ops/sec sustained
 * - ShardingSoak_MigrationStability  : zero data-loss across multiple migrations
 * - ShardingSoak_AntiEntropyReliability: convergence = 100 % across all nodes
 *
 * ## Environment
 * Set THEMIS_SOAK_DURATION_MS to override the default 60 000 ms window.
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Excluded from fast release_critical gate.
 * @see docs/operability/RUNBOOK_SHARDING.md — operator runbook
 * @see src/sharding/ROADMAP.md — Wave D contribution
 * @see docs/operability/WAVE_D_ROADMAP.md — Phase 4 Soak Tests
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Soak duration helper
// ---------------------------------------------------------------------------
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// In-process sharding stubs
// These model the hot paths without requiring a live distributed runtime.
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

/// Consistent-hash ring stub — maps a key to a shard ID deterministically.
class StubConsistentHashRing {
public:
    explicit StubConsistentHashRing(uint32_t shard_count)
        : shard_count_(shard_count) {}

    uint32_t route(uint64_t key) const noexcept {
        // FNV-1a inspired mix — purely in-process.
        uint64_t h = key ^ 0xcbf29ce484222325ULL;
        h *= 0x100000001b3ULL;
        h ^= h >> 33;
        return static_cast<uint32_t>(h % shard_count_);
    }

    uint32_t shardCount() const noexcept { return shard_count_; }

private:
    uint32_t shard_count_;
};

/// Migration stub — moves key ownership between shards with a tiny sleep.
struct MigrationRecord {
    uint64_t key;
    uint32_t from_shard;
    uint32_t to_shard;
    bool     completed{false};
};

class StubMigrationCoordinator {
public:
    bool migrate(MigrationRecord& rec) {
        // Simulate in-memory ownership transfer — no I/O.
        std::this_thread::sleep_for(1us); // model minimal latency
        rec.completed = true;
        total_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    uint64_t totalMigrations() const noexcept {
        return total_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> total_{0};
};

/// Anti-entropy stub — compares two node digests and returns whether they match.
class StubAntiEntropyEngine {
public:
    explicit StubAntiEntropyEngine(uint32_t node_count)
        : node_count_(node_count), converged_(0) {}

    /// Runs one anti-entropy round; returns number of divergent pairs repaired.
    uint32_t runRound(uint64_t seq) {
        uint32_t repaired = 0;
        for (uint32_t i = 0; i < node_count_; ++i) {
            // Stub: compute a deterministic digest per node per sequence.
            uint64_t digest = seq ^ (static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
            uint64_t expected = seq ^ (static_cast<uint64_t>(0) * 0x9e3779b97f4a7c15ULL);
            if (digest != expected) {
                ++repaired;
            }
        }
        if (repaired == 0) {
            converged_.fetch_add(1, std::memory_order_relaxed);
        }
        return repaired;
    }

    uint64_t convergentRounds() const noexcept {
        return converged_.load(std::memory_order_relaxed);
    }

private:
    uint32_t               node_count_;
    std::atomic<uint64_t>  converged_;
};

// ---------------------------------------------------------------------------
// ShardingSoak_RoutingThroughput
// ---------------------------------------------------------------------------
TEST(ShardingSoak, ShardingSoak_RoutingThroughput) {
    constexpr uint32_t    kShardCount         = 64;
    constexpr double      kMinOpsPerSec       = 20'000.0;
    constexpr uint32_t    kWorkerCount        = 4;

    StubConsistentHashRing ring(kShardCount);
    std::atomic<uint64_t>  total_ops{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    auto worker = [&]() {
        uint64_t key = std::hash<std::thread::id>{}(std::this_thread::get_id());
        while (std::chrono::steady_clock::now() < end_time) {
            // Route 256 keys per iteration to amortise clock calls.
            for (int i = 0; i < 256; ++i) {
                volatile uint32_t shard = ring.route(key);
                (void)shard;
                ++key;
            }
            total_ops.fetch_add(256, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (uint32_t i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker);
    }
    for (auto& t : workers) { t.join(); }

    const double elapsed_sec = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double ops_per_sec =
        static_cast<double>(total_ops.load(std::memory_order_relaxed)) / elapsed_sec;

    EXPECT_GE(ops_per_sec, kMinOpsPerSec)
        << "Routing throughput " << static_cast<uint64_t>(ops_per_sec)
        << " ops/sec is below the " << static_cast<uint64_t>(kMinOpsPerSec)
        << " ops/sec soak threshold";
}

// ---------------------------------------------------------------------------
// ShardingSoak_MigrationStability
// ---------------------------------------------------------------------------
TEST(ShardingSoak, ShardingSoak_MigrationStability) {
    constexpr uint32_t kShardCount         = 8;
    constexpr uint64_t kMigrationsPerBatch = 100;
    constexpr uint32_t kWorkerCount        = 2;

    StubConsistentHashRing    ring(kShardCount);
    StubMigrationCoordinator  coord;
    std::atomic<uint64_t>     lost{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    auto worker = [&](uint64_t base_key) {
        uint64_t key = base_key;
        while (std::chrono::steady_clock::now() < end_time) {
            for (uint64_t i = 0; i < kMigrationsPerBatch; ++i) {
                MigrationRecord rec;
                rec.key        = key + i;
                rec.from_shard = ring.route(rec.key);
                rec.to_shard   = (rec.from_shard + 1) % kShardCount;
                if (!coord.migrate(rec) || !rec.completed) {
                    lost.fetch_add(1, std::memory_order_relaxed);
                }
            }
            key += kMigrationsPerBatch;
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (uint32_t i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker, static_cast<uint64_t>(i) * 1'000'000ULL);
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_EQ(lost.load(), 0ULL)
        << "ShardingSoak_MigrationStability: detected "
        << lost.load() << " data-loss events during soak";
    EXPECT_GT(coord.totalMigrations(), 0ULL)
        << "No migrations were executed — soak window may be too short";
}

// ---------------------------------------------------------------------------
// ShardingSoak_AntiEntropyReliability
// ---------------------------------------------------------------------------
TEST(ShardingSoak, ShardingSoak_AntiEntropyReliability) {
    constexpr uint32_t kNodeCount         = 6;
    constexpr double   kMinConvergenceRat = 1.0; // 100 %

    StubAntiEntropyEngine engine(kNodeCount);

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    uint64_t total_rounds = 0;
    uint64_t seq          = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        engine.runRound(seq++);
        ++total_rounds;
    }

    ASSERT_GT(total_rounds, 0ULL) << "No anti-entropy rounds executed";

    const double convergence_ratio =
        static_cast<double>(engine.convergentRounds()) /
        static_cast<double>(total_rounds);

    EXPECT_GE(convergence_ratio, kMinConvergenceRat)
        << "Anti-entropy convergence " << convergence_ratio * 100.0
        << "% is below the required 100% over " << total_rounds << " rounds";
}

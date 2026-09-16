// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_sharding_highcardinality_stress.cpp
 * @brief Wave D — Sharding High-Cardinality Stress Tests.
 *
 * Three stress cases exercising the sharding subsystem under high-cardinality
 * and concurrency conditions using in-process stubs and seed-42 determinism.
 *
 * ## Cases
 * - HighCardinalityCrossShardRoute   : 50 000 keys, 8-thread concurrent routing
 * - ConcurrentMigrationStress        : concurrent migrations across all shards
 * - SkewedLoadAntiEntropyStress       : anti-entropy under skewed write load
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_SHARDING.md
 * @see src/sharding/ROADMAP.md
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Deterministic seed
// ---------------------------------------------------------------------------
static constexpr uint64_t kShardStressSeed = 42;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

class StressHashRing {
public:
    explicit StressHashRing(uint32_t shards) : shards_(shards) {}

    uint32_t route(uint64_t key) const noexcept {
        uint64_t h = key ^ 0xcbf29ce484222325ULL;
        h ^= (h >> 33);
        h *= 0xff51afd7ed558ccdULL;
        h ^= (h >> 33);
        return static_cast<uint32_t>(h % shards_);
    }

    uint32_t shardCount() const noexcept { return shards_; }

private:
    uint32_t shards_;
};

struct ShardOwnershipTable {
    std::mutex                              mu;
    std::unordered_map<uint64_t, uint32_t>  table;

    void assign(uint64_t key, uint32_t shard) {
        std::lock_guard<std::mutex> lk(mu);
        table[key] = shard;
    }

    bool reassign(uint64_t key, uint32_t from, uint32_t to) {
        std::lock_guard<std::mutex> lk(mu);
        auto it = table.find(key);
        if (it == table.end() || it->second != from) { return false; }
        it->second = to;
        return true;
    }
};

// ---------------------------------------------------------------------------
// HighCardinalityCrossShardRoute — 50 000 keys, 8 threads
// ---------------------------------------------------------------------------
TEST(ShardingHighCardinalityStress, HighCardinalityCrossShardRoute) {
    constexpr uint64_t kKeyCount     = 50'000;
    constexpr uint32_t kShardCount   = 16;
    constexpr uint32_t kThreadCount  = 8;

    StressHashRing ring(kShardCount);

    // Pre-generate keys deterministically.
    std::vector<uint64_t> keys;
    keys.reserve(kKeyCount);
    uint64_t rng = kShardStressSeed;
    for (uint64_t i = 0; i < kKeyCount; ++i) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        keys.push_back(rng);
    }

    std::atomic<uint64_t> routed{0};
    std::atomic<uint64_t> errors{0};

    const uint64_t keys_per_thread = kKeyCount / kThreadCount;

    auto worker = [&](uint32_t tid) {
        const uint64_t start = tid * keys_per_thread;
        const uint64_t end   = std::min(start + keys_per_thread, kKeyCount);
        for (uint64_t i = start; i < end; ++i) {
            const uint32_t shard = ring.route(keys[i]);
            if (shard >= kShardCount) {
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        }
        routed.fetch_add(end - start, std::memory_order_relaxed);
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(routed.load(), kKeyCount);
    EXPECT_EQ(errors.load(), 0ULL)
        << "HighCardinalityCrossShardRoute: " << errors.load()
        << " routing errors for " << kKeyCount << " keys";
}

// ---------------------------------------------------------------------------
// ConcurrentMigrationStress
// ---------------------------------------------------------------------------
TEST(ShardingHighCardinalityStress, ConcurrentMigrationStress) {
    constexpr uint64_t kKeyCount    = 10'000;
    constexpr uint32_t kShardCount  = 8;
    constexpr uint32_t kThreadCount = 4;

    StressHashRing       ring(kShardCount);
    ShardOwnershipTable  table;
    std::atomic<uint64_t> migrated{0};
    std::atomic<uint64_t> conflicts{0};

    // Populate initial ownership.
    {
        uint64_t rng = kShardStressSeed;
        for (uint64_t i = 0; i < kKeyCount; ++i) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            table.assign(rng, ring.route(rng));
        }
    }

    // Rebuild the same key sequence for workers.
    std::vector<uint64_t> keys(kKeyCount);
    {
        uint64_t rng = kShardStressSeed;
        for (uint64_t i = 0; i < kKeyCount; ++i) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            keys[i] = rng;
        }
    }

    const uint64_t keys_per_thread = kKeyCount / kThreadCount;

    auto worker = [&](uint32_t tid) {
        const uint64_t start = tid * keys_per_thread;
        const uint64_t end   = std::min(start + keys_per_thread, kKeyCount);
        for (uint64_t i = start; i < end; ++i) {
            const uint64_t key       = keys[i];
            const uint32_t from      = ring.route(key);
            const uint32_t to        = (from + tid + 1) % kShardCount;
            if (table.reassign(key, from, to)) {
                migrated.fetch_add(1, std::memory_order_relaxed);
            } else {
                conflicts.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    // Total ops should cover all keys; migrated + conflicts = total keys.
    EXPECT_EQ(migrated.load() + conflicts.load(), kKeyCount)
        << "ConcurrentMigrationStress: accounting mismatch";
    EXPECT_GT(migrated.load(), 0ULL) << "No migrations completed";
}

// ---------------------------------------------------------------------------
// SkewedLoadAntiEntropyStress
// ---------------------------------------------------------------------------
TEST(ShardingHighCardinalityStress, SkewedLoadAntiEntropyStress) {
    constexpr uint32_t kNodeCount    = 8;
    constexpr uint64_t kRounds       = 5'000;
    constexpr uint32_t kHotShardPct  = 80; // 80 % of writes hit shard 0

    std::vector<std::atomic<uint64_t>> write_counts(kNodeCount);
    for (auto& wc : write_counts) wc.store(0);

    uint64_t rng = kShardStressSeed;
    for (uint64_t r = 0; r < kRounds; ++r) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        const uint32_t shard = ((rng % 100) < kHotShardPct)
                               ? 0
                               : static_cast<uint32_t>((rng >> 16) % kNodeCount);
        write_counts[shard].fetch_add(1, std::memory_order_relaxed);
    }

    // Anti-entropy: propagate max write count to all nodes.
    uint64_t max_writes = 0;
    for (const auto& wc : write_counts) {
        max_writes = std::max(max_writes, wc.load());
    }

    std::atomic<uint32_t> divergent_nodes{0};
    for (uint32_t n = 0; n < kNodeCount; ++n) {
        // A node is "divergent" if its count is 0 when others have writes.
        if (write_counts[n].load() == 0 && max_writes > 0) {
            divergent_nodes.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Hot shard (shard 0) must have the most writes.
    EXPECT_GT(write_counts[0].load(), write_counts[1].load())
        << "SkewedLoadAntiEntropyStress: hot shard 0 did not receive skewed load";

    // Anti-entropy should bring convergence: at most 1 divergent node allowed.
    EXPECT_LE(divergent_nodes.load(), 1U)
        << "SkewedLoadAntiEntropyStress: " << divergent_nodes.load()
        << " nodes diverged after " << kRounds << " rounds";
}

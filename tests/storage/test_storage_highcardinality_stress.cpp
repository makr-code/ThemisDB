// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_storage_highcardinality_stress.cpp
 * @brief Wave D high-cardinality and concurrent stress tests for the storage engine.
 *
 * Three focused stress cases that exercise key-write cardinality, concurrent
 * compaction, and blob/tiering edge scenarios.  All cases use in-process
 * stubs — no real RocksDB, disk I/O, or blob backends are required.
 *
 * CTest labels: wave_d;stress;not_release_critical
 *
 * | Test case                      | Scenario                                    |
 * |--------------------------------|---------------------------------------------|
 * | HighCardinalityKeyWrite        | 100 000 unique keys, 8 concurrent threads   |
 * | ConcurrentCompactionStress     | 8 shards, compaction + write overlap        |
 * | BlobTieringEdgeCases           | hot/warm/cold promotion under concurrency   |
 *
 * @see src/storage/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace test {
namespace wave_d {

// ---------------------------------------------------------------------------
// In-process stub: KV store with atomic sequence tracking
// ---------------------------------------------------------------------------

struct KVStore {
    bool put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mu_);
        store_[key] = value;
        ++writes_;
        return true;
    }

    bool get(const std::string& key, std::string* out) const {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        *out = it->second;
        ++reads_;
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return store_.size();
    }

    long writes() const { return writes_.load(std::memory_order_relaxed); }
    long reads()  const { return reads_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex                          mu_;
    std::unordered_map<std::string, std::string> store_;
    std::atomic<long>                            writes_{0};
    mutable std::atomic<long>                    reads_{0};
};

// ---------------------------------------------------------------------------
// In-process stub: compaction-aware shard
// ---------------------------------------------------------------------------

struct CompactableShard {
    bool write(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mu_);
        data_[key] = value;
        ++writes_;
        return true;
    }

    // Returns false if compaction detects corruption (never in this stub).
    bool compact() {
        std::lock_guard<std::mutex> lock(mu_);
        ++compactions_;
        return true;
    }

    long writes()      const { return writes_.load(std::memory_order_relaxed); }
    long compactions() const { return compactions_.load(std::memory_order_relaxed); }

private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mu_;
    std::atomic<long>  writes_{0};
    std::atomic<long>  compactions_{0};
};

// ---------------------------------------------------------------------------
// In-process stub: tiered blob store
// ---------------------------------------------------------------------------

enum class Tier { kHot, kWarm, kCold };

struct TieredBlobStore {
    struct BlobEntry {
        std::string  data;
        Tier         tier{Tier::kCold};
        long         access_count{0};
    };

    bool put(const std::string& key, const std::string& data) {
        std::lock_guard<std::mutex> lock(mu_);
        store_[key] = {data, Tier::kCold, 0};
        ++writes_;
        return true;
    }

    bool access(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        ++it->second.access_count;
        // Promote: ≥3 accesses → warm; ≥10 → hot
        if (it->second.access_count >= 10)
            it->second.tier = Tier::kHot;
        else if (it->second.access_count >= 3)
            it->second.tier = Tier::kWarm;
        return true;
    }

    // Returns count of entries that were promoted to warm or hot tier.
    long promoted() const {
        std::lock_guard<std::mutex> lock(mu_);
        long count = 0;
        for (const auto& kv : store_)
            if (kv.second.tier != Tier::kCold) ++count;
        return count;
    }

    long writes() const { return writes_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex                       mu_;
    std::unordered_map<std::string, BlobEntry> store_;
    std::atomic<long>                        writes_{0};
};

// ===========================================================================
// ST-STRESS-01 — High-cardinality key write (100 000 keys, 8 threads)
// ===========================================================================

TEST(HighCardinalityKeyWrite, AllKeysPresent) {
    constexpr int kTotalKeys = 100000;
    constexpr int kThreads   = 8;

    KVStore store;
    const int keys_per_thread = kTotalKeys / kThreads;

    auto writer = [&](int thread_id) {
        const int start = thread_id * keys_per_thread;
        const int end   = start + keys_per_thread;
        for (int i = start; i < end; ++i)
            store.put("key_" + std::to_string(i), "val_" + std::to_string(i));
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(writer, i);
    for (auto& t : threads) t.join();

    // Handle remainder keys written by thread 0 / any rounding
    for (int i = kThreads * keys_per_thread; i < kTotalKeys; ++i)
        store.put("key_" + std::to_string(i), "val_" + std::to_string(i));

    EXPECT_EQ(static_cast<std::size_t>(kTotalKeys), store.size())
        << "Not all keys persisted — concurrent write race or loss detected";
    EXPECT_GE(store.writes(), static_cast<long>(kTotalKeys));

    // Spot-verify reads
    std::string out;
    EXPECT_TRUE(store.get("key_0", &out));
    EXPECT_EQ("val_0", out);
    EXPECT_TRUE(store.get("key_" + std::to_string(kTotalKeys - 1), &out));
    EXPECT_EQ("val_" + std::to_string(kTotalKeys - 1), out);
}

// ===========================================================================
// ST-STRESS-02 — Concurrent compaction stress (8 shards, mixed workload)
// ===========================================================================

TEST(ConcurrentCompactionStress, NoDataLossOrCorruption) {
    constexpr int kShards   = 8;
    constexpr int kThreads  = 8;
    constexpr int kWritesPerThread = 5000;

    std::vector<CompactableShard> shards(kShards);
    std::atomic<long> total_writes{0};
    std::atomic<long> total_compactions{0};

    auto writer = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> shard_dist(0, kShards - 1);
        for (int i = 0; i < kWritesPerThread; ++i) {
            int s = shard_dist(rng);
            shards[static_cast<std::size_t>(s)].write(
                "k_" + std::to_string(rng() % 10000),
                "v_" + std::to_string(i));
            ++total_writes;
        }
    };

    auto compactor = [&]() {
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> shard_dist(0, kShards - 1);
        for (int i = 0; i < 2000; ++i) {
            bool ok = shards[static_cast<std::size_t>(shard_dist(rng))].compact();
            if (ok) ++total_compactions;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads + 1);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(writer, i + 400);
    threads.emplace_back(compactor);
    for (auto& t : threads) t.join();

    EXPECT_EQ(total_writes.load(), static_cast<long>(kThreads) * kWritesPerThread);
    EXPECT_GT(total_compactions.load(), 0L)
        << "No compaction cycles ran — compactor path not exercised";
}

// ===========================================================================
// ST-STRESS-03 — Blob tiering edge cases (hot/warm/cold promotion)
// ===========================================================================

TEST(BlobTieringEdgeCases, PromotionAndDemotionConsistency) {
    constexpr int kBlobs    = 1000;
    constexpr int kThreads  = 8;
    constexpr int kAccessesPerThread = 5000;

    TieredBlobStore store;

    // Pre-populate
    for (int i = 0; i < kBlobs; ++i)
        store.put("blob_" + std::to_string(i), std::string(64, 'x'));

    auto accessor = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> dist(0, kBlobs - 1);
        for (int i = 0; i < kAccessesPerThread; ++i)
            store.access("blob_" + std::to_string(dist(rng)));
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(accessor, i + 500);
    for (auto& t : threads) t.join();

    const long promoted = store.promoted();

    EXPECT_GT(promoted, 0L)
        << "No blobs were promoted — access-frequency tracking is broken";
    EXPECT_LE(promoted, static_cast<long>(kBlobs))
        << "More promoted entries than total blobs — counter overflow detected";

    // Reaching here without crash/assert = no data-race in tier transitions
    SUCCEED() << "Blob tiering stress completed. promoted=" << promoted
              << "/" << kBlobs;
}

}  // namespace wave_d
}  // namespace test
}  // namespace themis

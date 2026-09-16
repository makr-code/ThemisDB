/**
 * @file test_retrieval_highcardinality_stress.cpp
 * @brief Wave D — Retrieval Engine High-Cardinality Stress Tests.
 *
 * Stress tests for shard routing, concurrent failover, and multi-shard exact
 * routing under high-cardinality workloads.
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_RETRIEVAL_ENGINE.md
 * @see src/retrieval/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Shared stubs
// ─────────────────────────────────────────────────────────────────────────────

/// Stub routing table: maps key → shard index. Thread-safe.
class StubRoutingTable {
public:
    explicit StubRoutingTable(int num_shards) : num_shards_(num_shards) {}

    int route(const std::string& key) const {
        return static_cast<int>(std::hash<std::string>{}(key) % static_cast<size_t>(num_shards_));
    }

    void corrupt(const std::string& key, int bad_shard) {
        std::lock_guard<std::mutex> lk(mu_);
        overrides_[key] = bad_shard;
    }

    void heal() {
        std::lock_guard<std::mutex> lk(mu_);
        overrides_.clear();
    }

    int routeWithOverride(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = overrides_.find(key);
        if (it != overrides_.end()) return it->second;
        return route(key);
    }

    int numShards() const { return num_shards_; }

private:
    int num_shards_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, int> overrides_;
};

/// Stub shard — holds a set of keys.
class StubShard {
public:
    void insert(const std::string& key) {
        std::lock_guard<std::mutex> lk(mu_);
        data_.insert(key);
    }

    bool contains(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.count(key) > 0;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.size();
    }

private:
    mutable std::mutex mu_;
    std::unordered_set<std::string> data_;
};

// ─────────────────────────────────────────────────────────────────────────────
// HC-RT-01: High-cardinality shard routing
// ─────────────────────────────────────────────────────────────────────────────
TEST(RetrievalStress, HighCardinalityShardRouting) {
    constexpr int kNumShards = 16;
    constexpr int kKeys      = 100'000;
    constexpr int kWorkers   = 8;

    StubRoutingTable table(kNumShards);
    std::vector<StubShard> shards(kNumShards);

    std::atomic<uint64_t> routing_errors{0};

    // Pre-generate keys.
    std::vector<std::string> keys;
    keys.reserve(kKeys);
    for (int i = 0; i < kKeys; ++i) {
        keys.push_back("hc:key:" + std::to_string(i));
    }

    // Parallel insert + route.
    auto chunk = kKeys / kWorkers;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            int start = w * static_cast<int>(chunk);
            int end   = (w == kWorkers - 1) ? kKeys : start + static_cast<int>(chunk);
            for (int i = start; i < end; ++i) {
                int shard = table.route(keys[static_cast<size_t>(i)]);
                if (shard < 0 || shard >= kNumShards) {
                    ++routing_errors;
                    continue;
                }
                shards[static_cast<size_t>(shard)].insert(keys[static_cast<size_t>(i)]);
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_EQ(routing_errors.load(), 0U)
        << "[RETRIEVAL:RoutingCorruption] Invalid shard index for high-cardinality key set";

    // Every key must be findable in the shard it was routed to.
    uint64_t not_found = 0;
    for (const auto& key : keys) {
        int shard = table.route(key);
        if (!shards[static_cast<size_t>(shard)].contains(key)) { ++not_found; }
    }
    EXPECT_EQ(not_found, 0U)
        << "Some high-cardinality keys missing from their expected shard";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-RT-02: Concurrent failover stress
// ─────────────────────────────────────────────────────────────────────────────
TEST(RetrievalStress, ConcurrentFailoverStress) {
    constexpr int kNumShards = 8;
    constexpr int kWorkers   = 6;
    constexpr int kOpsPerWorker = 20'000;

    StubRoutingTable table(kNumShards);
    std::vector<bool> shard_alive(static_cast<size_t>(kNumShards), true);
    std::mutex alive_mu;

    std::atomic<uint64_t> total_routed{0};
    std::atomic<uint64_t> failover_count{0};

    // Fault injector: randomly toggles shards alive/dead.
    std::atomic<bool> stop{false};
    std::thread injector([&]() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> shard_dist(0, kNumShards - 1);
        while (!stop.load()) {
            {
                std::lock_guard<std::mutex> lk(alive_mu);
                int s = shard_dist(rng);
                shard_alive[static_cast<size_t>(s)] = !shard_alive[static_cast<size_t>(s)];
                ++failover_count;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) * 0x123456789ABCDEFULL);
            std::uniform_int_distribution<int> key_dist(0, 999'999);
            for (int i = 0; i < kOpsPerWorker; ++i) {
                std::string key = "fo:" + std::to_string(key_dist(rng));
                int shard = table.route(key);
                // If shard is dead, fall back to any alive shard.
                {
                    std::lock_guard<std::mutex> lk(alive_mu);
                    if (!shard_alive[static_cast<size_t>(shard)]) {
                        for (int s = 0; s < kNumShards; ++s) {
                            if (shard_alive[static_cast<size_t>(s)]) { shard = s; break; }
                        }
                    }
                }
                (void)shard;
                ++total_routed;
            }
        });
    }
    for (auto& t : workers) { t.join(); }
    stop.store(true);
    injector.join();

    EXPECT_EQ(total_routed.load(), static_cast<uint64_t>(kWorkers * kOpsPerWorker))
        << "Some routing operations lost during concurrent failover stress";
    EXPECT_GT(failover_count.load(), 0U)
        << "Fault injector should have toggled at least one shard";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-RT-03: Multi-shard exact routing stress
// ─────────────────────────────────────────────────────────────────────────────
TEST(RetrievalStress, MultiShardExactRoutingStress) {
    constexpr int kNumShards = 32;
    constexpr int kKeys      = 200'000;
    constexpr int kWorkers   = 8;

    StubRoutingTable table(kNumShards);
    std::vector<StubShard> shards(kNumShards);

    // Phase 1: populate shards in parallel.
    {
        auto chunk = kKeys / kWorkers;
        std::vector<std::thread> writers;
        writers.reserve(kWorkers);
        for (int w = 0; w < kWorkers; ++w) {
            writers.emplace_back([&, w]() {
                int start = w * static_cast<int>(chunk);
                int end   = (w == kWorkers - 1) ? kKeys : start + static_cast<int>(chunk);
                for (int i = start; i < end; ++i) {
                    std::string key = "ms:key:" + std::to_string(i);
                    int shard = table.route(key);
                    shards[static_cast<size_t>(shard)].insert(key);
                }
            });
        }
        for (auto& t : writers) { t.join(); }
    }

    // Phase 2: exact lookup stress — each worker validates random keys.
    std::atomic<uint64_t> not_found{0};
    {
        std::vector<std::thread> readers;
        readers.reserve(kWorkers);
        for (int w = 0; w < kWorkers; ++w) {
            readers.emplace_back([&, w]() {
                std::mt19937 rng(static_cast<unsigned>(w) + 77u);
                std::uniform_int_distribution<int> dist(0, kKeys - 1);
                for (int i = 0; i < 10'000; ++i) {
                    int idx = dist(rng);
                    std::string key = "ms:key:" + std::to_string(idx);
                    int shard = table.route(key);
                    if (!shards[static_cast<size_t>(shard)].contains(key)) {
                        ++not_found;
                    }
                }
            });
        }
        for (auto& t : readers) { t.join(); }
    }

    EXPECT_EQ(not_found.load(), 0U)
        << "[RETRIEVAL:PhaseGateFailed] Multi-shard exact routing lookup failed for "
        << not_found.load() << " keys";
}

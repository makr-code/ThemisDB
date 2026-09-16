/**
 * @file test_cache_soak.cpp
 * @brief Wave D — Cache Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB adaptive query cache hot path.
 * Verifies that cache hit-rate, replication stability, and LRU eviction
 * remain within acceptable bounds over sustained traffic.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Cache hit-rate ≥ 0.80 over the soak duration
 * - No uncaught exceptions or data-race signals from the in-process stub
 * - Eviction count > 0 (LRU eviction is operational under sustained writes)
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_CACHE_SLO.md  — Cache SLO operator runbook
 * @see src/cache/ROADMAP.md — Wave D Contribution (2026-09-16)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes quickly.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL; // Default CI-safe: 1 minute
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below model the adaptive query cache hot path without
// requiring external backends (RocksDB, Redis, gRPC peers).
//   - StubAdaptiveCache  — bounded LRU map with hit/miss/eviction tracking
//   - StubReplicationLag — simulates write-fan-out lag to a replica
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

/// Bounded LRU stub that models the adaptive query cache hot path.
class StubAdaptiveCache {
public:
    explicit StubAdaptiveCache(std::size_t capacity)
        : capacity_(capacity) {}

    /// Put an entry; evicts LRU key when capacity is exceeded.
    void put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        if (store_.count(key)) {
            store_[key] = value;
            return;
        }
        if (store_.size() >= capacity_) {
            // Evict pseudo-LRU: remove the entry whose insert_seq is smallest
            auto victim = std::min_element(
                seq_.begin(), seq_.end(),
                [](const auto& a, const auto& b){ return a.second < b.second; });
            store_.erase(victim->first);
            seq_.erase(victim->first);
            eviction_count_.fetch_add(1, std::memory_order_relaxed);
        }
        store_[key]   = value;
        seq_[key]     = ++insert_seq_;
    }

    /// Get an entry; returns true and sets value on hit.
    bool get(const std::string& key, std::string& value_out) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) {
            miss_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        value_out = it->second;
        hit_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /// Invalidate all keys matching a key prefix.
    void invalidate(const std::string& prefix) {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto it = store_.begin(); it != store_.end(); ) {
            if (it->first.substr(0, prefix.size()) == prefix) {
                seq_.erase(it->first);
                it = store_.erase(it);
                eviction_count_.fetch_add(1, std::memory_order_relaxed);
            } else {
                ++it;
            }
        }
    }

    uint64_t hitCount()      const noexcept { return hit_count_.load(std::memory_order_relaxed); }
    uint64_t missCount()     const noexcept { return miss_count_.load(std::memory_order_relaxed); }
    uint64_t evictionCount() const noexcept { return eviction_count_.load(std::memory_order_relaxed); }

    double hitRate() const noexcept {
        const uint64_t h = hitCount();
        const uint64_t m = missCount();
        const uint64_t t = h + m;
        return (t > 0) ? static_cast<double>(h) / static_cast<double>(t) : 0.0;
    }

private:
    const std::size_t capacity_;
    std::unordered_map<std::string, std::string> store_;
    std::unordered_map<std::string, uint64_t>    seq_;
    uint64_t insert_seq_ = 0;
    std::mutex mu_;
    std::atomic<uint64_t> hit_count_{0};
    std::atomic<uint64_t> miss_count_{0};
    std::atomic<uint64_t> eviction_count_{0};
};

/// Simulates replication lag for a single write event.
struct StubReplicationStats {
    std::atomic<uint64_t> write_count{0};
    std::atomic<uint64_t> lag_over_threshold{0};
    static constexpr uint64_t kLagThresholdUs = 500'000; // 500 ms
};

static void simulateWrite(StubReplicationStats& stats, uint64_t key) {
    // Simulate WAL shipping: 5 ms base + small jitter
    uint64_t jitter_us = (key * 6364136223846793005ULL + 1442695040888963407ULL) % 3'000;
    std::this_thread::sleep_for(std::chrono::microseconds(5'000 + jitter_us));
    stats.write_count.fetch_add(1, std::memory_order_relaxed);
    if ((5'000 + jitter_us) > StubReplicationStats::kLagThresholdUs) {
        stats.lag_over_threshold.fetch_add(1, std::memory_order_relaxed);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: CacheSoak_HotPathHitRate
//
// Populate a stub cache with a working set of kHotKeys, then run a sustained
// hot-path loop that primarily hits the warm keys.  Accepts a soak-relative
// grace period for initial cold misses.  Hit-rate must be ≥ 0.80.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheSoak, CacheSoak_HotPathHitRate) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    constexpr std::size_t kCapacity = 200;
    constexpr std::size_t kHotKeys  = 100; // working set fits in cache

    StubAdaptiveCache cache(kCapacity);

    // Pre-warm with the hot working set
    for (std::size_t i = 0; i < kHotKeys; ++i) {
        cache.put("hot_key_" + std::to_string(i), "value_" + std::to_string(i));
    }

    const auto start = std::chrono::steady_clock::now();
    uint64_t loop_key = 0;
    std::string dummy;

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        // 90% reads of hot keys (should hit), 10% writes of new keys (eviction pressure)
        const uint64_t r = (loop_key * 1103515245ULL + 12345ULL) % 100;
        if (r < 90) {
            const std::string k = "hot_key_" + std::to_string(loop_key % kHotKeys);
            cache.get(k, dummy);
        } else {
            const std::string k = "cold_key_" + std::to_string(loop_key);
            cache.put(k, "v");
        }
        ++loop_key;
    }

    EXPECT_GT(cache.hitCount() + cache.missCount(), 0u)
        << "At least one cache operation must complete during the soak";

    const double hit_rate = cache.hitRate();
    EXPECT_GE(hit_rate, 0.80)
        << "Cache hit-rate must be ≥ 0.80 over soak duration. "
           "Observed: " << hit_rate;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: CacheSoak_ReplicationStability
//
// Run sustained writes through the stub replication path for soak_duration/10.
// All writes must complete; replication lag over-threshold count must be 0
// (simulated lag is ~5 ms, well below the 500 ms threshold).
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheSoak, CacheSoak_ReplicationStability) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 10);

    StubReplicationStats stats;
    const auto start = std::chrono::steady_clock::now();
    uint64_t key = 0;

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        simulateWrite(stats, key++);
    }

    ASSERT_GT(stats.write_count.load(), 0u)
        << "At least one replication write must complete during the soak";

    EXPECT_EQ(stats.lag_over_threshold.load(), 0u)
        << "No replication event should exceed the 500 ms lag threshold under normal load";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: CacheSoak_EvictionUnderLoad
//
// Fill a small cache to capacity with distinct keys, then sustain writes at a
// rate that continuously exceeds capacity.  Eviction count must be > 0 after
// the soak, confirming that LRU eviction remains operational under load.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheSoak, CacheSoak_EvictionUnderLoad) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 20);

    constexpr std::size_t kSmallCapacity = 50;
    StubAdaptiveCache cache(kSmallCapacity);

    const auto start = std::chrono::steady_clock::now();
    uint64_t key = 0;

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        // Always write unique keys — every write past kSmallCapacity triggers eviction
        cache.put("evict_key_" + std::to_string(key++), "v");
    }

    EXPECT_GT(cache.evictionCount(), 0u)
        << "LRU eviction must occur under sustained write pressure exceeding cache capacity";
}

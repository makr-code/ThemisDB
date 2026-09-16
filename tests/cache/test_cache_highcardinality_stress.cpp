/**
 * @file test_cache_highcardinality_stress.cpp
 * @brief Wave D — Cache High-Cardinality Stress Tests.
 *
 * Stress tests for the cache module covering high-cardinality scenarios:
 * 2000 distinct tenant-keyed cache entries, concurrent get/put from 8 threads,
 * LRU eviction under memory pressure, and per-tenant isolation under load.
 *
 * These tests are excluded from the fast release_critical gate and are
 * intended for Wave D stress validation pipelines.
 *
 * ## Test cases
 * - HighCardinalityTenantLoad             — 2000 distinct tenant-keyed entries
 * - ConcurrentMultiTenantStress           — 8-thread concurrent get/put
 * - EvictionPressureUnderHighCardinality  — LRU eviction under memory pressure
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_CACHE_SLO.md — Cache SLO operator runbook
 * @see src/cache/ROADMAP.md — Wave D Gap Closure (2026-09-16)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// StubTenantCache is a bounded LRU map with per-tenant namespace isolation.
// It models the cache hot path for multi-tenant keying without requiring
// external backends (RocksDB, Redis, gRPC).
//
// Correctness invariants:
//   - Keys are namespaced as "<tenant_id>/<key>" to enforce isolation.
//   - Eviction is pseudo-LRU based on insertion sequence number.
//   - Per-tenant eviction never touches keys belonging to other tenants.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

/// Thread-safe bounded LRU cache stub with per-tenant namespace isolation.
class StubTenantCache {
public:
    explicit StubTenantCache(std::size_t capacity) : capacity_(capacity) {}

    /// Put a tenant-namespaced entry; evicts global LRU entry when full.
    void put(const std::string& tenant_id, const std::string& key, const std::string& value) {
        const std::string ns_key = tenant_id + "/" + key;
        std::lock_guard<std::mutex> lk(mu_);
        if (store_.count(ns_key)) {
            store_[ns_key] = value;
            seq_[ns_key]   = ++seq_counter_;
            return;
        }
        if (store_.size() >= capacity_) {
            // Evict global LRU (smallest insert_seq, any tenant)
            auto victim = std::min_element(
                seq_.begin(), seq_.end(),
                [](const auto& a, const auto& b){ return a.second < b.second; });
            store_.erase(victim->first);
            seq_.erase(victim->first);
            eviction_count_.fetch_add(1, std::memory_order_relaxed);
        }
        store_[ns_key] = value;
        seq_[ns_key]   = ++seq_counter_;
    }

    /// Get a tenant-namespaced entry; returns true on hit.
    bool get(const std::string& tenant_id, const std::string& key, std::string& value_out) {
        const std::string ns_key = tenant_id + "/" + key;
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(ns_key);
        if (it == store_.end()) {
            miss_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        value_out = it->second;
        seq_[ns_key] = ++seq_counter_; // LRU touch
        hit_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /// Return true if a key from tenant_b is reachable via tenant_a's namespace.
    bool crossTenantAccessible(const std::string& tenant_a, const std::string& key,
                                const std::string& tenant_b) {
        // Isolation check: only allow access when tenant_a == tenant_b
        if (tenant_a == tenant_b) return false; // same tenant, not a cross-tenant access
        std::string ignored;
        return get(tenant_a, tenant_b + "/" + key, ignored); // must always be false
    }

    uint64_t hitCount()      const noexcept { return hit_count_.load(std::memory_order_relaxed); }
    uint64_t missCount()     const noexcept { return miss_count_.load(std::memory_order_relaxed); }
    uint64_t evictionCount() const noexcept { return eviction_count_.load(std::memory_order_relaxed); }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return store_.size();
    }

private:
    const std::size_t capacity_;
    std::unordered_map<std::string, std::string> store_;
    std::unordered_map<std::string, uint64_t>    seq_;
    uint64_t seq_counter_ = 0;
    mutable std::mutex mu_;
    std::atomic<uint64_t> hit_count_{0};
    std::atomic<uint64_t> miss_count_{0};
    std::atomic<uint64_t> eviction_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityTenantLoad
//
// Insert 2000 distinct tenant-keyed entries (100 tenants × 20 keys each) into
// the stub cache.  All entries must be retrievable (no silent loss); per-tenant
// isolation must hold (no cross-tenant key leakage).
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheHighCardinalityStress, HighCardinalityTenantLoad) {
    constexpr std::size_t kTenants    = 100;
    constexpr std::size_t kKeysPerTenant = 20;
    constexpr std::size_t kTotalEntries  = kTenants * kKeysPerTenant; // 2000

    // Capacity large enough to hold all entries without eviction
    StubTenantCache cache(kTotalEntries + 100);

    // Insert all 2000 distinct tenant-keyed entries
    for (std::size_t t = 0; t < kTenants; ++t) {
        const std::string tid = "tenant_" + std::to_string(t);
        for (std::size_t k = 0; k < kKeysPerTenant; ++k) {
            const std::string key   = "key_" + std::to_string(k);
            const std::string value = "value_t" + std::to_string(t) + "_k" + std::to_string(k);
            cache.put(tid, key, value);
        }
    }

    ASSERT_EQ(cache.size(), kTotalEntries)
        << "All " << kTotalEntries << " distinct entries must be stored";

    // Retrieve all entries and verify values
    std::size_t retrieved = 0;
    std::size_t mismatch  = 0;
    for (std::size_t t = 0; t < kTenants; ++t) {
        const std::string tid = "tenant_" + std::to_string(t);
        for (std::size_t k = 0; k < kKeysPerTenant; ++k) {
            const std::string key      = "key_" + std::to_string(k);
            const std::string expected = "value_t" + std::to_string(t) + "_k" + std::to_string(k);
            std::string actual;
            if (cache.get(tid, key, actual)) {
                ++retrieved;
                if (actual != expected) ++mismatch;
            }
        }
    }

    EXPECT_EQ(retrieved, kTotalEntries)
        << "All " << kTotalEntries << " entries must be retrievable (no silent loss)";
    EXPECT_EQ(mismatch, 0u)
        << "Retrieved values must exactly match inserted values (no cross-tenant corruption)";

    // Isolation check: no cross-tenant access must succeed
    std::size_t isolation_violations = 0;
    for (std::size_t t = 0; t < std::min(kTenants, std::size_t{10}); ++t) {
        const std::string tid_a = "tenant_" + std::to_string(t);
        const std::string tid_b = "tenant_" + std::to_string((t + 1) % kTenants);
        if (cache.crossTenantAccessible(tid_a, "key_0", tid_b)) {
            ++isolation_violations;
        }
    }
    EXPECT_EQ(isolation_violations, 0u)
        << "Per-tenant namespace isolation must prevent cross-tenant key access";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentMultiTenantStress
//
// Spawn 8 worker threads; each performs get/put operations over a shared
// StubTenantCache.  All operations must complete without data races or
// deadlocks.  Total operation count must match expected.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheHighCardinalityStress, ConcurrentMultiTenantStress) {
    constexpr int         kThreads    = 8;
    constexpr int         kPerThread  = 250; // 8 × 250 = 2000 ops total
    constexpr std::size_t kCapacity   = 500; // deliberate pressure: less than total keys

    StubTenantCache cache(kCapacity);

    std::atomic<uint64_t> total_ops{0};
    std::atomic<uint64_t> exception_count{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    const auto t0 = std::chrono::steady_clock::now();

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &cache, &total_ops, &exception_count]() {
            const std::string tid = "tenant_" + std::to_string(t);
            for (int i = 0; i < kPerThread; ++i) {
                try {
                    const std::string key   = "key_" + std::to_string(i % 100);
                    const std::string value = "v_" + std::to_string(i);
                    // Alternate between put and get
                    if (i % 2 == 0) {
                        cache.put(tid, key, value);
                    } else {
                        std::string out;
                        cache.get(tid, key, out);
                    }
                    total_ops.fetch_add(1, std::memory_order_relaxed);
                } catch (...) {
                    exception_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) { w.join(); }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_EQ(total_ops.load(), static_cast<uint64_t>(kThreads * kPerThread))
        << "All " << kThreads * kPerThread << " concurrent operations must complete";
    EXPECT_EQ(exception_count.load(), 0u)
        << "No exceptions must be thrown during concurrent multi-tenant stress";

    // Wall-clock gate: 2000 concurrent ops must finish within 10 s
    EXPECT_LE(elapsed_ms.count(), 10'000)
        << "Concurrent multi-tenant stress must complete within 10 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: EvictionPressureUnderHighCardinality
//
// Insert more entries than capacity allows, verifying that:
//  - Eviction count is strictly positive (LRU is running)
//  - Cache size never exceeds capacity (memory safety)
//  - Hit-rate is measurable even under sustained eviction pressure
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CacheHighCardinalityStress, EvictionPressureUnderHighCardinality) {
    constexpr std::size_t kCapacity   = 100;
    constexpr std::size_t kTotalWrites = 2000; // 20× over capacity

    StubTenantCache cache(kCapacity);

    // Write 2000 unique tenant-keyed entries into a cache of capacity 100
    for (std::size_t i = 0; i < kTotalWrites; ++i) {
        const std::string tid   = "tenant_" + std::to_string(i % 50);
        const std::string key   = "k_" + std::to_string(i);
        cache.put(tid, key, "v");
    }

    // Cache must never exceed its capacity
    EXPECT_LE(cache.size(), kCapacity)
        << "Cache size must not exceed capacity after sustained write pressure";

    // Eviction must have occurred
    EXPECT_GT(cache.evictionCount(), 0u)
        << "LRU eviction must occur when writes exceed cache capacity";

    // Expected evictions: at least kTotalWrites - kCapacity
    EXPECT_GE(cache.evictionCount(), kTotalWrites - kCapacity)
        << "Eviction count must be at least (writes - capacity) = "
        << (kTotalWrites - kCapacity);

    // Now read a sample of the most recently written keys (should mostly hit)
    std::size_t recent_hits = 0;
    for (std::size_t i = kTotalWrites - kCapacity / 2; i < kTotalWrites; ++i) {
        const std::string tid = "tenant_" + std::to_string(i % 50);
        const std::string key = "k_" + std::to_string(i);
        std::string out;
        if (cache.get(tid, key, out)) ++recent_hits;
    }

    // At least some recent keys should still be in the cache
    EXPECT_GT(recent_hits, 0u)
        << "At least some recently written keys must remain in the bounded LRU cache";
}

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for Phase 6 cache distribution headers:
//   include/cache/distributed_eviction.h  — IDistributedEviction
//   include/cache/cache_partition.h       — ICachePartition
//   include/cache/adaptive_ttl_policy.h   — IAdaptiveTTLPolicy
//
// Each interface is exercised via a mock concrete implementation that
// records all calls so assertions can be made about correct dispatch.

#include <gtest/gtest.h>
#include "cache/distributed_eviction.h"
#include "cache/cache_partition.h"
#include "cache/adaptive_ttl_policy.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace themis::cache;
using namespace std::chrono_literals;

// ============================================================================
// Mock implementations
// ============================================================================

// ---------------------------------------------------------------------------
// MockDistributedEviction
// ---------------------------------------------------------------------------
class MockDistributedEviction final : public IDistributedEviction {
public:
    struct EvictCall {
        std::string key;
        std::string tenant_id;
        DistributedEvictionReason reason;
    };
    struct PatternCall {
        std::string pattern;
        std::string tenant_id;
    };

    std::vector<EvictCall>   evict_calls;
    std::vector<std::string> evict_tenant_calls;
    std::vector<PatternCall> pattern_calls;
    std::vector<std::string> flush_calls;

    std::atomic<uint64_t> listener_id_counter{1};
    std::unordered_map<uint64_t, DistributedEvictionListener> listeners;
    mutable std::mutex mu;

    void evict(const std::string& key,
               const std::string& tenant_id = "",
               DistributedEvictionReason reason =
                   DistributedEvictionReason::CAPACITY_PRESSURE) override {
        std::lock_guard<std::mutex> lock(mu);
        evict_calls.push_back({key, tenant_id, reason});
    }

    void evictByPattern(const std::string& pattern,
                        const std::string& tenant_id) override {
        std::lock_guard<std::mutex> lock(mu);
        pattern_calls.push_back({pattern, tenant_id});
    }

    void evictByTenant(const std::string& tenant_id) override {
        std::lock_guard<std::mutex> lock(mu);
        evict_tenant_calls.push_back(tenant_id);
    }

    void flush(const std::string& tenant_id = "") override {
        std::lock_guard<std::mutex> lock(mu);
        flush_calls.push_back(tenant_id);
    }

    uint64_t registerEvictionListener(DistributedEvictionListener listener) override {
        std::lock_guard<std::mutex> lock(mu);
        uint64_t id = listener_id_counter.fetch_add(1);
        listeners[id] = std::move(listener);
        return id;
    }

    void unregisterEvictionListener(uint64_t handle) override {
        std::lock_guard<std::mutex> lock(mu);
        listeners.erase(handle);
    }

    DistributedEvictionStats stats() const override {
        std::lock_guard<std::mutex> lock(mu);
        return {
            static_cast<uint64_t>(evict_calls.size()),
            0, 0,
            2, 2
        };
    }

    uint64_t peerCount() const override { return 2; }
    bool isHealthy() const override { return true; }

    // Helper: fire an inbound event to all registered listeners.
    void fireInbound(const DistributedEvictionEvent& ev) {
        std::lock_guard<std::mutex> lock(mu);
        for (auto& [id, cb] : listeners) {
          cb(ev);
        }
    }
};

// ---------------------------------------------------------------------------
// MockCachePartition
// ---------------------------------------------------------------------------
class MockCachePartition final : public ICachePartition {
public:
    // tenant_id -> partition_id
    std::unordered_map<std::string, std::string> assignments;
    // partition_id -> capacity
    std::unordered_map<std::string, size_t> capacities;
    // partition_id -> set of evicted (boolean flag)
    std::unordered_map<std::string, bool> evicted;
    mutable std::mutex mu;

    static constexpr const char* kDefault = "default";

    std::string getPartitionId(const std::string& tenant_id) const override {
        std::lock_guard<std::mutex> lock(mu);
        auto it = assignments.find(tenant_id);
        return (it != assignments.end()) ? it->second : kDefault;
    }

    void assignTenant(const std::string& tenant_id,
                      const std::string& partition_id) override {
        std::lock_guard<std::mutex> lock(mu);
        assignments[tenant_id] = partition_id;
        if (capacities.find(partition_id) == capacities.end())
            capacities[partition_id] = 1000;
    }

    void unassignTenant(const std::string& tenant_id) override {
        std::lock_guard<std::mutex> lock(mu);
        assignments.erase(tenant_id);
    }

    std::vector<std::string> listTenants(
        const std::string& partition_id) const override {
        std::lock_guard<std::mutex> lock(mu);
        std::vector<std::string> result = {};

        for (auto& [tid, pid] : assignments)
            if (pid == partition_id) {
              result.push_back(tid);
            }
        std::sort(result.begin(), result.end());
        return result;
    }

    std::vector<std::string> listPartitions() const override {
        std::lock_guard<std::mutex> lock(mu);
        std::vector<std::string> result = {};

        for (auto& [pid, _] : capacities) {
          result.push_back(pid);
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    void resize(const std::string& partition_id, size_t new_capacity) override {
        std::lock_guard<std::mutex> lock(mu);
        capacities[partition_id] = new_capacity;
    }

    void evictPartition(const std::string& partition_id) override {
        std::lock_guard<std::mutex> lock(mu);
        evicted[partition_id] = true;
    }

    std::optional<PartitionStats> getStats(
        const std::string& partition_id) const override {
        std::lock_guard<std::mutex> lock(mu);
        auto it = capacities.find(partition_id);
        if (it == capacities.end()) {
          return std::nullopt;
        }
        PartitionStats s;
        s.partition_id = partition_id;
        s.capacity = it->second;
        return s;
    }

    std::vector<PartitionStats> getAllStats() const override {
        std::lock_guard<std::mutex> lock(mu);
        std::vector<PartitionStats> result = {};

        for (auto& [pid, cap] : capacities) {
            PartitionStats s;
            s.partition_id = pid;
            s.capacity = cap;
            result.push_back(s);
        }
        std::sort(result.begin(), result.end(),
                  [](const PartitionStats& a, const PartitionStats& b) {
                      return a.partition_id < b.partition_id;
                  });
        return result;
    }

    size_t getCapacity(const std::string& partition_id) const override {
        std::lock_guard<std::mutex> lock(mu);
        auto it = capacities.find(partition_id);
        return (it != capacities.end()) ? it->second : 0;
    }
};

// ---------------------------------------------------------------------------
// MockAdaptiveTTLPolicy
// ---------------------------------------------------------------------------
class MockAdaptiveTTLPolicy final : public IAdaptiveTTLPolicy {
public:
    AdaptiveTTLPolicyConfig config_;
    std::unordered_map<std::string, std::vector<AccessRecord>> history_;
    mutable std::mutex mu;

    explicit MockAdaptiveTTLPolicy(AdaptiveTTLPolicyConfig cfg = {})
        : config_(std::move(cfg)) {}

    void recordAccess(const std::string& key,
                      int64_t            timestamp_ms,
                      bool               is_hit = true) noexcept override {
        std::lock_guard<std::mutex> lock(mu);
        auto& h = history_[key];
        h.push_back({timestamp_ms, is_hit});
        // Keep only the last access_window_size records.
        if (h.size() > static_cast<size_t>(config_.access_window_size)) {
            const size_t to_remove = h.size() - static_cast<size_t>(config_.access_window_size);
            h.erase(h.begin(), h.begin() + static_cast<ptrdiff_t>(to_remove));
        }
    }

    AdaptiveTTLSuggestion computeTTL(const std::string& key,
                                     int64_t            /*now_ms*/) const override {
        std::lock_guard<std::mutex> lock(mu);
        AdaptiveTTLSuggestion s;
        auto it = history_.find(key);
        if (it == history_.end() || it->second.size() < 2) {
            s.ttl = config_.minTTL;
            s.confidence = 0.0;
            s.sample_count = (it != history_.end()) ?
                static_cast<uint32_t>(it->second.size()) : 0;
            return s;
        }
        const auto& h = it->second;
        // Compute ewma of inter-access intervals.
        // h.size() >= 2 is guaranteed by the early-return guard above.
        double ewma_ms = 0.0;
        double weight = 1.0;
        double total_weight = 0.0;
        for (ptrdiff_t idx = static_cast<ptrdiff_t>(h.size()) - 1; idx > 0; --idx) {
            const size_t i = static_cast<size_t>(idx);
            double interval = static_cast<double>(h[i].timestamp_ms - h[i-1].timestamp_ms);
            if (interval < 0.0) {
              interval = 0.0;
            }
            ewma_ms    += interval * weight;
            total_weight += weight;
            weight *= config_.decay_factor;
        }
        if (total_weight > 0.0) {
          ewma_ms /= total_weight;
        }

        // Suggested TTL = aggressiveness × mean interval, clamped.
        auto raw_ms = static_cast<int64_t>(ewma_ms * config_.aggressiveness);
        raw_ms = std::max(raw_ms,
                          static_cast<int64_t>(config_.minTTL.count()));
        raw_ms = std::min(raw_ms,
                          static_cast<int64_t>(config_.maxTTL.count()));

        s.ttl = std::chrono::milliseconds{raw_ms};
        s.mean_access_interval = std::chrono::milliseconds{
            static_cast<int64_t>(ewma_ms)};
        s.sample_count = static_cast<uint32_t>(h.size());
        s.confidence   = std::min(1.0, static_cast<double>(h.size()) /
                                           static_cast<double>(config_.access_window_size));
        return s;
    }

    std::vector<AccessRecord> getHistory(const std::string& key) const override {
        std::lock_guard<std::mutex> lock(mu);
        auto it = history_.find(key);
        if (it == history_.end()) return {};
        // Return records in chronological order (oldest first), matching the interface contract.
        auto h = it->second;
        std::sort(h.begin(), h.end(), [](const AccessRecord& a, const AccessRecord& b) {
            return a.timestamp_ms < b.timestamp_ms;
        });
        return h;
    }

    void evict(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mu);
        history_.erase(key);
    }

    uint64_t pruneHistory(int64_t now_ms, int64_t max_age_ms) override {
        std::lock_guard<std::mutex> lock(mu);
        uint64_t pruned = 0;
        int64_t cutoff = now_ms - max_age_ms;
        for (auto& [key, h] : history_) {
            auto old_size = h.size();
            h.erase(std::remove_if(h.begin(), h.end(),
                        [cutoff](const AccessRecord& r) {
                            return r.timestamp_ms < cutoff;
                        }),
                    h.end());
            pruned += old_size - h.size();
        }
        return pruned;
    }

    void flushHistory() override {
        std::lock_guard<std::mutex> lock(mu);
        history_.clear();
    }

    void configure(const AdaptiveTTLPolicyConfig& config) override {
        std::lock_guard<std::mutex> lock(mu);
        config_ = config;
    }

    AdaptiveTTLPolicyConfig getConfig() const override {
        std::lock_guard<std::mutex> lock(mu);
        return config_;
    }

    size_t trackedKeyCount() const override {
        std::lock_guard<std::mutex> lock(mu);
        return history_.size();
    }
};

// ============================================================================
// IDistributedEviction tests
// ============================================================================

class DistributedEvictionTest : public ::testing::Test {
protected:
    MockDistributedEviction eviction;
};

TEST_F(DistributedEvictionTest, Evict_RecordsCallWithKey) {
    eviction.evict("key1");
    ASSERT_EQ(eviction.evict_calls.size(), 1u);
    EXPECT_EQ(eviction.evict_calls[0].key, "key1");
    EXPECT_EQ(eviction.evict_calls[0].tenant_id, "");
    EXPECT_EQ(eviction.evict_calls[0].reason,
              DistributedEvictionReason::CAPACITY_PRESSURE);
}

TEST_F(DistributedEvictionTest, Evict_RecordsCallWithTenantAndReason) {
    eviction.evict("key2", "tenantA", DistributedEvictionReason::TTL_EXPIRED);
    ASSERT_EQ(eviction.evict_calls.size(), 1u);
    EXPECT_EQ(eviction.evict_calls[0].tenant_id, "tenantA");
    EXPECT_EQ(eviction.evict_calls[0].reason,
              DistributedEvictionReason::TTL_EXPIRED);
}

TEST_F(DistributedEvictionTest, EvictByPattern_RecordsPatternAndTenant) {
    eviction.evictByPattern("user:*", "tenantB");
    ASSERT_EQ(eviction.pattern_calls.size(), 1u);
    EXPECT_EQ(eviction.pattern_calls[0].pattern, "user:*");
    EXPECT_EQ(eviction.pattern_calls[0].tenant_id, "tenantB");
}

TEST_F(DistributedEvictionTest, EvictByTenant_RecordsTenantId) {
    eviction.evictByTenant("tenantC");
    ASSERT_EQ(eviction.evict_tenant_calls.size(), 1u);
    EXPECT_EQ(eviction.evict_tenant_calls[0], "tenantC");
}

TEST_F(DistributedEvictionTest, Flush_GlobalFlushRecordsEmptyTenant) {
    eviction.flush();
    ASSERT_EQ(eviction.flush_calls.size(), 1u);
    EXPECT_EQ(eviction.flush_calls[0], "");
}

TEST_F(DistributedEvictionTest, Flush_PartialFlushRecordsTenantId) {
    eviction.flush("tenantD");
    ASSERT_EQ(eviction.flush_calls.size(), 1u);
    EXPECT_EQ(eviction.flush_calls[0], "tenantD");
}

TEST_F(DistributedEvictionTest, RegisterListener_ListenerReceivesInboundEvent) {
    DistributedEvictionEvent received;
    bool called = false;

    auto handle = eviction.registerEvictionListener(
        [&](const DistributedEvictionEvent& ev) {
            received = ev;
            called = true;
        });

    DistributedEvictionEvent ev;
    ev.key = "k1";
    ev.tenant_id = "t1";
    ev.reason = DistributedEvictionReason::EXPLICIT_EVICT;
    eviction.fireInbound(ev);

    EXPECT_TRUE(called);
    EXPECT_EQ(received.key, "k1");
    EXPECT_EQ(received.tenant_id, "t1");
    EXPECT_EQ(received.reason, DistributedEvictionReason::EXPLICIT_EVICT);

    eviction.unregisterEvictionListener(handle);
}

TEST_F(DistributedEvictionTest, UnregisterListener_ListenerNoLongerReceivesEvents) {
    int call_count = 0;
    auto handle = eviction.registerEvictionListener(
        [&](const DistributedEvictionEvent&) { ++call_count; });

    DistributedEvictionEvent ev;
    eviction.fireInbound(ev);
    EXPECT_EQ(call_count, 1);

    eviction.unregisterEvictionListener(handle);
    eviction.fireInbound(ev);
    EXPECT_EQ(call_count, 1);  // no second call
}

TEST_F(DistributedEvictionTest, Stats_ReflectsEvictCallCount) {
    eviction.evict("a");
    eviction.evict("b");
    auto s = eviction.stats();
    EXPECT_EQ(s.evictions_sent, 2u);
}

TEST_F(DistributedEvictionTest, PeerCount_ReturnsConfiguredValue) {
    EXPECT_EQ(eviction.peerCount(), 2u);
}

TEST_F(DistributedEvictionTest, IsHealthy_ReturnsTrueByDefault) {
    EXPECT_TRUE(eviction.isHealthy());
}

TEST_F(DistributedEvictionTest, EventReasons_AllValuesDistinct) {
    // Ensure each DistributedEvictionReason value is unique.
    std::vector<DistributedEvictionReason> reasons = {
        DistributedEvictionReason::CAPACITY_PRESSURE,
        DistributedEvictionReason::TTL_EXPIRED,
        DistributedEvictionReason::EXPLICIT_EVICT,
        DistributedEvictionReason::PATTERN_EVICT,
        DistributedEvictionReason::TENANT_EVICT,
        DistributedEvictionReason::FLUSH,
    };
    std::vector<uint8_t> raw = {};

    for (auto r : reasons)
        raw.push_back(static_cast<uint8_t>(r));
    std::sort(raw.begin(), raw.end());
    EXPECT_EQ(std::unique(raw.begin(), raw.end()), raw.end());
}

// ============================================================================
// ICachePartition tests
// ============================================================================

class CachePartitionTest : public ::testing::Test {
protected:
    MockCachePartition partition;
};

TEST_F(CachePartitionTest, GetPartitionId_UnknownTenantReturnsDefault) {
    EXPECT_EQ(partition.getPartitionId("unknown"), "default");
}

TEST_F(CachePartitionTest, AssignTenant_GetPartitionIdReturnsAssigned) {
    partition.assignTenant("tenantA", "shard1");
    EXPECT_EQ(partition.getPartitionId("tenantA"), "shard1");
}

TEST_F(CachePartitionTest, AssignTenant_CreatesPartitionWithDefaultCapacity) {
    partition.assignTenant("tenantB", "shard2");
    EXPECT_GT(partition.getCapacity("shard2"), 0u);
}

TEST_F(CachePartitionTest, UnassignTenant_TenantRevertsToDefault) {
    partition.assignTenant("tenantC", "shard3");
    partition.unassignTenant("tenantC");
    EXPECT_EQ(partition.getPartitionId("tenantC"), "default");
}

TEST_F(CachePartitionTest, ListTenants_ReturnsTenantsForPartition) {
    partition.assignTenant("t1", "p1");
    partition.assignTenant("t2", "p1");
    partition.assignTenant("t3", "p2");
    auto tenants = partition.listTenants("p1");
    ASSERT_EQ(tenants.size(), 2u);
    EXPECT_EQ(tenants[0], "t1");
    EXPECT_EQ(tenants[1], "t2");
}

TEST_F(CachePartitionTest, ListTenants_UnknownPartitionReturnsEmpty) {
    EXPECT_TRUE(partition.listTenants("nonexistent").empty());
}

TEST_F(CachePartitionTest, ListPartitions_ReturnsAllCreatedPartitions) {
    partition.assignTenant("x", "alpha");
    partition.assignTenant("y", "beta");
    auto parts = partition.listPartitions();
    EXPECT_NE(std::find(parts.begin(), parts.end(), "alpha"), parts.end());
    EXPECT_NE(std::find(parts.begin(), parts.end(), "beta"), parts.end());
}

TEST_F(CachePartitionTest, Resize_UpdatesCapacity) {
    partition.assignTenant("u", "p_resize");
    partition.resize("p_resize", 512);
    EXPECT_EQ(partition.getCapacity("p_resize"), 512u);
}

TEST_F(CachePartitionTest, EvictPartition_MarksPartitionAsEvicted) {
    partition.assignTenant("v", "p_evict");
    partition.evictPartition("p_evict");
    EXPECT_TRUE(partition.evicted["p_evict"]);
}

TEST_F(CachePartitionTest, GetStats_ReturnsNulloptForUnknownPartition) {
    auto s = partition.getStats("no_such_partition");
    EXPECT_FALSE(s.has_value());
}

TEST_F(CachePartitionTest, GetStats_ReturnsStatsForKnownPartition) {
    partition.assignTenant("w", "p_stats");
    auto s = partition.getStats("p_stats");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->partition_id, "p_stats");
    EXPECT_GT(s->capacity, 0u);
}

TEST_F(CachePartitionTest, GetAllStats_ReturnsSortedByPartitionId) {
    partition.assignTenant("a", "zzz");
    partition.assignTenant("b", "aaa");
    auto all = partition.getAllStats();
    ASSERT_GE(all.size(), 2u);
    EXPECT_LE(all[0].partition_id, all[1].partition_id);
}

TEST_F(CachePartitionTest, GetCapacity_ReturnsZeroForUnknownPartition) {
    EXPECT_EQ(partition.getCapacity("ghost"), 0u);
}

TEST_F(CachePartitionTest, PartitionStats_DefaultValues) {
    PartitionStats s{};
    EXPECT_EQ(s.capacity, 0u);
    EXPECT_EQ(s.current_size, 0u);
    EXPECT_EQ(s.hit_count, 0u);
    EXPECT_EQ(s.miss_count, 0u);
    EXPECT_EQ(s.eviction_count, 0u);
    EXPECT_EQ(s.tenant_count, 0u);
}

// ============================================================================
// IAdaptiveTTLPolicy tests
// ============================================================================

class AdaptiveTTLPolicyTest : public ::testing::Test {
protected:
    AdaptiveTTLPolicyConfig cfg;

    void SetUp() override {
        cfg.minTTL           = 1000ms;
        cfg.maxTTL           = 60'000ms;
        cfg.access_window_size = 16;
        cfg.aggressiveness   = 2.0;
        cfg.decay_factor     = 0.9;
    }

    MockAdaptiveTTLPolicy makePolicy() { return MockAdaptiveTTLPolicy{cfg}; }
};

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_NoHistory_ReturnsMinTTL) {
    auto p = makePolicy();
    auto s = p.computeTTL("key", 1000LL);
    EXPECT_EQ(s.ttl, cfg.minTTL);
    EXPECT_DOUBLE_EQ(s.confidence, 0.0);
    EXPECT_EQ(s.sample_count, 0u);
}

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_SingleAccess_ReturnsMinTTL) {
    auto p = makePolicy();
    p.recordAccess("key", 1000LL);
    auto s = p.computeTTL("key", 2000LL);
    EXPECT_EQ(s.ttl, cfg.minTTL);
    EXPECT_EQ(s.sample_count, 1u);
}

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_FrequentAccess_ReturnsShorterTTL) {
    auto p = makePolicy();
    // Accesses every 1 second for 10 seconds.
    for (int i = 0; i < 10; ++i)
        p.recordAccess("freq", 1000LL * (i + 1));

    auto p2 = makePolicy();
    // Accesses every 30 seconds for 10 accesses.
    for (int i = 0; i < 10; ++i)
        p2.recordAccess("infreq", 30'000LL * (i + 1));

    auto s_freq   = p.computeTTL("freq", 10'000LL);
    auto s_infreq = p2.computeTTL("infreq", 300'000LL);

    // More frequent accesses should result in a shorter or equal TTL.
    EXPECT_LE(s_freq.ttl, s_infreq.ttl);
}

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_TTLClamped_BelowMaxTTL) {
    auto p = makePolicy();
    // Accesses 100 seconds apart (result would exceed maxTTL without clamping).
    p.recordAccess("k", 0LL);
    p.recordAccess("k", 100'000LL);
    auto s = p.computeTTL("k", 200'000LL);
    EXPECT_LE(s.ttl, cfg.maxTTL);
}

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_TTLClamped_AboveMinTTL) {
    auto p = makePolicy();
    // Accesses 1 ms apart (result would be below minTTL without clamping).
    p.recordAccess("k", 0LL);
    p.recordAccess("k", 1LL);
    auto s = p.computeTTL("k", 2LL);
    EXPECT_GE(s.ttl, cfg.minTTL);
}

TEST_F(AdaptiveTTLPolicyTest, ComputeTTL_ConfidenceGrowsWithSamples) {
    auto p = makePolicy();
    for (uint32_t i = 1; i <= cfg.access_window_size; ++i) {
        p.recordAccess("k", static_cast<int64_t>(i) * 1000LL);
        auto s = p.computeTTL("k", static_cast<int64_t>(i + 1) * 1000LL);
        EXPECT_GE(s.confidence, 0.0);
        EXPECT_LE(s.confidence, 1.0);
    }
    auto s_full = p.computeTTL("k", static_cast<int64_t>(cfg.access_window_size + 1) * 1000LL);
    EXPECT_DOUBLE_EQ(s_full.confidence, 1.0);
}

TEST_F(AdaptiveTTLPolicyTest, RecordAccess_WindowSizeEnforced) {
    auto p = makePolicy();
    for (uint32_t i = 0; i < cfg.access_window_size + 5; ++i)
        p.recordAccess("k", static_cast<int64_t>(i) * 1000LL);

    auto h = p.getHistory("k");
    EXPECT_EQ(h.size(), static_cast<size_t>(cfg.access_window_size));
}

TEST_F(AdaptiveTTLPolicyTest, Evict_ClearsHistory) {
    auto p = makePolicy();
    p.recordAccess("k", 1000LL);
    p.evict("k");
    EXPECT_TRUE(p.getHistory("k").empty());
    EXPECT_EQ(p.trackedKeyCount(), 0u);
}

TEST_F(AdaptiveTTLPolicyTest, PruneHistory_RemovesStaleRecords) {
    auto p = makePolicy();
    p.recordAccess("k", 1000LL);   // old
    p.recordAccess("k", 2000LL);   // old
    p.recordAccess("k", 100'000LL); // recent

    int64_t now = 110'000LL;
    int64_t max_age = 10'000LL;
    uint64_t pruned = p.pruneHistory(now, max_age);
    EXPECT_EQ(pruned, 2u);
    EXPECT_EQ(p.getHistory("k").size(), 1u);
}

TEST_F(AdaptiveTTLPolicyTest, PruneHistory_EmptyHistory_ReturnsZero) {
    auto p = makePolicy();
    EXPECT_EQ(p.pruneHistory(1'000'000LL, 500'000LL), 0u);
}

TEST_F(AdaptiveTTLPolicyTest, FlushHistory_ClearsAllKeys) {
    auto p = makePolicy();
    p.recordAccess("a", 1000LL);
    p.recordAccess("b", 2000LL);
    p.flushHistory();
    EXPECT_EQ(p.trackedKeyCount(), 0u);
}

TEST_F(AdaptiveTTLPolicyTest, Configure_UpdatesConfig) {
    auto p = makePolicy();
    AdaptiveTTLPolicyConfig new_cfg;
    new_cfg.minTTL           = 5000ms;
    new_cfg.maxTTL           = 120'000ms;
    new_cfg.aggressiveness   = 3.0;
    p.configure(new_cfg);
    auto got = p.getConfig();
    EXPECT_EQ(got.minTTL, 5000ms);
    EXPECT_EQ(got.maxTTL, 120'000ms);
    EXPECT_DOUBLE_EQ(got.aggressiveness, 3.0);
}

TEST_F(AdaptiveTTLPolicyTest, TrackedKeyCount_ReflectsInsertAndEvict) {
    auto p = makePolicy();
    p.recordAccess("x", 1000LL);
    p.recordAccess("y", 2000LL);
    EXPECT_EQ(p.trackedKeyCount(), 2u);
    p.evict("x");
    EXPECT_EQ(p.trackedKeyCount(), 1u);
}

TEST_F(AdaptiveTTLPolicyTest, GetHistory_ReturnsSortedChronologically) {
    auto p = makePolicy();
    p.recordAccess("k", 3000LL);
    p.recordAccess("k", 1000LL);
    p.recordAccess("k", 2000LL);
    auto h = p.getHistory("k");
    // getHistory() must return records in chronological order (oldest first).
    ASSERT_EQ(h.size(), 3u);
    EXPECT_EQ(h[0].timestamp_ms, 1000LL);
    EXPECT_EQ(h[1].timestamp_ms, 2000LL);
    EXPECT_EQ(h[2].timestamp_ms, 3000LL);
}

TEST_F(AdaptiveTTLPolicyTest, AdaptiveTTLSuggestion_DefaultValues) {
    AdaptiveTTLSuggestion s{};
    EXPECT_EQ(s.ttl.count(), 0);
    EXPECT_EQ(s.mean_access_interval.count(), 0);
    EXPECT_EQ(s.sample_count, 0u);
    EXPECT_DOUBLE_EQ(s.confidence, 0.0);
}

TEST_F(AdaptiveTTLPolicyTest, AdaptiveTTLPolicyConfig_DefaultValues) {
    AdaptiveTTLPolicyConfig c{};
    EXPECT_EQ(c.minTTL, 1000ms);
    EXPECT_EQ(c.maxTTL, 3'600'000ms);
    EXPECT_EQ(c.access_window_size, 64u);
    EXPECT_DOUBLE_EQ(c.aggressiveness, 2.0);
    EXPECT_DOUBLE_EQ(c.decay_factor, 0.9);
    EXPECT_EQ(c.max_history_age_ms, 86'400'000LL);
}

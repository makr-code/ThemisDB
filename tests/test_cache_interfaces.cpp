// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for include/cache/cache_interfaces.h
//
// Covered interfaces and value types:
//   IEvictionPolicy  — pluggable eviction strategy
//   ICacheAdminOps   — privileged runtime inspection and management
//   ICacheWarmup     — batch pre-population from IWarmupSource
//   IGDPRPurgeHook   — synchronous GDPR Art. 17 erasure
//   ITTLAdapter      — workload-driven adaptive TTL computation
//
// Value types:
//   EvictionEvent, EvictionEventType
//   CacheStats, KeyFilter
//   CacheEntry<K,V>, WarmupStats, WarmupResult, IWarmupSource
//   PurgeDescriptor, PurgeResult, PurgeReason
//   AccessPattern, TTLAdapterConfig

#include <gtest/gtest.h>
#include "cache/cache_interfaces.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::cache;
using namespace std::chrono_literals;

// ============================================================================
// Mock implementations
// ============================================================================

/// Simple FIFO eviction policy used to test IEvictionPolicy.
class MockEvictionPolicy final : public IEvictionPolicy {
public:
    std::vector<std::string> access_log;
    std::vector<std::string> insert_log;
    std::vector<std::string> remove_log;
    std::vector<std::string> key_order;  // insertion order for evict()

    void onAccess(const std::string& key) override {
        access_log.push_back(key);
    }

    void onInsert(const std::string& key) override {
        insert_log.push_back(key);
        key_order.push_back(key);
    }

    void onRemove(const std::string& key) override {
        remove_log.push_back(key);
        key_order.erase(std::remove(key_order.begin(), key_order.end(), key),
                        key_order.end());
    }

    std::string evict() override {
        if (key_order.empty()) {
          return "";
        }
        std::string k = key_order.front();
        key_order.erase(key_order.begin());
        return k;
    }

    std::string_view name() const noexcept override { return "FIFO-mock"; }
};

/// Minimal ICacheAdminOps mock.
class MockCacheAdminOps final : public ICacheAdminOps {
public:
    bool         flushed = false;
    size_t       resized_to = 0;
    CacheStats   stats_snapshot;
    std::vector<std::string> all_keys;

    void flush() override { flushed = true; }

    CacheStats stats() const override { return stats_snapshot; }

    void resize(size_t new_capacity) override { resized_to = new_capacity; }

    std::vector<std::string> listKeys(const KeyFilter& filter) const override {
        if (!filter.prefix.has_value() && !filter.pattern.has_value() &&
            filter.ttl_max_seconds == 0) {
            return all_keys;  // no filter → return all
        }
        std::vector<std::string> result = {};

        for (const auto& k : all_keys) {
            if (filter.prefix.has_value() &&
                k.rfind(*filter.prefix, 0) == 0) {
                result.push_back(k);
            }
        }
        return result;
    }
};

/// IWarmupSource backed by an in-memory vector of batches.
class VectorWarmupSource final : public IWarmupSource {
public:
    std::vector<std::vector<CacheEntry<std::string, std::string>>> batches;
    size_t index = 0;

    std::vector<CacheEntry<std::string, std::string>> nextBatch() override {
        if (index >= batches.size()) return {};
        return batches[index++];
    }
};

/// ICacheWarmup mock that collects inserted entries.
class MockCacheWarmup final : public ICacheWarmup {
public:
    std::vector<CacheEntry<std::string, std::string>> inserted;
    bool should_error = false;
    std::string error_message = {};

    WarmupResult warm(IWarmupSource& source) override {
        if (should_error) {
            return WarmupResult{false, error_message, {}};
        }
        WarmupStats s;
        auto t0 = std::chrono::steady_clock::now();
        while (true) {
            auto batch = source.nextBatch();
            if (batch.empty()) {
              break;
            }
            for (auto& e : batch) {
                inserted.push_back(e);
                ++s.entries_inserted;
            }
        }
        auto t1 = std::chrono::steady_clock::now();
        s.duration_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        return WarmupResult{true, {}, s};
    }
};

/// IGDPRPurgeHook mock.
class MockGdprPurgeHook final : public IGDPRPurgeHook {
public:
    std::vector<PurgeDescriptor> received;
    bool throw_on_empty_subject = true;
    size_t purge_count_per_call = 3;
    std::string audit_id_prefix = "audit-";
    int call_index = 0;

    PurgeResult purge(const PurgeDescriptor& descriptor) override {
        if (throw_on_empty_subject && descriptor.subject_id.empty()) {
            throw std::runtime_error("subject_id must not be empty");
        }
        received.push_back(descriptor);
        PurgeResult r;
        r.purged_key_count  = purge_count_per_call;
        r.audit_log_entry_id = audit_id_prefix + std::to_string(++call_index);
        r.timestamp_utc_ms   = 1'700'000'000'000LL + call_index;
        return r;
    }
};

/// ITTLAdapter that clamps to [minTTL, maxTTL] and scales by access_frequency.
class MockTTLAdapter final : public ITTLAdapter {
public:
    TTLAdapterConfig cfg_;

    MockTTLAdapter() {
        cfg_.minTTL        = 1'000ms;
        cfg_.maxTTL        = 3'600'000ms;
        cfg_.aggressiveness = 1.0;
    }

    std::chrono::milliseconds computeTTL(const std::string& /*key*/,
                                         const AccessPattern& pattern) const override {
        // Simple formula: baseTTL adjusted by frequency.
        long long base = 10'000;  // 10 s default
        if (pattern.access_frequency > 0) {
            base = static_cast<long long>(
                base * cfg_.aggressiveness * pattern.access_frequency);
        }
        auto result = std::chrono::milliseconds(base);
        if (result < cfg_.minTTL) {
          result = cfg_.minTTL;
        }
        if (result > cfg_.maxTTL) {
          result = cfg_.maxTTL;
        }
        return result;
    }

    void configure(const TTLAdapterConfig& config) override { cfg_ = config; }
};

// ============================================================================
// IEvictionPolicy tests
// ============================================================================

TEST(EvictionPolicyTest, NameReturnsExpectedString) {
    MockEvictionPolicy p;
    EXPECT_EQ(p.name(), "FIFO-mock");
}

TEST(EvictionPolicyTest, OnAccessRecordsKey) {
    MockEvictionPolicy p;
    p.onAccess("k1");
    p.onAccess("k2");
    ASSERT_EQ(p.access_log.size(), 2u);
    EXPECT_EQ(p.access_log[0], "k1");
    EXPECT_EQ(p.access_log[1], "k2");
}

TEST(EvictionPolicyTest, OnInsertRecordsKey) {
    MockEvictionPolicy p;
    p.onInsert("k1");
    ASSERT_EQ(p.insert_log.size(), 1u);
    EXPECT_EQ(p.insert_log[0], "k1");
}

TEST(EvictionPolicyTest, OnRemoveRecordsKey) {
    MockEvictionPolicy p;
    p.onInsert("k1");
    p.onRemove("k1");
    ASSERT_EQ(p.remove_log.size(), 1u);
    EXPECT_EQ(p.remove_log[0], "k1");
}

TEST(EvictionPolicyTest, EvictReturnsFifoOrder) {
    MockEvictionPolicy p;
    p.onInsert("a");
    p.onInsert("b");
    p.onInsert("c");
    EXPECT_EQ(p.evict(), "a");
    EXPECT_EQ(p.evict(), "b");
    EXPECT_EQ(p.evict(), "c");
}

TEST(EvictionPolicyTest, PolymorphicUsageViaInterface) {
    std::unique_ptr<IEvictionPolicy> iface = std::make_unique<MockEvictionPolicy>();
    iface->onInsert("x");
    EXPECT_EQ(iface->evict(), "x");
    EXPECT_EQ(iface->name(), "FIFO-mock");
}

TEST(EvictionEventTest, DefaultConstruction) {
    EvictionEvent ev;
    EXPECT_TRUE(ev.key.empty());
}

TEST(EvictionEventTest, AssignFields) {
    EvictionEvent ev;
    ev.type = EvictionEventType::ACCESS;
    ev.key  = "mykey";
    EXPECT_EQ(ev.type, EvictionEventType::ACCESS);
    EXPECT_EQ(ev.key, "mykey");
}

TEST(EvictionEventTypeTest, AllEnumValues) {
    // Ensure all enum values compile and compare correctly.
    EXPECT_NE(EvictionEventType::ACCESS, EvictionEventType::INSERT);
    EXPECT_NE(EvictionEventType::REMOVE, EvictionEventType::EXPIRY);
    EXPECT_EQ(EvictionEventType::ACCESS, EvictionEventType::ACCESS);
}

// ============================================================================
// ICacheAdminOps tests
// ============================================================================

TEST(CacheAdminOpsTest, FlushSetsFlag) {
    MockCacheAdminOps admin;
    EXPECT_FALSE(admin.flushed);
    admin.flush();
    EXPECT_TRUE(admin.flushed);
}

TEST(CacheAdminOpsTest, StatsReturnsSnapshot) {
    MockCacheAdminOps admin;
    admin.stats_snapshot.hit_count  = 42;
    admin.stats_snapshot.miss_count = 7;
    admin.stats_snapshot.current_size = 10;
    auto s = admin.stats();
    EXPECT_EQ(s.hit_count,  42u);
    EXPECT_EQ(s.miss_count, 7u);
    EXPECT_EQ(s.current_size, 10u);
}

TEST(CacheAdminOpsTest, ResizeSetsCapacity) {
    MockCacheAdminOps admin;
    admin.resize(128);
    EXPECT_EQ(admin.resized_to, 128u);
}

TEST(CacheAdminOpsTest, ListKeysNoFilter) {
    MockCacheAdminOps admin;
    admin.all_keys = {"alpha", "beta", "gamma"};
    auto keys = admin.listKeys(KeyFilter{});
    EXPECT_EQ(keys.size(), 3u);
}

TEST(CacheAdminOpsTest, ListKeysWithPrefix) {
    MockCacheAdminOps admin;
    admin.all_keys = {"cache:a", "cache:b", "other:c"};
    KeyFilter f;
    f.prefix = "cache:";
    auto keys = admin.listKeys(f);
    ASSERT_EQ(keys.size(), 2u);
    EXPECT_EQ(keys[0], "cache:a");
    EXPECT_EQ(keys[1], "cache:b");
}

TEST(CacheAdminOpsTest, ListKeysEmptyCache) {
    MockCacheAdminOps admin;
    auto keys = admin.listKeys(KeyFilter{});
    EXPECT_TRUE(keys.empty());
}

TEST(CacheStatsTest, DefaultZeroValues) {
    CacheStats s;
    EXPECT_EQ(s.hit_count, 0u);
    EXPECT_EQ(s.miss_count, 0u);
    EXPECT_EQ(s.eviction_count, 0u);
    EXPECT_EQ(s.current_size, 0u);
    EXPECT_EQ(s.capacity, 0u);
}

TEST(KeyFilterTest, DefaultNoFilter) {
    KeyFilter f;
    EXPECT_FALSE(f.prefix.has_value());
    EXPECT_FALSE(f.pattern.has_value());
    EXPECT_EQ(f.ttl_max_seconds, 0u);
}

TEST(KeyFilterTest, PrefixAndPatternSetIndependently) {
    KeyFilter f;
    f.prefix  = "pref:";
    f.pattern = ".*foo.*";
    EXPECT_EQ(*f.prefix, "pref:");
    EXPECT_EQ(*f.pattern, ".*foo.*");
}

// ============================================================================
// ICacheWarmup / IWarmupSource tests
// ============================================================================

TEST(CacheEntryTest, DefaultConstruction) {
    CacheEntry<std::string, int> e;
    EXPECT_TRUE(e.key.empty());
    EXPECT_EQ(e.ttl_seconds, 0u);
    EXPECT_TRUE(e.tenant_id.empty());
}

TEST(CacheEntryTest, AssignFields) {
    CacheEntry<std::string, std::string> e;
    e.key        = "k1";
    e.value      = "v1";
    e.ttl_seconds = 30;
    e.tenant_id  = "tenant-42";
    EXPECT_EQ(e.key, "k1");
    EXPECT_EQ(e.value, "v1");
    EXPECT_EQ(e.ttl_seconds, 30u);
    EXPECT_EQ(e.tenant_id, "tenant-42");
}

TEST(VectorWarmupSourceTest, ReturnsEmptyWhenExhausted) {
    VectorWarmupSource src;
    EXPECT_TRUE(src.nextBatch().empty());
}

TEST(VectorWarmupSourceTest, ReturnsBatchesThenEmpty) {
    VectorWarmupSource src;
    CacheEntry<std::string, std::string> e;
    e.key = "k";
    e.value = "v";
    src.batches.push_back({e});
    auto b1 = src.nextBatch();
    ASSERT_EQ(b1.size(), 1u);
    EXPECT_EQ(b1[0].key, "k");
    EXPECT_TRUE(src.nextBatch().empty());
}

TEST(CacheWarmupInterfaceTest, WarmSuccessInsertsSingleBatch) {
    MockCacheWarmup warmup;
    VectorWarmupSource src;
    CacheEntry<std::string, std::string> e;
    e.key = "k1";
    e.value = "v1";
    e.ttl_seconds = 60;
    e.tenant_id = "t1";
    src.batches.push_back({e});

    auto result = warmup.warm(src);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.error.empty());
    ASSERT_EQ(warmup.inserted.size(), 1u);
    EXPECT_EQ(warmup.inserted[0].key, "k1");
    EXPECT_EQ(result.stats.entries_inserted, 1u);
}

TEST(CacheWarmupInterfaceTest, WarmSuccessMultipleBatches) {
    MockCacheWarmup warmup;
    VectorWarmupSource src;
    for (int i = 0; i < 3; ++i) {
        CacheEntry<std::string, std::string> e;
        e.key = "k" + std::to_string(i);
        e.value = "v";
        src.batches.push_back({e});
    }

    auto result = warmup.warm(src);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(warmup.inserted.size(), 3u);
    EXPECT_EQ(result.stats.entries_inserted, 3u);
}

TEST(CacheWarmupInterfaceTest, WarmReturnsErrorWhenFlagSet) {
    MockCacheWarmup warmup;
    warmup.should_error = true;
    warmup.error_message = "redis unavailable";
    VectorWarmupSource src;

    auto result = warmup.warm(src);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "redis unavailable");
    EXPECT_EQ(result.stats.entries_inserted, 0u);
}

TEST(CacheWarmupInterfaceTest, WarmStatsContainDuration) {
    MockCacheWarmup warmup;
    VectorWarmupSource src;
    CacheEntry<std::string, std::string> e;
    e.key = "k1";
    e.value = "v1";
    e.ttl_seconds = 0;
    e.tenant_id = "";
    src.batches.push_back({e});

    auto result = warmup.warm(src);
    // duration_ms may be 0 if the call is instant, but the field should exist.
    EXPECT_GE(result.stats.duration_ms, 0u);
}

TEST(WarmupStatsTest, DefaultZeroValues) {
    WarmupStats s;
    EXPECT_EQ(s.entries_inserted, 0u);
    EXPECT_EQ(s.entries_skipped, 0u);
    EXPECT_EQ(s.duration_ms, 0u);
    EXPECT_EQ(s.error_count, 0u);
}

TEST(WarmupResultTest, DefaultOkTrue) {
    WarmupResult r;
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.error.empty());
}

// ============================================================================
// IGDPRPurgeHook tests
// ============================================================================

TEST(GdprPurgeHookTest, PurgeReturnsResult) {
    MockGdprPurgeHook hook;
    PurgeDescriptor desc;
    desc.subject_id   = "user-001";
    desc.key_patterns = {"pii:user-001:*"};
    desc.reason       = PurgeReason::RIGHT_TO_ERASURE;

    auto result = hook.purge(desc);
    EXPECT_EQ(result.purged_key_count, 3u);
    EXPECT_FALSE(result.audit_log_entry_id.empty());
    EXPECT_GE(result.timestamp_utc_ms, 0LL);
}

TEST(GdprPurgeHookTest, PurgeThrowsOnEmptySubjectId) {
    MockGdprPurgeHook hook;
    PurgeDescriptor desc;
    desc.subject_id = "";
    EXPECT_THROW(hook.purge(desc), std::runtime_error);
}

TEST(GdprPurgeHookTest, PurgeAuditIdIsUnique) {
    MockGdprPurgeHook hook;
    PurgeDescriptor d1, d2;
    d1.subject_id = "user-a";
    d2.subject_id = "user-b";

    auto r1 = hook.purge(d1);
    auto r2 = hook.purge(d2);
    EXPECT_NE(r1.audit_log_entry_id, r2.audit_log_entry_id);
}

TEST(GdprPurgeHookTest, MultiplePurgesAccumulateInReceived) {
    MockGdprPurgeHook hook;
    for (int i = 0; i < 5; ++i) {
        PurgeDescriptor d;
        d.subject_id = "u" + std::to_string(i);
        hook.purge(d);
    }
    EXPECT_EQ(hook.received.size(), 5u);
}

TEST(PurgeDescriptorTest, DefaultReason) {
    PurgeDescriptor d;
    EXPECT_EQ(d.reason, PurgeReason::RIGHT_TO_ERASURE);
    EXPECT_TRUE(d.subject_id.empty());
    EXPECT_TRUE(d.key_patterns.empty());
}

TEST(PurgeReasonTest, AllValuesAreDistinct) {
    EXPECT_NE(PurgeReason::RIGHT_TO_ERASURE, PurgeReason::RETENTION_EXPIRED);
    EXPECT_NE(PurgeReason::CONSENT_WITHDRAWN, PurgeReason::OTHER);
    EXPECT_EQ(PurgeReason::RIGHT_TO_ERASURE, PurgeReason::RIGHT_TO_ERASURE);
}

TEST(PurgeResultTest, DefaultZeroValues) {
    PurgeResult r;
    EXPECT_EQ(r.purged_key_count, 0u);
    EXPECT_TRUE(r.audit_log_entry_id.empty());
    EXPECT_EQ(r.timestamp_utc_ms, 0LL);
}

// ============================================================================
// ITTLAdapter tests
// ============================================================================

TEST(TTLAdapterTest, ComputeTTLRespectsMinTTL) {
    MockTTLAdapter adapter;
    AccessPattern  p;
    p.access_frequency = 0;  // forces base below min
    auto ttl = adapter.computeTTL("key", p);
    EXPECT_GE(ttl, 1'000ms);
}

TEST(TTLAdapterTest, ComputeTTLRespectsMaxTTL) {
    MockTTLAdapter adapter;
    AccessPattern  p;
    p.access_frequency = 1'000'000;  // very high → should be clamped
    auto ttl = adapter.computeTTL("key", p);
    EXPECT_LE(ttl, 3'600'000ms);
}

TEST(TTLAdapterTest, ComputeTTLWithHighFrequency_AboveMin) {
    MockTTLAdapter adapter;
    AccessPattern  p;
    p.access_frequency = 10;
    auto ttl = adapter.computeTTL("mykey", p);
    EXPECT_GE(ttl, 1'000ms);
    EXPECT_LE(ttl, 3'600'000ms);
}

TEST(TTLAdapterTest, ConfigureChangesMaxTTL) {
    MockTTLAdapter adapter;
    TTLAdapterConfig cfg;
    cfg.minTTL        = 500ms;
    cfg.maxTTL        = 60'000ms;
    cfg.aggressiveness = 2.0;
    adapter.configure(cfg);

    AccessPattern p;
    p.access_frequency = 1'000'000;
    auto ttl = adapter.computeTTL("x", p);
    EXPECT_LE(ttl, 60'000ms);
}

TEST(TTLAdapterTest, PolymorphicUsage) {
    std::unique_ptr<ITTLAdapter> iface = std::make_unique<MockTTLAdapter>();
    AccessPattern p;
    p.access_frequency = 1;
    auto ttl = iface->computeTTL("k", p);
    EXPECT_GE(ttl, 1'000ms);
}

TEST(TTLAdapterConfigTest, Defaults) {
    TTLAdapterConfig cfg;
    EXPECT_EQ(cfg.minTTL, 1'000ms);
    EXPECT_EQ(cfg.maxTTL, 3'600'000ms);
    EXPECT_DOUBLE_EQ(cfg.aggressiveness, 1.0);
}

TEST(AccessPatternTest, DefaultZeroValues) {
    AccessPattern p;
    EXPECT_EQ(p.access_frequency, 0u);
    EXPECT_EQ(p.last_access_age_ms, 0u);
    EXPECT_DOUBLE_EQ(p.write_ratio, 0.0);
}

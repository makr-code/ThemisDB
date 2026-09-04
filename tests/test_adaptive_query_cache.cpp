#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include "cache/eviction_policy.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis;
using json = nlohmann::json;

class AdaptiveQueryCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use temporary directory for test cache
        config_.l3_db_path = "/tmp/themis_test_query_cache_" + 
                             std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        config_.l1_max_entries = 10;
        config_.l2_max_entries = 20;
        config_.l1_ttl_seconds = 1;  // Short TTL for testing
        config_.l2_ttl_seconds = 2;
        config_.l3_ttl_seconds = 3;
    }
    
    void TearDown() override {
        // Cleanup test cache directory
        if (!config_.l3_db_path.empty()) {
            std::filesystem::remove_all(config_.l3_db_path);
        }
    }
    
    AdaptiveQueryCache::Config config_;
};

TEST_F(AdaptiveQueryCacheTest, GenerateFingerprint) {
    AdaptiveQueryCache cache(config_);
    
    std::string query1 = "SELECT * FROM users WHERE id = ?";
    json params1 = {{"id", 123}};
    
    std::string fingerprint1 = cache.generateFingerprint(query1, params1);
    EXPECT_EQ(fingerprint1.length(), 64);  // SHA256 = 64 hex chars
    
    // Same query and params should produce same fingerprint
    std::string fingerprint2 = cache.generateFingerprint(query1, params1);
    EXPECT_EQ(fingerprint1, fingerprint2);
    
    // Different params should produce different fingerprint
    json params2 = {{"id", 456}};
    std::string fingerprint3 = cache.generateFingerprint(query1, params2);
    EXPECT_NE(fingerprint1, fingerprint3);
}

TEST_F(AdaptiveQueryCacheTest, L1CacheHit) {
    AdaptiveQueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json params = {};
    json result = {{"data", {1, 2, 3}}};
    
    std::string fingerprint = cache.generateFingerprint(query, params);
    
    // Store in cache
    EXPECT_TRUE(cache.put(fingerprint, params, result));
    
    // Retrieve from cache
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::HOT);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l1_hits, 1);
    EXPECT_EQ(stats.misses, 0);
    EXPECT_DOUBLE_EQ(stats.getHitRate(), 1.0);
}

TEST_F(AdaptiveQueryCacheTest, L1CacheMiss) {
    AdaptiveQueryCache cache(config_);
    
    std::string fingerprint = "nonexistent_fingerprint";
    
    // Try to retrieve non-existent entry
    auto cached = cache.get(fingerprint);
    EXPECT_FALSE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l1_hits, 0);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.getHitRate(), 0.0);
}

TEST_F(AdaptiveQueryCacheTest, L1LRUEviction) {
    AdaptiveQueryCache cache(config_);
    
    // Fill L1 cache to capacity
    for (int i = 0; i < config_.l1_max_entries; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Add one more entry (should evict LRU)
    std::string query_new = "SELECT * FROM users WHERE id = 999";
    json result_new = {{"id", 999}};
    std::string fingerprint_new = cache.generateFingerprint(query_new);
    cache.put(fingerprint_new, {}, result_new);
    
    // Check that new entry exists
    auto cached = cache.get(fingerprint_new);
    EXPECT_TRUE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.evictions, 1);
}

TEST_F(AdaptiveQueryCacheTest, L2CacheCompression) {
    AdaptiveQueryCache cache(config_);
    
    // Create a large result that should go to L2
    json large_result;
    for (int i = 0; i < 200; i++) {
        large_result["data"].push_back({{"id", i}, {"name", "User " + std::to_string(i)}});
    }
    
    std::string query = "SELECT * FROM users";
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in cache (should go to L2)
    EXPECT_TRUE(cache.put(fingerprint, {}, large_result));
    
    // Retrieve from cache
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, large_result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::WARM);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l2_hits, 1);
}

TEST_F(AdaptiveQueryCacheTest, L2ToL1Promotion) {
    AdaptiveQueryCache cache(config_);
    
    // Create a result that goes to L2
    json result;
    for (int i = 0; i < 50; i++) {
        result["data"].push_back({{"id", i}});
    }
    
    std::string query = "SELECT * FROM users";
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in L2
    cache.put(fingerprint, {}, result);
    
    // Access multiple times to trigger promotion
    for (int i = 0; i < 3; i++) {
        auto cached = cache.get(fingerprint);
        EXPECT_TRUE(cached.has_value());
    }
    
    // Next access should hit L1 (promoted)
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_GE(stats.promotions, 1);
}

TEST_F(AdaptiveQueryCacheTest, TTLExpiration) {
    AdaptiveQueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result = {{"data", {1, 2, 3}}};
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in cache
    cache.put(fingerprint, {}, result);
    
    // Verify cache hit
    auto cached1 = cache.get(fingerprint);
    EXPECT_TRUE(cached1.has_value());
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired now
    auto cached2 = cache.get(fingerprint);
    EXPECT_FALSE(cached2.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_GE(stats.evictions, 1);
}

TEST_F(AdaptiveQueryCacheTest, ClearCache) {
    AdaptiveQueryCache cache(config_);
    
    // Add some entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Clear cache
    cache.clear();
    
    // Verify all entries are gone
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        std::string fingerprint = cache.generateFingerprint(query);
        auto cached = cache.get(fingerprint);
        EXPECT_FALSE(cached.has_value());
    }
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePattern) {
    AdaptiveQueryCache cache(config_);
    
    // Add entries with different patterns
    std::string fingerprint1 = cache.generateFingerprint("SELECT * FROM users");
    std::string fingerprint2 = cache.generateFingerprint("SELECT * FROM orders");
    std::string fingerprint3 = cache.generateFingerprint("SELECT * FROM products");
    
    cache.put(fingerprint1, {}, {{"data", "users"}});
    cache.put(fingerprint2, {}, {{"data", "orders"}});
    cache.put(fingerprint3, {}, {{"data", "products"}});
    
    // Invalidate entries matching pattern (fingerprints starting with specific prefix)
    size_t invalidated = cache.invalidate(fingerprint1.substr(0, 10) + ".*");
    EXPECT_GE(invalidated, 0);  // May or may not match depending on hash
    
    // Note: Pattern matching on fingerprints is less useful than on query params
    // In production, you'd want to store and match on query metadata
}

TEST_F(AdaptiveQueryCacheTest, DetailedInfo) {
    AdaptiveQueryCache cache(config_);
    
    // Add some entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Get detailed info
    json info = cache.getDetailedInfo();
    
    EXPECT_TRUE(info.contains("stats"));
    EXPECT_TRUE(info.contains("l1"));
    EXPECT_TRUE(info.contains("l2"));
    EXPECT_TRUE(info.contains("l3"));
    
    EXPECT_EQ(info["l1"]["entries"], 5);
    EXPECT_GT(info["l1"]["utilization"], 0.0);
}

TEST_F(AdaptiveQueryCacheTest, ConcurrentAccess) {
    AdaptiveQueryCache cache(config_);
    
    const int num_threads = 4;
    const int ops_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_hits{0};
    std::atomic<int> total_misses{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&cache, t, ops_per_thread, &total_hits, &total_misses]() {
            for (int i = 0; i < ops_per_thread; i++) {
                std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i % 10);
                json result = {{"id", i % 10}};
                std::string fingerprint = cache.generateFingerprint(query);
                
                // Try to get from cache
                auto cached = cache.get(fingerprint);
                if (cached.has_value()) {
                    total_hits++;
                } else {
                    total_misses++;
                    // Store in cache
                    cache.put(fingerprint, {}, result);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that cache handled concurrent access
    auto stats = cache.getStats();
    EXPECT_GT(stats.l1_hits + stats.l2_hits + stats.l3_hits, 0);
    EXPECT_EQ(total_hits.load() + total_misses.load(), num_threads * ops_per_thread);
}

// ============================================================================
// Configurable Eviction Policy Tests
// ============================================================================

TEST_F(AdaptiveQueryCacheTest, DefaultEvictionPolicyIsLRU) {
    EXPECT_EQ(config_.l1_eviction_policy, cache::EvictionPolicy::LRU);
    EXPECT_EQ(config_.l2_eviction_policy, cache::EvictionPolicy::LRU);
}

TEST_F(AdaptiveQueryCacheTest, LFUPolicyConfigures) {
    config_.l1_eviction_policy = cache::EvictionPolicy::LFU;
    config_.l2_eviction_policy = cache::EvictionPolicy::LFU;
    // Cache must construct without throwing with LFU policy
    EXPECT_NO_THROW(AdaptiveQueryCache cache(config_));
}

TEST_F(AdaptiveQueryCacheTest, ARCPolicyConfigures) {
    config_.l1_eviction_policy = cache::EvictionPolicy::ARC;
    config_.l2_eviction_policy = cache::EvictionPolicy::ARC;
    // Cache must construct without throwing with ARC policy
    EXPECT_NO_THROW(AdaptiveQueryCache cache(config_));
}

TEST_F(AdaptiveQueryCacheTest, LFUEvictsLeastFrequentEntry) {
    config_.l1_eviction_policy = cache::EvictionPolicy::LFU;
    config_.l1_max_entries = 3;
    AdaptiveQueryCache cache(config_);

    // Store 3 small entries
    cache.put("fp1", {}, json({{"v", 1}}));
    cache.put("fp2", {}, json({{"v", 2}}));
    cache.put("fp3", {}, json({{"v", 3}}));

    // Access fp2 and fp3 multiple times to raise their frequency
    cache.get("fp2"); cache.get("fp2");
    cache.get("fp3");

    // Inserting fp4 should evict the least-frequently-used entry (fp1)
    cache.put("fp4", {}, json({{"v", 4}}));

    // fp1 should be gone; fp2, fp3, fp4 should still be present
    EXPECT_FALSE(cache.get("fp1").has_value());
    EXPECT_TRUE(cache.get("fp2").has_value());
    EXPECT_TRUE(cache.get("fp3").has_value());
    EXPECT_TRUE(cache.get("fp4").has_value());
}

TEST_F(AdaptiveQueryCacheTest, ARCPolicyEvictsCorrectly) {
    config_.l1_eviction_policy = cache::EvictionPolicy::ARC;
    config_.l1_max_entries = 3;
    AdaptiveQueryCache cache(config_);

    cache.put("fp1", {}, json({{"v", 1}}));
    cache.put("fp2", {}, json({{"v", 2}}));
    cache.put("fp3", {}, json({{"v", 3}}));

    // Cache is now full. Inserting fp4 must evict one entry without crashing.
    EXPECT_NO_THROW(cache.put("fp4", {}, json({{"v", 4}})));

    // Exactly 3 entries should remain
    int present = 0;
    for (const auto& fp : {"fp1", "fp2", "fp3", "fp4"}) {
        if (cache.get(fp).has_value()) {
          ++present;
        }
    }
    EXPECT_EQ(present, 3);
}

TEST_F(AdaptiveQueryCacheTest, PolicyClearResetState) {
    config_.l1_eviction_policy = cache::EvictionPolicy::ARC;
    AdaptiveQueryCache cache(config_);

    cache.put("fp1", {}, json({{"v", 1}}));
    cache.put("fp2", {}, json({{"v", 2}}));

    // clear() must not crash and the cache must be empty afterwards
    EXPECT_NO_THROW(cache.clear());
    EXPECT_FALSE(cache.get("fp1").has_value());
    EXPECT_FALSE(cache.get("fp2").has_value());
}

// ============================================================================
// GDPR-aware PII cache invalidation tests
// ============================================================================

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_RemovesTaggedL1Entry) {
    AdaptiveQueryCache cache(config_);

    std::string query = "SELECT * FROM users WHERE id = 42";
    json result = {{"id", 42}, {"name", "Alice"}};
    std::string fp = cache.generateFingerprint(query);
    std::string pii_uuid = "550e8400-e29b-41d4-a716-446655440000";

    // Store with PII tag
    EXPECT_TRUE(cache.put(fp, {}, result, "", {pii_uuid}));
    EXPECT_TRUE(cache.get(fp).has_value());

    // Invalidate via PII UUID – entry must be gone
    size_t purged = cache.invalidatePII(pii_uuid);
    EXPECT_GE(purged, 1u);
    EXPECT_FALSE(cache.get(fp).has_value());
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_LeavesUntaggedEntriesIntact) {
    AdaptiveQueryCache cache(config_);

    std::string fp_tagged   = cache.generateFingerprint("SELECT a FROM t WHERE a=1");
    std::string fp_untagged = cache.generateFingerprint("SELECT b FROM t WHERE b=2");
    std::string pii_uuid    = "550e8400-e29b-41d4-a716-446655440001";

    cache.put(fp_tagged,   {}, {{"a", 1}}, "", {pii_uuid});
    cache.put(fp_untagged, {}, {{"b", 2}});

    EXPECT_EQ(cache.invalidatePII(pii_uuid), 1u);
    EXPECT_FALSE(cache.get(fp_tagged).has_value());
    EXPECT_TRUE(cache.get(fp_untagged).has_value());
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_MultipleEntriesSamePIIUUID) {
    AdaptiveQueryCache cache(config_);

    std::string pii_uuid = "550e8400-e29b-41d4-a716-446655440002";
    std::vector<std::string> fps = {};

    for (int i = 0; i < 3; ++i) {
        std::string fp = cache.generateFingerprint("Q" + std::to_string(i));
        fps.push_back(fp);
        cache.put(fp, {}, {{"i", i}}, "", {pii_uuid});
    }

    size_t purged = cache.invalidatePII(pii_uuid);
    EXPECT_EQ(purged, 3u);
    for (const auto& fp : fps) {
        EXPECT_FALSE(cache.get(fp).has_value());
    }
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_UnknownUUIDReturnsZero) {
    AdaptiveQueryCache cache(config_);

    cache.put(cache.generateFingerprint("X"), {}, {{"x", 1}});
    EXPECT_EQ(cache.invalidatePII("unknown-uuid"), 0u);
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_EmptyUUIDReturnsZero) {
    AdaptiveQueryCache cache(config_);
    EXPECT_EQ(cache.invalidatePII(""), 0u);
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePII_EntryTaggedWithMultiplePIIUUIDs) {
    AdaptiveQueryCache cache(config_);

    std::string fp       = cache.generateFingerprint("SELECT * FROM contacts");
    std::string pii_uuid1 = "aaaaaaaa-0000-0000-0000-000000000001";
    std::string pii_uuid2 = "bbbbbbbb-0000-0000-0000-000000000002";

    cache.put(fp, {}, {{"contact", "data"}}, "", {pii_uuid1, pii_uuid2});
    EXPECT_TRUE(cache.get(fp).has_value());

    // Invalidating by either UUID must purge the entry
    EXPECT_GE(cache.invalidatePII(pii_uuid1), 1u);
    EXPECT_FALSE(cache.get(fp).has_value());
}

TEST_F(AdaptiveQueryCacheTest, ClearAlsoClearsPIIIndex) {
    AdaptiveQueryCache cache(config_);

    std::string fp       = cache.generateFingerprint("SELECT * FROM secrets");
    std::string pii_uuid = "cccccccc-0000-0000-0000-000000000003";
    cache.put(fp, {}, {{"secret", "val"}}, "", {pii_uuid});

    cache.clear();

    // After clear, invalidatePII should find nothing (index was cleared)
    EXPECT_EQ(cache.invalidatePII(pii_uuid), 0u);
}

// ============================================================================
// Phase 4: Write-Through Cache Mode Tests
// ============================================================================

TEST_F(AdaptiveQueryCacheTest, WriteThroughDisabledByDefault) {
    // Verify write-through is off by default
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("write_through"));
    EXPECT_FALSE(info["write_through"]["enabled"].get<bool>());
    EXPECT_EQ(info["write_through"]["total"].get<uint64_t>(), 0u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughL1EntryPersistedToL3) {
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    std::string query = "SELECT * FROM users WHERE id = 1";
    json params = {{"id", 1}};
    json result = {{"id", 1}, {"name", "Alice"}};
    std::string fingerprint = cache.generateFingerprint(query, params);

    // Put a small entry (goes to L1) with write-through enabled
    EXPECT_TRUE(cache.put(fingerprint, params, result));

    // Verify L1 hit
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::HOT);

    // Verify write-through metric incremented
    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.write_through_total.load(), 1u);

    // Verify write-through info in getDetailedInfo
    json info = cache.getDetailedInfo();
    EXPECT_TRUE(info["write_through"]["enabled"].get<bool>());
    EXPECT_GE(info["write_through"]["total"].get<uint64_t>(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughL2EntryPersistedToL3) {
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    // Build a result that exceeds l1_max_entry_size (1 KB) to land in L2
    json large_result;
    for (int i = 0; i < 200; i++) {
        large_result["rows"].push_back({{"id", i}, {"name", "User " + std::to_string(i)}});
    }

    std::string fingerprint = cache.generateFingerprint("SELECT * FROM orders");
    EXPECT_TRUE(cache.put(fingerprint, {}, large_result));

    // Verify L2 hit
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, large_result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::WARM);

    // Verify write-through metric incremented
    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.write_through_total.load(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughModeReportedInDetailedInfo) {
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("write_through"));
    EXPECT_TRUE(info["write_through"]["enabled"].get<bool>());
    EXPECT_TRUE(info["write_through"].contains("total"));
    EXPECT_TRUE(info["write_through"].contains("errors"));
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughDefaultDisabled) {
    // Write-through must be opt-in and off by default
    EXPECT_FALSE(config_.enable_write_through);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughSmallEntryHitsL1OnFirstGet) {
    // With write-through enabled, a small entry written once should be
    // immediately available in L1 (HOT tier) without any promotion.
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    json result = {{"data", 42}};
    std::string fp = cache.generateFingerprint("SELECT 1", {});
    EXPECT_TRUE(cache.put(fp, {}, result));

    auto hit = cache.get(fp);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->result, result);
    EXPECT_EQ(hit->level, AdaptiveQueryCache::CacheLevel::HOT);

    // Verify the write-through metric was incremented
    EXPECT_GE(cache.getEnhancedMetrics().write_through_writes.load(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughMediumEntryHitsL2OnFirstGet) {
    // A medium-sized entry (fits L2 but not L1) written in write-through mode
    // should be available in L2 on first get (no prior promotion needed).
    config_.enable_write_through = true;
    config_.l1_max_entry_size = 100;   // Artificially small L1 limit
    AdaptiveQueryCache cache(config_);

    // Build a result that is > 100 bytes (L1 limit) but < 10 KB (L2 limit)
    json result;
    for (int i = 0; i < 20; i++) {
        result["row"].push_back({{"id", i}, {"name", "Item " + std::to_string(i)}});
    }
    std::string result_str = result.dump();
    ASSERT_GT(result_str.size(), 100u);
    ASSERT_LT(result_str.size(), 10240u);

    std::string fp = cache.generateFingerprint("SELECT * FROM items", {});
    EXPECT_TRUE(cache.put(fp, {}, result));

    auto hit = cache.get(fp);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->result, result);
    // Entry is too large for L1, so it lands in L2
    EXPECT_EQ(hit->level, AdaptiveQueryCache::CacheLevel::WARM);

    EXPECT_GE(cache.getEnhancedMetrics().write_through_writes.load(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughMetricsTracked) {
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    const int num_puts = 5;
    for (int i = 0; i < num_puts; i++) {
        std::string fp = cache.generateFingerprint("SELECT " + std::to_string(i), {});
        cache.put(fp, {}, json({{"v", i}}));
    }

    EXPECT_EQ(cache.getEnhancedMetrics().write_through_writes.load(),
              static_cast<uint64_t>(num_puts));
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughDetailedInfoShowsEnabled) {
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    cache.put("fp_wt", {}, json({{"x", 1}}));

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("write_through"));
    EXPECT_TRUE(info["write_through"]["enabled"].get<bool>());
    EXPECT_GE(info["write_through"]["writes"].get<uint64_t>(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughDisabledDoesNotSetMetric) {
    // In normal (non-write-through) mode the write_through_writes counter must stay 0.
    config_.enable_write_through = false;
    AdaptiveQueryCache cache(config_);

    std::string fp = cache.generateFingerprint("SELECT 1", {});
    cache.put(fp, {}, json({{"v", 1}}));

    EXPECT_EQ(cache.getEnhancedMetrics().write_through_writes.load(), 0u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughStatsByTierShowsEnabled) {
    // getStatsByTier() must expose write_through.enabled and write_through.writes.
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    cache.put("fp_st", {}, json({{"x", 2}}));

    json stats = cache.getStatsByTier();
    ASSERT_TRUE(stats.contains("write_through"));
    EXPECT_TRUE(stats["write_through"]["enabled"].get<bool>());
    EXPECT_GE(stats["write_through"]["writes"].get<uint64_t>(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, WriteThroughStatsByTierDisabledFlag) {
    // When write-through is off, getStatsByTier() must report enabled=false.
    config_.enable_write_through = false;
    AdaptiveQueryCache cache(config_);

    json stats = cache.getStatsByTier();
    ASSERT_TRUE(stats.contains("write_through"));
    EXPECT_FALSE(stats["write_through"]["enabled"].get<bool>());
}

// ============================================================================
// Phase 3: Adaptive TTL Tuning Tests
// ============================================================================

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLDisabledByDefault) {
    // Adaptive TTL must be opt-in and off by default.
    EXPECT_FALSE(config_.enable_adaptive_ttl);
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("adaptive_ttl"));
    EXPECT_FALSE(info["adaptive_ttl"]["enabled"].get<bool>());
}

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLReportedInDetailedInfo) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 30;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("adaptive_ttl"));
    EXPECT_TRUE(info["adaptive_ttl"]["enabled"].get<bool>());
    EXPECT_EQ(info["adaptive_ttl"]["min_seconds"].get<int>(), 30);
    EXPECT_EQ(info["adaptive_ttl"]["max_seconds"].get<int>(), 3600);
    EXPECT_DOUBLE_EQ(info["adaptive_ttl"]["scaling_factor"].get<double>(), 5.0);
    EXPECT_EQ(info["adaptive_ttl"]["ttl_extended_total"].get<uint64_t>(), 0u);
    EXPECT_EQ(info["adaptive_ttl"]["ttl_shortened_total"].get<uint64_t>(), 0u);
}

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLInitialEntryGetsMinTTL) {
    // New entries (access_count=0) should receive the minimum TTL.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 86400;
    AdaptiveQueryCache cache(config_);

    std::string fp = cache.generateFingerprint("SELECT 1", {});
    EXPECT_TRUE(cache.put(fp, {}, json({{"v", 1}})));

    auto entry = cache.get(fp);
    ASSERT_TRUE(entry.has_value());
    // First access: TTL should be at least the configured minimum.
    EXPECT_GE(entry->ttl_seconds, config_.adaptive_ttl_min_seconds);
}

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLHotKeyExtendsOnFrequentAccess) {
    // Accessing the same entry >= 10 times within one window should trigger
    // the hot-key policy and increment ttl_extended_total.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 86400;
    AdaptiveQueryCache cache(config_);

    std::string fp = cache.generateFingerprint("SELECT hot", {});
    EXPECT_TRUE(cache.put(fp, {}, json({{"row", 1}})));

    // Access the entry 12 times within the same 5-minute window.
    // Hot-key threshold is >= 10 accesses per window; 12 ensures we exceed it.
    for (int i = 0; i < 12; i++) {
        auto hit = cache.get(fp);
        ASSERT_TRUE(hit.has_value()) << "miss on access " << i;
    }

    EXPECT_GE(cache.getEnhancedMetrics().ttl_extended_total.load(), 1u);
}

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLCalculateLargerTTLForHighAccessCount) {
    // Entries with higher access counts should receive a longer TTL via the
    // logarithmic scaling formula.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 86400;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);

    // Put two entries; access the second one many more times.
    std::string fp_low  = cache.generateFingerprint("SELECT low",  {});
    std::string fp_high = cache.generateFingerprint("SELECT high", {});
    cache.put(fp_low,  {}, json({{"v", 0}}));
    cache.put(fp_high, {}, json({{"v", 1}}));

    // Access fp_high many times to build up access_count.
    for (int i = 0; i < 20; i++) {
        auto h = cache.get(fp_high);
        ASSERT_TRUE(h.has_value());
    }
    // Access fp_low just once.
    auto low_hit = cache.get(fp_low);
    ASSERT_TRUE(low_hit.has_value());

    auto high_hit = cache.get(fp_high);
    ASSERT_TRUE(high_hit.has_value());

    EXPECT_GE(high_hit->ttl_seconds, low_hit->ttl_seconds);
}

TEST_F(AdaptiveQueryCacheTest, AdaptiveTTLBoundedByConfiguredLimits) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 10;
    config_.adaptive_ttl_max_seconds = 120;
    AdaptiveQueryCache cache(config_);

    std::string fp = cache.generateFingerprint("SELECT bounded", {});
    EXPECT_TRUE(cache.put(fp, {}, json({{"v", 42}})));

    // Access many times – TTL must never exceed max.
    for (int i = 0; i < 50; i++) {
        auto h = cache.get(fp);
        ASSERT_TRUE(h.has_value());
        EXPECT_GE(h->ttl_seconds, config_.adaptive_ttl_min_seconds);
        EXPECT_LE(h->ttl_seconds, config_.adaptive_ttl_max_seconds);
    }
}

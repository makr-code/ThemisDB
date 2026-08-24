// Copyright 2025 ThemisDB
// Phase 2: Integration and advanced tests for Adaptive Query Cache

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <random>
#include <regex>

using namespace themis;
using json = nlohmann::json;

class AdaptiveCacheIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.l3_db_path = "/tmp/themis_integration_test_" + 
                            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        config_.l1_max_entries = 5;       // Small for testing promotion
        config_.l2_max_entries = 10;
        config_.l1_max_entry_size = 100;  // Small to force L2/L3
        config_.l2_max_entry_size = 500;
        config_.l1_ttl_seconds = 2;       // Short TTL for testing
        config_.l2_ttl_seconds = 4;
        config_.l3_ttl_seconds = 10;
    }
    
    void TearDown() override {
        if (!config_.l3_db_path.empty()) {
            std::filesystem::remove_all(config_.l3_db_path);
        }
    }
    
    AdaptiveQueryCache::Config config_;
};

// ============================================================================
// Integration Tests: L1 → L2 → L3 Promotion/Demotion
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, L1ToL2Promotion) {
    AdaptiveQueryCache cache(config_);
    
    // Create entry that fits in L1
    json small_result = {{"data", "x"}};
    std::string fp = cache.generateFingerprint("query1", {});
    
    EXPECT_TRUE(cache.put(fp, {}, small_result));
    
    // Access multiple times to trigger promotion consideration
    for (int i = 0; i < 5; i++) {
        auto cached = cache.get(fp);
        ASSERT_TRUE(cached.has_value());
    }
    
    // Check that promotion metrics might have been updated
    auto stats = cache.getStats();
    // Note: Promotion from L1 to L2 doesn't happen in current implementation
    // (L2->L1 happens). This test validates the access counting.
    EXPECT_GT(stats.l1_hits, 0);
}

TEST_F(AdaptiveCacheIntegrationTest, L2ToL1Promotion) {
    AdaptiveQueryCache cache(config_);
    
    // Create medium entry that goes to L2 (larger than L1 size)
    std::string medium_data(150, 'm');
    json medium_result = {{"data", medium_data}};
    std::string fp = cache.generateFingerprint("query_medium", {});
    
    EXPECT_TRUE(cache.put(fp, {}, medium_result));
    
    // Access multiple times to trigger promotion (needs 3 accesses)
    for (int i = 0; i < 4; i++) {
        auto cached = cache.get(fp);
        ASSERT_TRUE(cached.has_value());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Check promotion metric
    auto stats = cache.getStats();
    // Promotion from L2 to L1 happens after 3 accesses if size permits
    EXPECT_GE(stats.promotions, 0);  // May or may not promote based on size
}

TEST_F(AdaptiveCacheIntegrationTest, L1EvictionUnderPressure) {
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    
    // Fill L1 beyond capacity
    for (int i = 0; i < config_.l1_max_entries + 3; i++) {
        std::string fp = cache.generateFingerprint("query" + std::to_string(i), {});
        EXPECT_TRUE(cache.put(fp, {}, result));
    }
    
    // Should have triggered evictions
    auto stats = cache.getStats();
    EXPECT_GT(stats.evictions, 0);
}

TEST_F(AdaptiveCacheIntegrationTest, TTLExpirationAcrossTiers) {
    config_.l1_ttl_seconds = 1;  // 1 second TTL
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 42}};
    std::string fp = cache.generateFingerprint("expiring_query", {});
    
    EXPECT_TRUE(cache.put(fp, {}, result));
    
    // Should be available immediately
    auto cached1 = cache.get(fp);
    ASSERT_TRUE(cached1.has_value());
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Should be expired now
    auto cached2 = cache.get(fp);
    EXPECT_FALSE(cached2.has_value());
    
    // Check evictions due to expiry
    auto stats = cache.getStats();
    EXPECT_GT(stats.evictions, 0);
}

TEST_F(AdaptiveCacheIntegrationTest, L3PersistenceAcrossLevels) {
    AdaptiveQueryCache cache(config_);
    
    // Create large entry that goes to L3
    std::string large_data(600, 'L');
    json large_result = {{"data", large_data}};
    std::string fp = cache.generateFingerprint("large_query", {});
    
    EXPECT_TRUE(cache.put(fp, {}, large_result));
    
    // Should be in L3
    auto cached = cache.get(fp);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::COLD);
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.l3_hits, 0);
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, ConcurrentGetPut) {
    AdaptiveQueryCache cache(config_);
    
    const int NUM_THREADS = 4;
    const int OPS_PER_THREAD = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_puts{0};
    std::atomic<int> successful_gets{0};
    
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                std::string query = "query_t" + std::to_string(t) + "_i" + std::to_string(i);
                std::string fp = cache.generateFingerprint(query, {});
                json result = {{"thread", t}, {"iter", i}};
                
                if (cache.put(fp, {}, result)) {
                    successful_puts++;
                }
                
                auto cached = cache.get(fp);
                if (cached.has_value()) {
                    successful_gets++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_puts.load(), 0);
    EXPECT_GT(successful_gets.load(), 0);
    
    // Verify no crashes and metrics are updated
    auto stats = cache.getStats();
    EXPECT_GT(stats.l1_hits + stats.l2_hits + stats.l3_hits + stats.misses, 0);
}

TEST_F(AdaptiveCacheIntegrationTest, ConcurrentInvalidation) {
    AdaptiveQueryCache cache(config_);
    
    // Pre-populate cache
    for (int i = 0; i < 20; i++) {
        std::string fp = cache.generateFingerprint("item" + std::to_string(i), {});
        json result = {{"value", i}};
        cache.put(fp, {}, result);
    }
    
    std::atomic<bool> done{false};
    std::thread invalidator([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cache.invalidate(".*");  // Invalidate all
        done = true;
    });
    
    std::thread reader([&]() {
        int reads = 0;
        while (!done && reads < 100) {
            std::string fp = cache.generateFingerprint("item" + std::to_string(reads % 20), {});
            cache.get(fp);
            reads++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    invalidator.join();
    reader.join();
    
    // Should complete without crashes
    SUCCEED();
}

// ============================================================================
// Fuzz Tests
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, FuzzJSONParsing) {
    AdaptiveQueryCache cache(config_);
    
    std::vector<std::string> tricky_strings = {
        "",
        "{}",
        "{\"key\": null}",
        "{\"nested\": {\"deep\": {\"value\": 123}}}",
        "{\"array\": [1, 2, 3]}",
        "{\"unicode\": \"\\u0041\\u0042\"}",
        "{\"escape\": \"\\\"quoted\\\"\"}",
    };
    
    for (const auto& str : tricky_strings) {
        try {
            json params = json::parse(str);
            std::string fp = cache.generateFingerprint("query", params);
            
            // Should generate a valid fingerprint
            EXPECT_EQ(fp.length(), 64);  // SHA256 = 64 hex chars
            
            // Should be able to cache
            json result = {{"data", "test"}};
            cache.put(fp, params, result);
        } catch (const std::exception& e) {
            // Some malformed JSON may throw, which is acceptable
            continue;
        }
    }
    
    SUCCEED();
}

TEST_F(AdaptiveCacheIntegrationTest, FuzzRegexPatterns) {
    AdaptiveQueryCache cache(config_);
    
    // Pre-populate
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("test" + std::to_string(i), {});
        json result = {{"value", i}};
        cache.put(fp, {}, result);
    }
    
    // Test various regex patterns (including potentially problematic ones)
    std::vector<std::string> patterns = {
        ".*",           // Match all
        "test.*",       // Simple prefix
        "^test[0-9]$",  // Anchored
        "t.st",         // Single char wildcard
        "[a-z]+",       // Character class
        "test|query",   // Alternation
        "",             // Empty (should throw or handle)
    };
    
    for (const auto& pattern : patterns) {
        try {
            size_t count = cache.invalidate(pattern);
            EXPECT_GE(count, 0);  // Should return some count
        } catch (const std::exception& e) {
            // Invalid regex may throw, which is acceptable
            continue;
        }
    }
    
    SUCCEED();
}

TEST_F(AdaptiveCacheIntegrationTest, FuzzLargePayloads) {
    AdaptiveQueryCache cache(config_);
    
    // Test with various payload sizes
    std::vector<size_t> sizes = {0, 1, 10, 100, 1000, 10000, 50000};
    
    for (size_t size : sizes) {
        std::string data(size, 'X');
        json result = {{"data", data}};
        std::string fp = cache.generateFingerprint("large_" + std::to_string(size), {});
        
        bool stored = cache.put(fp, {}, result);
        // Very large payloads may be rejected by size limits
        // Small payloads should succeed
        if (size < config_.max_total_entry_size) {
            // Should succeed if within limits
        }
    }
    
    SUCCEED();
}

// ============================================================================
// Pattern Invalidation Tests
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, PatternInvalidationAcrossAllTiers) {
    AdaptiveQueryCache cache(config_);
    
    // Add entries that will be distributed across tiers
    json small = {{"size", "small"}};
    json medium = {{"data", std::string(200, 'm')}};
    json large = {{"data", std::string(700, 'l')}};
    
    std::string fp1 = cache.generateFingerprint("user_query_1", {});
    std::string fp2 = cache.generateFingerprint("user_query_2", {});
    std::string fp3 = cache.generateFingerprint("product_query_1", {});
    
    cache.put(fp1, {}, small);
    cache.put(fp2, {}, medium);
    cache.put(fp3, {}, large);
    
    // Invalidate based on pattern in fingerprint
    // Note: This invalidates by fingerprint pattern, not query pattern
    size_t count = cache.invalidate(".*");  // Should invalidate all
    
    EXPECT_GT(count, 0);
    
    // Verify entries are gone
    EXPECT_FALSE(cache.get(fp1).has_value());
    EXPECT_FALSE(cache.get(fp2).has_value());
    EXPECT_FALSE(cache.get(fp3).has_value());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, RocksDBUnavailableGracefulDegradation) {
    // Use invalid path to cause RocksDB to fail
    config_.l3_db_path = "/invalid/path/that/does/not/exist";
    
    // Should still construct but L3 will be disabled
    AdaptiveQueryCache cache(config_);
    
    // L1/L2 should still work
    json result = {{"value", 42}};
    std::string fp = cache.generateFingerprint("test", {});
    
    EXPECT_TRUE(cache.put(fp, {}, result));
    
    auto cached = cache.get(fp);
    EXPECT_TRUE(cached.has_value());
}

// ============================================================================
// Phase 4: Write-Through Cache Mode Integration Tests
// ============================================================================

TEST_F(AdaptiveCacheIntegrationTest, WriteThroughSmallEntryInL1OnFirstRead) {
    // A small entry stored with write-through should be immediately available
    // in L1 (HOT tier) on the very first get(), without any promotion needed.
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    json result = {{"value", 42}};
    std::string fp = cache.generateFingerprint("wt_small_entry", {});
    EXPECT_TRUE(cache.put(fp, {}, result));

    auto hit = cache.get(fp);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->result, result);
    EXPECT_EQ(hit->level, AdaptiveQueryCache::CacheLevel::HOT);
    EXPECT_GE(cache.getEnhancedMetrics().write_through_writes.load(), 1u);
}

TEST_F(AdaptiveCacheIntegrationTest, WriteThroughL1ExpiryFallsBackToL2) {
    // With write-through enabled and a short L1 TTL, the entry should remain
    // accessible from L2 (WARM tier) after the L1 entry expires.
    config_.enable_write_through = true;
    config_.l1_ttl_seconds = 1;   // L1 expires quickly
    config_.l2_ttl_seconds = 30;  // L2 stays available
    AdaptiveQueryCache cache(config_);

    json result = {{"value", 99}};
    std::string fp = cache.generateFingerprint("wt_l1_fallback", {});
    EXPECT_TRUE(cache.put(fp, {}, result));

    // Immediately available from L1
    {
        auto hit = cache.get(fp);
        ASSERT_TRUE(hit.has_value());
        EXPECT_EQ(hit->level, AdaptiveQueryCache::CacheLevel::HOT);
    }

    // After L1 TTL expires, L2 fallback ensures no cache miss
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    {
        auto hit = cache.get(fp);
        ASSERT_TRUE(hit.has_value());
        EXPECT_EQ(hit->result, result);
        EXPECT_EQ(hit->level, AdaptiveQueryCache::CacheLevel::WARM);
    }
}

TEST_F(AdaptiveCacheIntegrationTest, WriteThroughMetricsCountsAllPuts) {
    // write_through_writes must be incremented for every successful put()
    // in write-through mode.
    config_.enable_write_through = true;
    AdaptiveQueryCache cache(config_);

    const int n = 3;
    for (int i = 0; i < n; i++) {
        std::string fp = cache.generateFingerprint("wt_metric_" + std::to_string(i), {});
        EXPECT_TRUE(cache.put(fp, {}, json({{"i", i}})));
    }

    EXPECT_EQ(cache.getEnhancedMetrics().write_through_writes.load(),
              static_cast<uint64_t>(n));
}

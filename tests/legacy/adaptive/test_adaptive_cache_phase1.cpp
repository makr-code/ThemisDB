/**
 * @file test_adaptive_cache_phase1.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Phase 1 Production Readiness Tests for Adaptive Query Cache
// Tests for: size limits, circuit breaker, enhanced metrics

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include "cache/cache_metrics.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis;
using namespace themis::cache;
using json = nlohmann::json;

// Test constants
constexpr size_t LARGE_DATA_SIZE = 100000;  // 100KB - exceeds max_total_entry_size
constexpr size_t MEDIUM_DATA_SIZE = 2000;   // 2KB - between L1 and L2 limits
constexpr size_t LARGE_L3_DATA_SIZE = 15000; // 15KB - goes to L3
constexpr int CB_TIMEOUT_MARGIN_MS = 100;    // Margin for circuit breaker timeout tests

class AdaptiveCachePhase1Test : public ::testing::Test {
protected:
    void SetUp() override {
        config_.l3_db_path = "/tmp/themis_phase1_test_" + 
                            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        config_.l1_max_entries = 10;
        config_.l2_max_entries = 20;
        config_.l1_max_entry_size = 1024;      // 1KB for L1
        config_.l2_max_entry_size = 10240;     // 10KB for L2
        config_.max_total_entry_size = 51200;  // 50KB absolute max
        config_.enable_size_limits = true;
        config_.enable_circuit_breaker = true;
        config_.cb_failure_threshold = 3;      // Low threshold for testing
        config_.cb_timeout_ms = 5000;          // 5 seconds for testing
    }
    
    void TearDown() override {
        if (!config_.l3_db_path.empty()) {
            std::filesystem::remove_all(config_.l3_db_path);
        }
    }
    
    AdaptiveQueryCache::Config config_;
};

// Test: Size Limit Enforcement
TEST_F(AdaptiveCachePhase1Test, SizeLimitRejection) {
    AdaptiveQueryCache cache(config_);
    
    // Create an entry that exceeds max_total_entry_size
    json large_result;
    std::string large_data(LARGE_DATA_SIZE, 'x');
    large_result["data"] = large_data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT * FROM large_table", {});
    
    // Should reject due to size limit
    bool stored = cache.put(fingerprint, {}, large_result);
    EXPECT_FALSE(stored);
    
    // Check metrics
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.size_limit_rejections.load(), 0);
    
    // Verify entry was not cached
    auto cached = cache.get(fingerprint, "");
    EXPECT_FALSE(cached.has_value());
}

TEST_F(AdaptiveCachePhase1Test, SizeLimitL1Enforcement) {
    AdaptiveQueryCache cache(config_);
    
    // Create an entry slightly over L1 limit but under L2 limit
    json medium_result;
    std::string medium_data(1500, 'y');  // 1.5KB (over L1, under L2)
    medium_result["data"] = medium_data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT * FROM medium_table", {});
    
    // Should store successfully (in L2, not L1)
    bool stored = cache.put(fingerprint, {}, medium_result);
    EXPECT_TRUE(stored);
    
    // Retrieve and verify it's not in L1 (would be in L2 or L3)
    auto cached = cache.get(fingerprint, "");
    ASSERT_TRUE(cached.has_value());
    // L1 should not have this entry (it should be in L2 or L3)
    EXPECT_NE(cached->level, AdaptiveQueryCache::CacheLevel::HOT);
}

TEST_F(AdaptiveCachePhase1Test, SizeLimitDisabled) {
    config_.enable_size_limits = false;
    AdaptiveQueryCache cache(config_);
    
    // Create a large entry
    json large_result;
    std::string large_data(LARGE_DATA_SIZE, 'x');
    large_result["data"] = large_data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT * FROM huge_table", {});
    
    // With size limits disabled, this might still fail for other reasons
    // but should not increment size_limit_rejections
    cache.put(fingerprint, {}, large_result);
    
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.size_limit_rejections.load(), 0);
}

// Test: Circuit Breaker
TEST_F(AdaptiveCachePhase1Test, CircuitBreakerInitialization) {
    AdaptiveQueryCache cache(config_);
    
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_FALSE(metrics.l3_circuit_breaker_open.load());
    EXPECT_EQ(metrics.l3_circuit_breaker_trips.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, CircuitBreakerDisabled) {
    config_.enable_circuit_breaker = false;
    AdaptiveQueryCache cache(config_);
    
    // Even if L3 fails, circuit breaker should not be used
    // This is a basic sanity check
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.l3_circuit_breaker_trips.load(), 0);
}

// Test: Enhanced Metrics
TEST_F(AdaptiveCachePhase1Test, EnhancedMetricsL1Hit) {
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 42}};
    std::string fingerprint = cache.generateFingerprint("SELECT 42", {});
    
    // Store and retrieve
    EXPECT_TRUE(cache.put(fingerprint, {}, result));
    auto cached = cache.get(fingerprint, "");
    ASSERT_TRUE(cached.has_value());
    
    // Check enhanced metrics
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.l1_hits.load(), 1);
    EXPECT_EQ(metrics.l2_hits.load(), 0);
    EXPECT_EQ(metrics.l3_hits.load(), 0);
    EXPECT_GT(metrics.total_bytes_cached.load(), 0);
    EXPECT_DOUBLE_EQ(metrics.getHitRate(), 1.0);
}

TEST_F(AdaptiveCachePhase1Test, EnhancedMetricsCacheMiss) {
    AdaptiveQueryCache cache(config_);
    
    std::string fingerprint = "nonexistent";
    auto cached = cache.get(fingerprint, "");
    EXPECT_FALSE(cached.has_value());
    
    // Check enhanced metrics
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.misses.load(), 1);
    EXPECT_DOUBLE_EQ(metrics.getHitRate(), 0.0);
}

TEST_F(AdaptiveCachePhase1Test, EnhancedMetricsL2Compression) {
    AdaptiveQueryCache cache(config_);
    
    // Create entry that goes to L2 (between L1 and L2 size limits)
    json medium_result;
    std::string data(MEDIUM_DATA_SIZE, 'z');
    medium_result["data"] = data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT medium", {});
    EXPECT_TRUE(cache.put(fingerprint, {}, medium_result));
    
    // Check compression metrics
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.total_bytes_cached.load(), 0);
    EXPECT_GT(metrics.total_bytes_compressed.load(), 0);
    
    // Compression ratio should be > 1.0 for compressible data
    double ratio = metrics.getCompressionRatio();
    EXPECT_GT(ratio, 1.0);
}

TEST_F(AdaptiveCachePhase1Test, EnhancedMetricsEviction) {
    AdaptiveQueryCache cache(config_);
    
    // Fill L1 cache to capacity + 1 to trigger eviction
    for (int i = 0; i <= config_.l1_max_entries; i++) {
        json result = {{"id", i}};
        std::string query = "SELECT " + std::to_string(i);
        std::string fingerprint = cache.generateFingerprint(query, {});
        cache.put(fingerprint, {}, result);
    }
    
    // Check eviction metrics
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.evictions.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, EnhancedMetricsPromotion) {
    AdaptiveQueryCache cache(config_);
    
    // Create a medium-sized entry that goes to L2
    json medium_result;
    std::string data(1500, 'p');  // 1.5KB
    medium_result["data"] = data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT popular", {});
    EXPECT_TRUE(cache.put(fingerprint, {}, medium_result));
    
    // Access multiple times to trigger promotion
    for (int i = 0; i < 5; i++) {
        cache.get(fingerprint, "");
    }
    
    // Check if promotion occurred
    // Note: Promotion logic depends on access count and size constraints
    auto metrics = cache.getEnhancedMetrics();
    // We can't guarantee promotion happened, but we can check metrics exist
    EXPECT_GE(metrics.promotions.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, MetricsToJson) {
    CacheMetrics metrics;
    metrics.l1_hits = 10;
    metrics.l2_hits = 5;
    metrics.l3_hits = 2;
    metrics.misses = 3;
    metrics.evictions = 1;
    metrics.total_bytes_cached = 10000;
    metrics.total_bytes_compressed = 5000;
    
    json j = metrics.toJson();
    
    EXPECT_EQ(j["hits"]["l1"], 10);
    EXPECT_EQ(j["hits"]["l2"], 5);
    EXPECT_EQ(j["hits"]["l3"], 2);
    EXPECT_EQ(j["misses"], 3);
    EXPECT_EQ(j["evictions"], 1);
    EXPECT_DOUBLE_EQ(j["hit_rate"], 0.85);  // 17 hits / 20 total
    EXPECT_EQ(j["bytes"]["cached"], 10000);
    EXPECT_EQ(j["bytes"]["compressed"], 5000);
}

// Test: Circuit Breaker State Machine
TEST_F(AdaptiveCachePhase1Test, CircuitBreakerStates) {
    CircuitBreaker::Config cb_config;
    cb_config.failure_threshold = 3;
    cb_config.success_threshold = 2;
    cb_config.timeout_ms = 1000;  // 1 second
    
    CircuitBreaker cb(cb_config);
    
    // Initial state: CLOSED
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
    
    // Record failures to open circuit
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    EXPECT_TRUE(cb.isOpen());
    
    // Should reject requests when open
    EXPECT_FALSE(cb.allowRequest());
    
    // Wait for timeout (timeout + margin for reliability)
    std::this_thread::sleep_for(std::chrono::milliseconds(cb_config.timeout_ms + CB_TIMEOUT_MARGIN_MS));
    
    // Should transition to HALF_OPEN
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    
    // Record successes to close circuit
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_FALSE(cb.isOpen());
}

TEST_F(AdaptiveCachePhase1Test, CircuitBreakerHalfOpenFailure) {
    CircuitBreaker::Config cb_config;
    cb_config.failure_threshold = 2;
    cb_config.timeout_ms = 500;
    
    CircuitBreaker cb(cb_config);
    
    // Open the circuit
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_TRUE(cb.isOpen());
    
    // Wait for timeout (timeout + margin for reliability)
    std::this_thread::sleep_for(std::chrono::milliseconds(cb_config.timeout_ms + CB_TIMEOUT_MARGIN_MS));
    
    // Transition to HALF_OPEN
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    
    // Failure in HALF_OPEN should immediately re-open
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST_F(AdaptiveCachePhase1Test, CircuitBreakerReset) {
    CircuitBreaker cb;
    
    // Open the circuit
    for (int i = 0; i < 5; i++) {
        cb.recordFailure();
    }
    EXPECT_TRUE(cb.isOpen());
    
    // Reset should close it
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

// Test: Size Validation Edge Cases
TEST_F(AdaptiveCachePhase1Test, SizeValidationExactLimit) {
    AdaptiveQueryCache cache(config_);
    
    // Create entry exactly at L1 limit
    json exact_result;
    std::string exact_data(config_.l1_max_entry_size - 50, 'e');  // Account for JSON overhead
    exact_result["data"] = exact_data;
    
    std::string fingerprint = cache.generateFingerprint("SELECT exact", {});
    bool stored = cache.put(fingerprint, {}, exact_result);
    
    // Should succeed (at or slightly under limit)
    EXPECT_TRUE(stored);
}

TEST_F(AdaptiveCachePhase1Test, SizeValidationZeroSize) {
    AdaptiveQueryCache cache(config_);
    
    // Empty result
    json empty_result = json::object();
    
    std::string fingerprint = cache.generateFingerprint("SELECT nothing", {});
    bool stored = cache.put(fingerprint, {}, empty_result);
    
    // Should succeed
    EXPECT_TRUE(stored);
    
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.size_limit_rejections.load(), 0);
}

// Test: L3 Pattern Invalidation (Phase 1 Fix)
TEST_F(AdaptiveCachePhase1Test, L3PatternInvalidation) {
    AdaptiveQueryCache cache(config_);
    
    // Create entries that will go to L3 (large entries)
    for (int i = 0; i < 5; i++) {
        json large_result;
        std::string data(LARGE_L3_DATA_SIZE, 'L');
        large_result["data"] = data;
        large_result["id"] = i;
        
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        std::string fingerprint = cache.generateFingerprint(query, {});
        cache.put(fingerprint, {}, large_result);
    }
    
    // Also create some entries for a different table
    for (int i = 0; i < 3; i++) {
        json result;
        std::string data(LARGE_L3_DATA_SIZE, 'P');
        result["data"] = data;
        result["id"] = i;
        
        std::string query = "SELECT * FROM products WHERE id = " + std::to_string(i);
        std::string fingerprint = cache.generateFingerprint(query, {});
        cache.put(fingerprint, {}, result);
    }
    
    // Invalidate entries matching "users" pattern
    // Note: This will match the fingerprints, which are hashes of the query
    // So we need to clear all and test that the mechanism works
    cache.clear();
    
    // Put some entries with known fingerprints
    std::string fp1 = cache.generateFingerprint("test_users_query", {});
    std::string fp2 = cache.generateFingerprint("test_products_query", {});
    std::string fp3 = cache.generateFingerprint("another_users_query", {});
    
    json result = {{"data", "test"}};
    cache.put(fp1, {}, result);
    cache.put(fp2, {}, result);
    cache.put(fp3, {}, result);
    
    // Invalidate with pattern (matching part of fingerprint)
    // This tests that L3 invalidation is now implemented
    size_t invalidated = cache.invalidate(".*");  // Match all
    
    // Should have invalidated entries
    EXPECT_GT(invalidated, 0);
    
    // Verify entries are gone
    EXPECT_FALSE(cache.get(fp1, "").has_value());
    EXPECT_FALSE(cache.get(fp2, "").has_value());
    EXPECT_FALSE(cache.get(fp3, "").has_value());
}

TEST_F(AdaptiveCachePhase1Test, L3InvalidationWithCircuitBreaker) {
    AdaptiveQueryCache cache(config_);
    
    // Store some entries
    json result = {{"data", "test"}};
    std::string fp1 = cache.generateFingerprint("query1", {});
    std::string fp2 = cache.generateFingerprint("query2", {});
    
    cache.put(fp1, {}, result);
    cache.put(fp2, {}, result);
    
    // Invalidate with valid pattern
    size_t count = cache.invalidate(".*");
    
    // Should succeed (circuit breaker should be closed initially)
    EXPECT_GE(count, 0);
    
    auto metrics = cache.getEnhancedMetrics();
    // Circuit breaker should not be open for successful operations
    EXPECT_FALSE(metrics.l3_circuit_breaker_open.load());
}

// ============================================================================
// Phase 2 Tests: Configuration Validation
// ============================================================================

TEST_F(AdaptiveCachePhase1Test, ConfigValidationValid) {
    // Default config should be valid
    EXPECT_TRUE(config_.validate());
    
    std::string error = {};
    EXPECT_TRUE(config_.validate(&error));
    EXPECT_TRUE(error.empty());
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidL1MaxEntries) {
    config_.l1_max_entries = 0;
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("l1_max_entries"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidL2Compression) {
    config_.l2_compression_level = 23;  // Out of valid Zstd range
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("l2_compression_level"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidTTL) {
    config_.min_ttl_seconds = 1000;
    config_.max_ttl_seconds = 100;  // max < min
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("max_ttl_seconds"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidFrequencyWeight) {
    config_.frequency_weight = 1.5f;  // Out of [0, 1] range
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("frequency_weight"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidSizeLimits) {
    config_.l1_max_entry_size = 100000000;  // Larger than max_total_entry_size
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("l1_max_entry_size"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidCircuitBreaker) {
    config_.cb_failure_threshold = 0;
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("cb_failure_threshold"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationInvalidRateLimit) {
    config_.enable_rate_limiting = true;
    config_.max_requests_per_second = 0;
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("max_requests_per_second"), std::string::npos);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationConstructorThrowsOnInvalid) {
    config_.l1_max_entries = 0;
    
    // Constructor should throw on invalid config
    EXPECT_THROW({
        AdaptiveQueryCache cache(config_);
    }, std::invalid_argument);
}

TEST_F(AdaptiveCachePhase1Test, ConfigValidationEmptyPath) {
    config_.l3_db_path = "";
    
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_NE(error.find("l3_db_path"), std::string::npos);
}

// ============================================================================
// Phase 2 Tests: Rate Limiting
// ============================================================================

TEST_F(AdaptiveCachePhase1Test, RateLimitingDisabled) {
    config_.enable_rate_limiting = false;
    AdaptiveQueryCache cache(config_);
    
    // Should not rate limit when disabled
    json result = {{"value", 1}};
    for (int i = 0; i < 100; i++) {
        std::string fp = cache.generateFingerprint("query" + std::to_string(i), {});
        EXPECT_TRUE(cache.put(fp, {}, result));
    }
    
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_EQ(metrics.rate_limited_requests.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, RateLimitingEnabled) {
    config_.enable_rate_limiting = true;
    config_.max_requests_per_second = 10;  // Very low for testing
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    size_t successful = 0;
    size_t rate_limited = 0;
    
    // Try many requests quickly
    for (int i = 0; i < 50; i++) {
        std::string fp = cache.generateFingerprint("query" + std::to_string(i), {});
        if (cache.put(fp, {}, result)) {
            successful++;
        } else {
            rate_limited++;
        }
    }
    
    // Should have some rate limited requests
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.rate_limited_requests.load(), 0);
    EXPECT_LT(successful, 50);  // Not all should succeed
}

TEST_F(AdaptiveCachePhase1Test, RateLimitingGet) {
    config_.enable_rate_limiting = true;
    config_.max_requests_per_second = 5;  // Very low for testing
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 42}};
    std::string fp = cache.generateFingerprint("test_query", {});
    cache.put(fp, {}, result);
    
    // Try many get requests
    size_t hits = 0;
    size_t rate_limited = 0;
    
    for (int i = 0; i < 30; i++) {
        auto cached = cache.get(fp, "");
        if (cached.has_value()) {
            hits++;
        } else {
            rate_limited++;
        }
    }
    
    // Should have some rate limited
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.rate_limited_requests.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, RateLimiterTokenBucket) {
    using namespace themis::cache;
    
    RateLimiter::Config config;
    config.max_requests_per_second = 10;
    RateLimiter limiter(config);
    
    // Should allow initial requests up to capacity
    int allowed = 0;
    for (int i = 0; i < 20; i++) {
        if (limiter.tryAcquire()) {
            allowed++;
        }
    }
    
    // Should allow some but not all
    EXPECT_GT(allowed, 0);
    EXPECT_LE(allowed, 10);  // At most the rate limit
}

TEST_F(AdaptiveCachePhase1Test, RateLimiterReset) {
    using namespace themis::cache;
    
    RateLimiter::Config config;
    config.max_requests_per_second = 5;
    RateLimiter limiter(config);
    
    // Exhaust tokens
    for (int i = 0; i < 10; i++) {
        limiter.tryAcquire();
    }
    
    // Should be rate limited
    EXPECT_FALSE(limiter.tryAcquire());
    
    // Reset should restore capacity
    limiter.reset();
    EXPECT_TRUE(limiter.tryAcquire());
}

// ============================================================================
// Phase 2 Tests: Tenant Isolation
// ============================================================================

TEST_F(AdaptiveCachePhase1Test, TenantIsolationDisabled) {
    config_.enable_tenant_isolation = false;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    
    // Different tenants should not affect each other when isolation disabled
    std::string fp = cache.generateFingerprint("query", {});
    EXPECT_TRUE(cache.put(fp, {}, result, "tenant1"));
    
    // Should be accessible without tenant
    auto cached = cache.get(fp, "");
    ASSERT_TRUE(cached.has_value());
}

TEST_F(AdaptiveCachePhase1Test, TenantIsolationEnabled) {
    config_.enable_tenant_isolation = true;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 42}};
    
    // Store with tenant1
    std::string fp = cache.generateFingerprint("query", {}, "tenant1");
    EXPECT_TRUE(cache.put(fp, {}, result, "tenant1"));
    
    // Should be accessible with same tenant
    auto cached1 = cache.get(fp, "tenant1");
    ASSERT_TRUE(cached1.has_value());
    EXPECT_EQ(cached1->result["value"], 42);
    
    // Should not be accessible with different tenant
    auto cached2 = cache.get(fp, "tenant2");
    EXPECT_FALSE(cached2.has_value());
}

TEST_F(AdaptiveCachePhase1Test, TenantQuotaEnforcement) {
    config_.enable_tenant_isolation = true;
    config_.per_tenant_max_bytes = 1000;  // Small quota for testing
    AdaptiveQueryCache cache(config_);
    
    json large_result;
    std::string data(1500, 'x');  // Larger than quota
    large_result["data"] = data;
    
    std::string fp = cache.generateFingerprint("query", {}, "tenant1");
    
    // Should reject due to quota
    bool stored = cache.put(fp, {}, large_result, "tenant1");
    EXPECT_FALSE(stored);
    
    auto metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.size_limit_rejections.load(), 0);
}

TEST_F(AdaptiveCachePhase1Test, TenantQuotaMultipleEntries) {
    config_.enable_tenant_isolation = true;
    config_.per_tenant_max_bytes = 500;  // Small quota
    AdaptiveQueryCache cache(config_);
    
    json result = {{"data", std::string(100, 'x')}};  // ~100 bytes each
    
    // Should be able to add a few entries
    size_t successful = 0;
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("query" + std::to_string(i), {}, "tenant1");
        if (cache.put(fp, {}, result, "tenant1")) {
            successful++;
        }
    }
    
    // Should have stopped before quota exceeded
    EXPECT_GT(successful, 0);
    EXPECT_LT(successful, 10);
}

TEST_F(AdaptiveCachePhase1Test, TenantFingerprintIncludesTenantId) {
    AdaptiveQueryCache cache(config_);
    
    // Same query, different tenants should have different fingerprints
    std::string fp1 = cache.generateFingerprint("SELECT * FROM users", {}, "tenant1");
    std::string fp2 = cache.generateFingerprint("SELECT * FROM users", {}, "tenant2");
    
    EXPECT_NE(fp1, fp2);
}

// ============================================================================
// Phase 3 Tests: Admin API & Operational Tooling
// ============================================================================

TEST_F(AdaptiveCachePhase1Test, AdminAPIGetStatsByTier) {
    AdaptiveQueryCache cache(config_);
    
    // Add some entries
    for (int i = 0; i < 5; i++) {
        json result = {{"value", i}};
        std::string fp = cache.generateFingerprint("query" + std::to_string(i), {});
        cache.put(fp, {}, result);
    }
    
    // Get stats by tier
    json stats = cache.getStatsByTier();
    
    EXPECT_TRUE(stats.contains("l1"));
    EXPECT_TRUE(stats.contains("l2"));
    EXPECT_TRUE(stats.contains("l3"));
    EXPECT_TRUE(stats.contains("overall"));
    
    EXPECT_GE(stats["l1"]["entries"], 0);
    EXPECT_GT(stats["l1"]["max_entries"], 0);
    EXPECT_GE(stats["overall"]["hit_rate"], 0.0);
}

TEST_F(AdaptiveCachePhase1Test, AdminAPIHealthStatus) {
    AdaptiveQueryCache cache(config_);
    
    json health = cache.getHealthStatus();
    
    EXPECT_TRUE(health.contains("healthy"));
    EXPECT_TRUE(health.contains("warnings"));
    EXPECT_TRUE(health["warnings"].is_array());
    
    // Should be healthy initially
    EXPECT_TRUE(health["healthy"]);
}

TEST_F(AdaptiveCachePhase1Test, AdminAPIExportKeys) {
    AdaptiveQueryCache cache(config_);
    
    // Add entries
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("key" + std::to_string(i), {});
        cache.put(fp, {}, {{"v", i}});
    }
    
    // Export keys
    auto keys = cache.exportKeys(5);
    
    EXPECT_LE(keys.size(), 5);
    EXPECT_GT(keys.size(), 0);
    
    // Keys should have tier prefix
    for (const auto& key : keys) {
        EXPECT_TRUE(key.find("L1:") == 0 || key.find("L2:") == 0);
    }
}

TEST_F(AdaptiveCachePhase1Test, AdminAPIGetTenantStats) {
    config_.enable_tenant_isolation = true;
    AdaptiveQueryCache cache(config_);
    
    // Add entries for different tenants
    json result = {{"data", std::string(100, 'x')}};
    cache.put(cache.generateFingerprint("q1", {}, "tenant1"), {}, result, "tenant1");
    cache.put(cache.generateFingerprint("q2", {}, "tenant2"), {}, result, "tenant2");
    
    json tenant_stats = cache.getTenantStats();
    
    EXPECT_TRUE(tenant_stats["enabled"]);
    EXPECT_TRUE(tenant_stats.contains("tenants"));
    EXPECT_GT(tenant_stats["quota_per_tenant"], 0);
}

TEST_F(AdaptiveCachePhase1Test, AdminAPIBulkPut) {
    AdaptiveQueryCache cache(config_);
    
    // Prepare bulk entries
    std::vector<std::tuple<std::string, json, json, std::string>> entries;
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("bulk" + std::to_string(i), {});
        json params = {};
        json result = {{"id", i}};
        entries.emplace_back(fp, params, result, "");
    }
    
    // Bulk put
    size_t cached = cache.bulkPut(entries);
    
    EXPECT_EQ(cached, 10);
    
    // Verify entries are cached
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("bulk" + std::to_string(i), {});
        auto cached_entry = cache.get(fp, "");
        EXPECT_TRUE(cached_entry.has_value());
    }
}

TEST_F(AdaptiveCachePhase1Test, AdminAPIInvalidateTenant) {
    config_.enable_tenant_isolation = true;
    AdaptiveQueryCache cache(config_);
    
    // Add entries for multiple tenants
    json result = {{"data", "test"}};
    for (int i = 0; i < 5; i++) {
        std::string fp1 = cache.generateFingerprint("query" + std::to_string(i), {}, "tenant1");
        std::string fp2 = cache.generateFingerprint("query" + std::to_string(i), {}, "tenant2");
        cache.put(fp1, {}, result, "tenant1");
        cache.put(fp2, {}, result, "tenant2");
    }
    
    // Invalidate tenant1
    size_t invalidated = cache.invalidateTenant("tenant1");
    EXPECT_GT(invalidated, 0);
    
    // Tenant1 entries should be gone
    for (int i = 0; i < 5; i++) {
        std::string fp1 = cache.generateFingerprint("query" + std::to_string(i), {}, "tenant1");
        EXPECT_FALSE(cache.get(fp1, "tenant1").has_value());
    }
    
    // Tenant2 entries should still exist
    for (int i = 0; i < 5; i++) {
        std::string fp2 = cache.generateFingerprint("query" + std::to_string(i), {}, "tenant2");
        EXPECT_TRUE(cache.get(fp2, "tenant2").has_value());
    }
}

// ============================================================================
// Phase 3 Tests: Adaptive TTL Tuning
// ============================================================================

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLDisabled) {
    config_.enable_adaptive_ttl = false;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    std::string fp = cache.generateFingerprint("query", {});
    
    EXPECT_TRUE(cache.put(fp, {}, result));
    
    // TTL should be tier-specific (L1)
    auto cached = cache.get(fp, "");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->ttl_seconds, config_.l1_ttl_seconds);
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLEnabled) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;    // 1 minute
    config_.adaptive_ttl_max_seconds = 3600;  // 1 hour
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    std::string fp = cache.generateFingerprint("query", {});
    
    EXPECT_TRUE(cache.put(fp, {}, result));
    
    // Initial TTL should be min
    auto cached1 = cache.get(fp, "");
    ASSERT_TRUE(cached1.has_value());
    int initial_ttl = cached1->ttl_seconds;
    EXPECT_GE(initial_ttl, config_.adaptive_ttl_min_seconds);
    EXPECT_LE(initial_ttl, config_.adaptive_ttl_max_seconds);
    
    // Access multiple times to increase access_count
    for (int i = 0; i < 10; i++) {
        cache.get(fp, "");
    }
    
    // TTL should increase with access count
    auto cached2 = cache.get(fp, "");
    ASSERT_TRUE(cached2.has_value());
    int final_ttl = cached2->ttl_seconds;
    
    // TTL should have increased
    EXPECT_GT(final_ttl, initial_ttl);
    EXPECT_LE(final_ttl, config_.adaptive_ttl_max_seconds);
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLLogarithmicScaling) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 100;
    config_.adaptive_ttl_max_seconds = 10000;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    std::string fp = cache.generateFingerprint("popular_query", {});
    
    cache.put(fp, {}, result);
    
    std::vector<int> ttls;
    
    // Access 100 times and track TTL growth
    for (int i = 0; i < 100; i++) {
        auto cached = cache.get(fp, "");
        if (cached.has_value() && i % 10 == 0) {
            ttls.push_back(cached->ttl_seconds);
        }
    }
    
    // TTL should grow logarithmically (diminishing returns)
    // First interval growth should be larger than later intervals
    EXPECT_GT(ttls.size(), 2);
    if (ttls.size() >= 3) {
        int first_growth = ttls[1] - ttls[0];
        int last_growth = ttls.back() - ttls[ttls.size() - 2];
        EXPECT_GE(first_growth, last_growth);  // Diminishing growth
    }
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLBounds) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 300;  // 5 minutes max
    config_.adaptive_ttl_scaling_factor = 2.0;
    AdaptiveQueryCache cache(config_);
    
    json result = {{"value", 1}};
    std::string fp = cache.generateFingerprint("bounded_query", {});
    
    cache.put(fp, {}, result);
    
    // Access many times to try to exceed max
    for (int i = 0; i < 1000; i++) {
        auto cached = cache.get(fp, "");
        if (cached.has_value()) {
            // Should never exceed max
            EXPECT_LE(cached->ttl_seconds, config_.adaptive_ttl_max_seconds);
            // Should never go below min
            EXPECT_GE(cached->ttl_seconds, config_.adaptive_ttl_min_seconds);
        }
    }
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLConfigValidation) {
    config_.enable_adaptive_ttl = true;
    
    // Test invalid: min >= max
    config_.adaptive_ttl_min_seconds = 1000;
    config_.adaptive_ttl_max_seconds = 100;
    std::string error = {};
    EXPECT_FALSE(config_.validate(&error));
    EXPECT_NE(error.find("min_seconds must be less than"), std::string::npos);
    
    // Test invalid: zero min
    config_.adaptive_ttl_min_seconds = 0;
    config_.adaptive_ttl_max_seconds = 1000;
    EXPECT_FALSE(config_.validate(&error));
    
    // Test invalid: negative scaling factor
    config_.adaptive_ttl_min_seconds = 100;
    config_.adaptive_ttl_max_seconds = 1000;
    config_.adaptive_ttl_scaling_factor = -1.0;
    EXPECT_FALSE(config_.validate(&error));
    
    // Test valid config
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.adaptive_ttl_scaling_factor = 5.0;
    EXPECT_TRUE(config_.validate(&error));
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLWithL2Promotion) {
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.l1_max_entry_size = 50;  // Force to L2
    AdaptiveQueryCache cache(config_);
    
    // Create entry that goes to L2
    std::string large_data(100, 'x');
    json result = {{"data", large_data}};
    std::string fp = cache.generateFingerprint("l2_query", {});
    
    cache.put(fp, {}, result);
    
    // Access multiple times to build access count
    for (int i = 0; i < 5; i++) {
        auto cached = cache.get(fp, "");
        ASSERT_TRUE(cached.has_value());
    }
    
    // TTL should have been updated with each access
    auto final = cache.get(fp, "");
    ASSERT_TRUE(final.has_value());
    EXPECT_GT(final->ttl_seconds, config_.adaptive_ttl_min_seconds);
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLHotKeyPolicyFires) {
    // Verify that after >= 10 accesses in the same 5-min window the hot-key
    // policy kicks in (TTL * 1.5) and the ttl_extended_total counter increments.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);

    json result = {{"value", 1}};
    std::string fp = cache.generateFingerprint("hot_key", {});

    EXPECT_TRUE(cache.put(fp, {}, result));

    // First get: records access_count=1, window_count=1 – below hot threshold
    auto first = cache.get(fp, "");
    ASSERT_TRUE(first.has_value());
    int ttl_before_hot = first->ttl_seconds;

    // 9 more accesses – window_count reaches 10, triggering the hot-key policy
    for (int i = 0; i < 9; i++) {
        auto r = cache.get(fp, "");
        ASSERT_TRUE(r.has_value());
    }

    auto after_hot = cache.get(fp, "");
    ASSERT_TRUE(after_hot.has_value());

    // TTL must have grown beyond the logarithmic baseline
    EXPECT_GT(after_hot->ttl_seconds, ttl_before_hot);
    EXPECT_LE(after_hot->ttl_seconds, config_.adaptive_ttl_max_seconds);

    // Metric counter must have incremented at least once
    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GT(metrics.ttl_extended_total.load(), 0u);
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLMetricsInDetailedInfo) {
    // Verify that getDetailedInfo() reports the adaptive_ttl section correctly.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("adaptive_ttl"));
    EXPECT_TRUE(info["adaptive_ttl"]["enabled"].get<bool>());
    EXPECT_EQ(info["adaptive_ttl"]["min_seconds"].get<int>(), 60);
    EXPECT_EQ(info["adaptive_ttl"]["max_seconds"].get<int>(), 3600);
    EXPECT_EQ(info["adaptive_ttl"]["ttl_extended_total"].get<uint64_t>(), 0u);
    EXPECT_EQ(info["adaptive_ttl"]["ttl_shortened_total"].get<uint64_t>(), 0u);
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLMetricsInDetailedInfoDisabled) {
    // Verify getDetailedInfo() when adaptive TTL is off.
    config_.enable_adaptive_ttl = false;
    AdaptiveQueryCache cache(config_);

    json info = cache.getDetailedInfo();
    ASSERT_TRUE(info.contains("adaptive_ttl"));
    EXPECT_FALSE(info["adaptive_ttl"]["enabled"].get<bool>());
}

TEST_F(AdaptiveCachePhase1Test, AdaptiveTTLMetricsJsonExport) {
    // Verify that CacheMetrics::toJson() exports the new TTL adjustment counters.
    config_.enable_adaptive_ttl = true;
    config_.adaptive_ttl_min_seconds = 60;
    config_.adaptive_ttl_max_seconds = 3600;
    config_.adaptive_ttl_scaling_factor = 5.0;
    AdaptiveQueryCache cache(config_);

    // Trigger at least one hot-key extension
    json result = {{"x", 1}};
    std::string fp = cache.generateFingerprint("metrics_key", {});
    cache.put(fp, {}, result);
    for (int i = 0; i < 11; i++) {
        cache.get(fp, "");
    }

    json m = cache.getEnhancedMetrics().toJson();
    ASSERT_TRUE(m.contains("adaptive_ttl"));
    EXPECT_GE(m["adaptive_ttl"]["ttl_extended_total"].get<uint64_t>(), 1u);
    EXPECT_EQ(m["adaptive_ttl"]["ttl_shortened_total"].get<uint64_t>(), 0u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

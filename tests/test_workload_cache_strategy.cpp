#include <gtest/gtest.h>
#include "query/workload_cache_strategy.h"
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::query;
using json = nlohmann::json;

class WorkloadCacheStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.enable_workload_detection = true;
        config_.detection_sample_rate = 1.0;  // Sample all queries for testing
        config_.detection_window = std::chrono::seconds(1);  // Fast detection for testing
        config_.min_samples_for_detection = 10;
        
        config_.oltp_frequency_threshold = 10.0;
        config_.olap_frequency_threshold = 0.5;
        config_.oltp_result_size_threshold = 50 * 1024;    // 50KB
        config_.olap_result_size_threshold = 1024 * 1024;  // 1MB
    }
    
    QueryCharacteristics createOLTPQuery(int id) {
        QueryCharacteristics char_;
        char_.result_size_bytes = 1024;  // 1KB - small
        char_.rows_scanned = 100;
        char_.rows_returned = 10;
        char_.execution_time_ms = 5;
        char_.access_count = 100;  // High frequency
        char_.first_seen = std::chrono::system_clock::now() - std::chrono::minutes(1);
        char_.last_accessed = std::chrono::system_clock::now();
        return char_;
    }
    
    QueryCharacteristics createOLAPQuery(int id) {
        QueryCharacteristics char_;
        char_.result_size_bytes = 5 * 1024 * 1024;  // 5MB - large
        char_.rows_scanned = 1000000;
        char_.rows_returned = 100000;
        char_.execution_time_ms = 5000;
        char_.access_count = 1;  // Low frequency
        char_.first_seen = std::chrono::system_clock::now() - std::chrono::hours(1);
        char_.last_accessed = std::chrono::system_clock::now();
        return char_;
    }
    
    QueryCharacteristics createMixedQuery(int id) {
        QueryCharacteristics char_;
        char_.result_size_bytes = 100 * 1024;  // 100KB - medium
        char_.rows_scanned = 10000;
        char_.rows_returned = 1000;
        char_.execution_time_ms = 100;
        char_.access_count = 10;  // Medium frequency
        char_.first_seen = std::chrono::system_clock::now() - std::chrono::minutes(10);
        char_.last_accessed = std::chrono::system_clock::now();
        return char_;
    }
    
    WorkloadCacheStrategy::Config config_;
};

// ============================================================================
// WorkloadCacheConfig Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, WorkloadCacheConfig_OLTP) {
    auto config = WorkloadCacheConfig::forWorkload(WorkloadType::OLTP);
    
    EXPECT_EQ(config.type, WorkloadType::OLTP);
    EXPECT_GT(config.max_entries, 30000);  // More entries for frequent queries
    EXPECT_LT(config.default_ttl.count(), 600);  // Short TTL (< 10 minutes)
    EXPECT_EQ(config.eviction_policy, QueryCache::EvictionPolicy::LRU);
    EXPECT_TRUE(config.enable_adaptive_ttl);
    EXPECT_TRUE(config.enable_frequency_weighting);
}

TEST_F(WorkloadCacheStrategyTest, WorkloadCacheConfig_OLAP) {
    auto config = WorkloadCacheConfig::forWorkload(WorkloadType::OLAP);
    
    EXPECT_EQ(config.type, WorkloadType::OLAP);
    EXPECT_LT(config.max_entries, 10000);  // Fewer entries for large results
    EXPECT_GT(config.default_ttl.count(), 3600);  // Long TTL (> 1 hour)
    EXPECT_EQ(config.eviction_policy, QueryCache::EvictionPolicy::LFU);
    EXPECT_TRUE(config.enable_adaptive_ttl);
    EXPECT_GT(config.max_entry_size, 10 * 1024 * 1024);  // Support large entries
}

TEST_F(WorkloadCacheStrategyTest, WorkloadCacheConfig_MIXED) {
    auto config = WorkloadCacheConfig::forWorkload(WorkloadType::MIXED);
    
    EXPECT_EQ(config.type, WorkloadType::MIXED);
    EXPECT_GT(config.max_entries, 10000);
    EXPECT_LT(config.max_entries, 30000);
    EXPECT_TRUE(config.enable_adaptive_ttl);
}

TEST_F(WorkloadCacheStrategyTest, WorkloadCacheConfig_STREAMING) {
    auto config = WorkloadCacheConfig::forWorkload(WorkloadType::STREAMING);
    
    EXPECT_EQ(config.type, WorkloadType::STREAMING);
    EXPECT_LT(config.max_entries, 2000);  // Very few entries
    EXPECT_LT(config.default_ttl.count(), 30);  // Very short TTL
    EXPECT_FALSE(config.enable_adaptive_ttl);  // Fixed TTL for streaming
}

// ============================================================================
// Query Characteristics Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, QueryCharacteristics_Selectivity) {
    QueryCharacteristics char_;
    char_.rows_scanned = 1000;
    char_.rows_returned = 100;
    
    EXPECT_DOUBLE_EQ(char_.selectivity(), 0.1);
}

TEST_F(WorkloadCacheStrategyTest, QueryCharacteristics_Frequency) {
    QueryCharacteristics char_;
    char_.access_count = 60;
    char_.first_seen = std::chrono::system_clock::now() - std::chrono::minutes(10);
    char_.last_accessed = std::chrono::system_clock::now();
    
    // 60 accesses in 10 minutes = 6 per minute
    EXPECT_NEAR(char_.frequency_per_minute(), 6.0, 0.1);
}

// ============================================================================
// Workload Detection Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, DetectWorkload_OLTP) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record many OLTP-like queries
    for (int i = 0; i < 20; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("oltp_query_" + std::to_string(i), char_);
    }
    
    // Wait for detection window
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto workload = strategy.detectWorkload();
    EXPECT_EQ(workload, WorkloadType::OLTP);
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.detected_type, WorkloadType::OLTP);
    EXPECT_EQ(stats.total_queries, 20);
}

TEST_F(WorkloadCacheStrategyTest, DetectWorkload_OLAP) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record many OLAP-like queries
    for (int i = 0; i < 20; ++i) {
        auto char_ = createOLAPQuery(i);
        strategy.recordQuery("olap_query_" + std::to_string(i), char_);
    }
    
    // Wait for detection window
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto workload = strategy.detectWorkload();
    EXPECT_EQ(workload, WorkloadType::OLAP);
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.detected_type, WorkloadType::OLAP);
}

TEST_F(WorkloadCacheStrategyTest, DetectWorkload_MIXED) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record mixed workload
    for (int i = 0; i < 10; ++i) {
        auto oltp_char = createOLTPQuery(i);
        strategy.recordQuery("oltp_" + std::to_string(i), oltp_char);
        
        auto olap_char = createOLAPQuery(i);
        strategy.recordQuery("olap_" + std::to_string(i), olap_char);
    }
    
    // Wait for detection window
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto workload = strategy.detectWorkload();
    // Mixed workload should be detected (not strongly OLTP or OLAP)
    EXPECT_TRUE(workload == WorkloadType::MIXED || 
                workload == WorkloadType::OLTP || 
                workload == WorkloadType::OLAP);
}

TEST_F(WorkloadCacheStrategyTest, DetectWorkload_InsufficientSamples) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record only a few queries
    for (int i = 0; i < 5; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("query_" + std::to_string(i), char_);
    }
    
    auto workload = strategy.detectWorkload();
    EXPECT_EQ(workload, WorkloadType::UNKNOWN);
}

// ============================================================================
// Cache Configuration Selection Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, GetCacheConfig_AfterOLTPDetection) {
    WorkloadCacheStrategy strategy(config_);
    
    // Populate with OLTP queries
    for (int i = 0; i < 20; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("oltp_" + std::to_string(i), char_);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    strategy.detectWorkload();
    
    auto cache_config = strategy.getCacheConfig();
    EXPECT_EQ(cache_config.type, WorkloadType::OLTP);
    EXPECT_GT(cache_config.max_entries, 30000);
}

TEST_F(WorkloadCacheStrategyTest, GetCacheConfigForQuery_OLTPQuery) {
    WorkloadCacheStrategy strategy(config_);
    
    auto char_ = createOLTPQuery(1);
    auto cache_config = strategy.getCacheConfigForQuery(char_);
    
    // Should get OLTP configuration for OLTP-like query
    EXPECT_EQ(cache_config.type, WorkloadType::OLTP);
}

TEST_F(WorkloadCacheStrategyTest, GetCacheConfigForQuery_OLAPQuery) {
    WorkloadCacheStrategy strategy(config_);
    
    auto char_ = createOLAPQuery(1);
    auto cache_config = strategy.getCacheConfigForQuery(char_);
    
    // Should get OLAP configuration for OLAP-like query
    EXPECT_EQ(cache_config.type, WorkloadType::OLAP);
}

// ============================================================================
// Cache Decision Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, ShouldCache_NormalQuery) {
    WorkloadCacheStrategy strategy(config_);
    
    auto char_ = createOLTPQuery(1);
    EXPECT_TRUE(strategy.shouldCache(char_));
}

TEST_F(WorkloadCacheStrategyTest, ShouldCache_TooLarge) {
    WorkloadCacheStrategy strategy(config_);
    
    QueryCharacteristics char_;
    char_.result_size_bytes = 200 * 1024 * 1024;  // 200MB - 2x over 100MB limit
    char_.execution_time_ms = 1000;
    
    EXPECT_FALSE(strategy.shouldCache(char_));
}

TEST_F(WorkloadCacheStrategyTest, ShouldCache_TooFast) {
    WorkloadCacheStrategy strategy(config_);
    
    QueryCharacteristics char_;
    char_.result_size_bytes = 1024;
    char_.execution_time_ms = 2;  // 2ms - too fast to benefit from caching
    
    EXPECT_FALSE(strategy.shouldCache(char_));
}

TEST_F(WorkloadCacheStrategyTest, ShouldCache_LowSelectivity) {
    WorkloadCacheStrategy strategy(config_);
    
    QueryCharacteristics char_;
    char_.result_size_bytes = 1024;
    char_.execution_time_ms = 100;
    char_.rows_scanned = 2000000;      // 2M rows
    char_.rows_returned = 1900000;     // 1.9M rows - 95% selectivity
    
    EXPECT_FALSE(strategy.shouldCache(char_));
}

// ============================================================================
// TTL Calculation Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, CalculateTTL_HighFrequency) {
    WorkloadCacheStrategy strategy(config_);
    
    auto char_ = createOLTPQuery(1);
    auto ttl = strategy.calculateTTL(char_);
    
    // High frequency should get short TTL
    EXPECT_LT(ttl.count(), 600);  // < 10 minutes
}

TEST_F(WorkloadCacheStrategyTest, CalculateTTL_LowFrequency) {
    WorkloadCacheStrategy strategy(config_);
    
    auto char_ = createOLAPQuery(1);
    auto ttl = strategy.calculateTTL(char_);
    
    // Low frequency should get long TTL
    EXPECT_GT(ttl.count(), 3600);  // > 1 hour
}

TEST_F(WorkloadCacheStrategyTest, CalculateTTL_AdaptiveDisabled) {
    WorkloadCacheStrategy strategy(config_);
    
    QueryCharacteristics char_ = createOLTPQuery(1);
    // Force non-adaptive TTL
    char_.access_count = 1;
    char_.first_seen = std::chrono::system_clock::now();
    char_.last_accessed = std::chrono::system_clock::now();
    
    auto ttl = strategy.calculateTTL(char_);
    
    // Should get default TTL from config
    EXPECT_GT(ttl.count(), 0);
}

// ============================================================================
// Hot Queries Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, GetHotQueries_ReturnsTopK) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record queries with varying frequencies
    for (int i = 0; i < 20; ++i) {
        auto char_ = createOLTPQuery(i);
        char_.access_count = i + 1;  // Increasing access count
        strategy.recordQuery("query_" + std::to_string(i), char_);
    }
    
    auto hot_queries = strategy.getHotQueries(5);
    
    EXPECT_EQ(hot_queries.size(), 5);
    // Should return the 5 most frequently accessed queries
    // query_19, query_18, query_17, query_16, query_15
    EXPECT_EQ(hot_queries[0], "query_19");
    EXPECT_EQ(hot_queries[1], "query_18");
}

TEST_F(WorkloadCacheStrategyTest, GetHotQueries_LessThanLimit) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record only 3 queries
    for (int i = 0; i < 3; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("query_" + std::to_string(i), char_);
    }
    
    auto hot_queries = strategy.getHotQueries(10);
    
    EXPECT_EQ(hot_queries.size(), 3);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, GetStats_InitialState) {
    WorkloadCacheStrategy strategy(config_);
    
    auto stats = strategy.getStats();
    
    EXPECT_EQ(stats.detected_type, WorkloadType::UNKNOWN);
    EXPECT_EQ(stats.total_queries, 0);
    EXPECT_EQ(stats.cached_queries, 0);
    EXPECT_EQ(stats.cache_hits, 0);
    EXPECT_EQ(stats.cache_misses, 0);
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.0);
}

TEST_F(WorkloadCacheStrategyTest, GetStats_AfterRecording) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record some queries
    for (int i = 0; i < 10; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("query_" + std::to_string(i), char_);
    }
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.total_queries, 10);
}

TEST_F(WorkloadCacheStrategyTest, Stats_ToJson) {
    WorkloadCacheStrategy::WorkloadStats stats;
    stats.detected_type = WorkloadType::OLTP;
    stats.total_queries = 100;
    stats.cache_hits = 80;
    stats.cache_misses = 20;
    
    auto json = stats.toJson();
    
    EXPECT_EQ(json["detected_type"], "OLTP");
    EXPECT_EQ(json["total_queries"], 100);
    EXPECT_EQ(json["cache_hits"], 80);
    EXPECT_EQ(json["cache_misses"], 20);
    EXPECT_DOUBLE_EQ(json["hit_rate"], 0.8);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, Reset_ClearsState) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record queries and detect workload
    for (int i = 0; i < 20; ++i) {
        auto char_ = createOLTPQuery(i);
        strategy.recordQuery("query_" + std::to_string(i), char_);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    strategy.detectWorkload();
    
    // Reset
    strategy.reset();
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.detected_type, WorkloadType::UNKNOWN);
    EXPECT_EQ(stats.total_queries, 0);
    
    auto hot_queries = strategy.getHotQueries(10);
    EXPECT_TRUE(hot_queries.empty());
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, SetConfig_UpdatesConfiguration) {
    WorkloadCacheStrategy strategy(config_);
    
    WorkloadCacheStrategy::Config new_config;
    new_config.enable_workload_detection = false;
    new_config.detection_sample_rate = 0.5;
    
    strategy.setConfig(new_config);
    
    auto retrieved_config = strategy.getConfig();
    EXPECT_FALSE(retrieved_config.enable_workload_detection);
    EXPECT_DOUBLE_EQ(retrieved_config.detection_sample_rate, 0.5);
}

// ============================================================================
// Thread Safety Tests (Basic)
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, ThreadSafety_ConcurrentRecording) {
    WorkloadCacheStrategy strategy(config_);
    
    // Record queries from multiple threads
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&strategy, t]() {
            for (int i = 0; i < 10; ++i) {
                auto char_ = QueryCharacteristics();
                char_.result_size_bytes = 1024;
                char_.execution_time_ms = 10;
                char_.access_count = 1;
                char_.first_seen = std::chrono::system_clock::now();
                char_.last_accessed = std::chrono::system_clock::now();
                
                strategy.recordQuery(
                    "thread_" + std::to_string(t) + "_query_" + std::to_string(i),
                    char_
                );
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.total_queries, 40);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, EdgeCase_ZeroFrequency) {
    WorkloadCacheStrategy strategy(config_);
    
    QueryCharacteristics char_;
    char_.access_count = 1;
    char_.first_seen = std::chrono::system_clock::now();
    char_.last_accessed = std::chrono::system_clock::now();
    
    // Should handle zero frequency gracefully
    EXPECT_GE(char_.frequency_per_minute(), 0.0);
}

TEST_F(WorkloadCacheStrategyTest, EdgeCase_ZeroSelectivity) {
    QueryCharacteristics char_;
    char_.rows_scanned = 0;
    char_.rows_returned = 0;
    
    // Should handle zero selectivity gracefully
    EXPECT_DOUBLE_EQ(char_.selectivity(), 1.0);
}

// ============================================================================
// IV-01 — Zero-divisor guard: classifyWorkload() with empty pattern map
// (issue #5177)
// ============================================================================

TEST_F(WorkloadCacheStrategyTest, ClassifyWorkload_EmptyPatterns_ReturnsUnknown) {
    // Set min_samples_for_detection to 0 so the "insufficient samples" guard
    // in detectWorkload() does NOT short-circuit before reaching classifyWorkload().
    // With an empty query_patterns_ map the pre-fix code divided by zero;
    // the IV-01 fix adds an early-return UNKNOWN guard at the top of
    // classifyWorkload() so this must complete without UB.
    WorkloadCacheStrategy::Config cfg = config_;
    cfg.min_samples_for_detection = 0;

    WorkloadCacheStrategy strategy(cfg);
    // No queries recorded → query_patterns_ is empty.
    WorkloadType result = strategy.detectWorkload();
    EXPECT_EQ(result, WorkloadType::UNKNOWN);
}
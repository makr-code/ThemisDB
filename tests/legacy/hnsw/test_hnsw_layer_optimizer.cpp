// Unit tests for HNSW Layer Optimizer

#include <gtest/gtest.h>
#include "index/hnsw_layer_optimizer.h"
#include <chrono>
#include <thread>

using namespace themis;

// Test basic configuration
TEST(HnswLayerOptimizerTest, BasicConfiguration) {
    HnswOptimizationConfig config;
    config.enabled = true;
    config.layer_pruning.enabled = true;
    config.layer_pruning.threshold_multiplier = 5.0;
    
    HnswLayerOptimizer optimizer(config);
    
    EXPECT_TRUE(optimizer.isEnabled());
    EXPECT_EQ(config.enabled, optimizer.getConfig().enabled);
    EXPECT_EQ(config.layer_pruning.enabled, optimizer.getConfig().layer_pruning.enabled);
    EXPECT_DOUBLE_EQ(5.0, optimizer.getConfig().layer_pruning.threshold_multiplier);
}

// Test layer pruning logic
TEST(HnswLayerOptimizerTest, LayerPruning) {
    HnswOptimizationConfig config;
    config.enabled = true;
    config.layer_pruning.enabled = true;
    config.layer_pruning.threshold_multiplier = 5.0;
    
    HnswLayerOptimizer optimizer(config);
    
    // Should prune when candidates > k * threshold_multiplier
    EXPECT_TRUE(optimizer.shouldPruneLayer(3, 60, 10));  // 60 > 10*5
    EXPECT_FALSE(optimizer.shouldPruneLayer(3, 40, 10)); // 40 < 10*5
    EXPECT_FALSE(optimizer.shouldPruneLayer(3, 50, 10)); // 50 = 10*5 (not strictly greater)
}

// Test layer statistics recording
TEST(HnswLayerOptimizerTest, LayerStatisticsRecording) {
    HnswOptimizationConfig config;
    config.enabled = true;
    
    HnswLayerOptimizer optimizer(config);
    
    // Record some layer accesses
    optimizer.recordLayerAccess(0, 100, 1.0);
    optimizer.recordLayerAccess(0, 120, 1.5);
    optimizer.recordLayerAccess(1, 50, 0.5);
    
    auto stats = optimizer.getLayerStats();
    
    EXPECT_EQ(2, stats.size());  // Two layers
    EXPECT_EQ(2, stats[0].access_count);
    EXPECT_EQ(220, stats[0].candidates_found);
    EXPECT_DOUBLE_EQ(1.25, stats[0].avg_search_time_ms);  // (1.0 + 1.5) / 2
}

// Test query statistics recording
TEST(HnswLayerOptimizerTest, QueryStatisticsRecording) {
    HnswOptimizationConfig config;
    config.enabled = true;
    config.adaptive_layer_selection.enabled = true;
    config.adaptive_layer_selection.stats_window_size = 10;
    
    HnswLayerOptimizer optimizer(config);
    
    // Record some queries
    for (int i = 0; i < 15; i++) {
        optimizer.recordQueryStats(5, 64, 5, 10, 2.0 + i * 0.1);
    }
    
    auto recent_queries = optimizer.getRecentQueryStats();
    
    // Should only keep last 10 queries (window size)
    EXPECT_EQ(10, recent_queries.size());
    EXPECT_EQ(5, recent_queries[0].entry_layer);
    EXPECT_EQ(64, recent_queries[0].ef_used);
}

// Test adaptive ef selection
TEST(HnswLayerOptimizerTest, AdaptiveEfSelection) {
    HnswOptimizationConfig config;
    config.enabled = true;
    config.adaptive_layer_selection.enabled = true;
    config.adaptive_layer_selection.stats_window_size = 100;
    
    HnswLayerOptimizer optimizer(config);
    
    // Record queries with different ef values
    // ef=64 is faster
    for (int i = 0; i < 20; i++) {
        optimizer.recordQueryStats(5, 64, 5, 10, 1.0);
    }
    // ef=128 is slower
    for (int i = 0; i < 20; i++) {
        optimizer.recordQueryStats(5, 128, 5, 10, 2.0);
    }
    
    // Should recommend ef=64 as it's faster
    int optimal_ef = optimizer.getOptimalEf(10);
    EXPECT_EQ(64, optimal_ef);
}

// Test disabled optimizer
TEST(HnswLayerOptimizerTest, DisabledOptimizer) {
    HnswOptimizationConfig config;
    config.enabled = false;
    
    HnswLayerOptimizer optimizer(config);
    
    EXPECT_FALSE(optimizer.isEnabled());
    EXPECT_FALSE(optimizer.shouldPruneLayer(3, 100, 10));  // Should not prune when disabled
    EXPECT_EQ(-1, optimizer.getOptimalEf(10));  // Should return -1 when disabled
    EXPECT_EQ(-1, optimizer.getOptimalEntryLayer());
}

// Test statistics reset
TEST(HnswLayerOptimizerTest, StatisticsReset) {
    HnswOptimizationConfig config;
    config.enabled = true;
    config.adaptive_layer_selection.enabled = true;
    
    HnswLayerOptimizer optimizer(config);
    
    // Record some data
    optimizer.recordLayerAccess(0, 100, 1.0);
    optimizer.recordQueryStats(5, 64, 5, 10, 2.0);
    
    EXPECT_EQ(1, optimizer.getLayerStats().size());
    EXPECT_EQ(1, optimizer.getRecentQueryStats().size());
    
    // Reset
    optimizer.resetStats();
    
    EXPECT_EQ(0, optimizer.getLayerStats().size());
    EXPECT_EQ(0, optimizer.getRecentQueryStats().size());
}

// Test efficiency score calculation
TEST(HnswLayerOptimizerTest, EfficiencyScoreCalculation) {
    HnswOptimizationConfig config;
    config.enabled = true;
    
    HnswLayerOptimizer optimizer(config);
    
    // Record layer access with known values
    optimizer.recordLayerAccess(0, 100, 2.0);
    
    auto stats = optimizer.getLayerStats();
    auto layer0_stats = stats[0];
    
    // Efficiency = candidates_found / avg_search_time = 100 / (2.0 / 1) = 100 / 2.0 = 50.0
    EXPECT_DOUBLE_EQ(50.0, layer0_stats.efficiency_score);
}



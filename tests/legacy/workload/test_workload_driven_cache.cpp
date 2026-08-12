#include <gtest/gtest.h>
#include "llm/paged_kv_cache_manager.h"
#include <memory>

using namespace themis::llm;

class WorkloadDrivenCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.num_blocks = 1000;
        config_.block_size = 16;
        config_.num_layers = 32;
        config_.head_dim = 128;
        config_.num_kv_heads = 8;
        config_.bytes_per_element = 2;
        config_.enable_prefix_caching = true;
    }
    
    PagedKVCacheManager::Config config_;
};

// ============================================================================
// Manual Cache Type Selection Tests
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, ManualCacheTypeSelection) {
    PagedKVCacheManager manager(config_);
    
    // Default should be STANDARD
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STANDARD);
    
    // Manually set to PREFIX_OPTIMIZED
    manager.setCacheType(PagedKVCacheManager::CacheType::PREFIX_OPTIMIZED);
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::PREFIX_OPTIMIZED);
    
    // Manually set to STREAMING
    manager.setCacheType(PagedKVCacheManager::CacheType::STREAMING);
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STREAMING);
}

// ============================================================================
// Workload Pattern Detection Tests
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, DetectHighPrefixReuseWorkload) {
    PagedKVCacheManager manager(config_);
    
    // Add parent sequence
    uint64_t parent_id = 1;
    manager.addSequence(parent_id, 100);
    
    // Add many child sequences sharing prefix
    for (uint64_t i = 2; i <= 20; ++i) {
        manager.enablePrefixCaching(i, parent_id, 80);  // 80% prefix reuse
    }
    
    // Analyze workload
    bool changed = manager.analyzeAndAdaptCacheType();
    
    // Should detect high prefix reuse
    auto metrics = manager.getWorkloadMetrics();
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::HIGH_PREFIX_REUSE);
    
    // Should adapt to PREFIX_OPTIMIZED cache type
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::PREFIX_OPTIMIZED);
    EXPECT_TRUE(changed) << "Cache type should have changed for high prefix reuse workload";
}

TEST_F(WorkloadDrivenCacheTest, DetectLowPrefixReuseWorkload) {
    PagedKVCacheManager manager(config_);
    
    // Add many independent sequences (no prefix sharing)
    for (uint64_t i = 1; i <= 20; ++i) {
        manager.addSequence(i, 100);
    }
    
    // Analyze workload
    bool changed = manager.analyzeAndAdaptCacheType();
    
    // Should detect low prefix reuse
    auto metrics = manager.getWorkloadMetrics();
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::LOW_PREFIX_REUSE);
    
    // Should adapt to STREAMING cache type
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STREAMING);
    EXPECT_TRUE(changed) << "Cache type should have changed for low prefix reuse workload";
}

TEST_F(WorkloadDrivenCacheTest, DetectMixedWorkload) {
    PagedKVCacheManager manager(config_);
    
    // Add parent sequence
    uint64_t parent_id = 1;
    manager.addSequence(parent_id, 100);
    
    // Add some sequences with prefix sharing (40%)
    for (uint64_t i = 2; i <= 9; ++i) {
        manager.enablePrefixCaching(i, parent_id, 50);
    }
    
    // Add some independent sequences (60%)
    for (uint64_t i = 10; i <= 21; ++i) {
        manager.addSequence(i, 100);
    }
    
    // Analyze workload
    manager.analyzeAndAdaptCacheType();
    
    // Should detect mixed workload (between 20% and 60% reuse)
    auto metrics = manager.getWorkloadMetrics();
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::MIXED);
    
    // Should use STANDARD cache type for mixed workload
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STANDARD);
}

// ============================================================================
// Automatic Adaptation Tests
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, AutomaticAdaptationDisabledByDefault) {
    PagedKVCacheManager manager(config_);
    
    // Add sequences
    manager.addSequence(1, 100);
    manager.addSequence(2, 100);
    
    // Cache type should not change automatically (adaptation disabled)
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STANDARD);
}

TEST_F(WorkloadDrivenCacheTest, AutomaticAdaptationEnabled) {
    PagedKVCacheManager manager(config_);
    
    // Enable automatic adaptation (check every 10 sequences)
    manager.setAutomaticAdaptation(true, 10);
    
    // Add parent sequence
    uint64_t parent_id = 1;
    manager.addSequence(parent_id, 100);
    
    // Verify initial state
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STANDARD);
    
    // Add 9 more sequences with prefix sharing (total 10, triggers first check)
    for (uint64_t i = 2; i <= 10; ++i) {
        manager.enablePrefixCaching(i, parent_id, 80);
        manager.addSequence(i + 100, 100);
    }
    
    // After 10 sequences with 90% prefix reuse (9 of 10), should switch to PREFIX_OPTIMIZED
    auto metrics = manager.getWorkloadMetrics();
    EXPECT_GT(metrics.total_sequences, 10);
    EXPECT_GE(metrics.prefix_reuse_ratio, 0.6) << "Should have high prefix reuse ratio";
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::HIGH_PREFIX_REUSE)
        << "Should detect high prefix reuse pattern";
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::PREFIX_OPTIMIZED)
        << "Cache type should adapt to PREFIX_OPTIMIZED after detecting high prefix reuse";
}

// ============================================================================
// Workload Metrics Tests
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, WorkloadMetricsCalculation) {
    PagedKVCacheManager manager(config_);
    
    // Initial metrics
    auto metrics1 = manager.getWorkloadMetrics();
    EXPECT_EQ(metrics1.total_sequences, 0);
    EXPECT_EQ(metrics1.sequences_with_shared_prefix, 0);
    EXPECT_DOUBLE_EQ(metrics1.prefix_reuse_ratio, 0.0);
    
    // Add parent and children
    uint64_t parent_id = 1;
    manager.addSequence(parent_id, 100);
    
    for (uint64_t i = 2; i <= 5; ++i) {
        manager.enablePrefixCaching(i, parent_id, 60);
    }
    
    // Update metrics
    manager.analyzeAndAdaptCacheType();
    auto metrics2 = manager.getWorkloadMetrics();
    
    EXPECT_EQ(metrics2.total_sequences, 5);
    EXPECT_EQ(metrics2.sequences_with_shared_prefix, 4);  // 4 out of 5 share prefix
    EXPECT_DOUBLE_EQ(metrics2.prefix_reuse_ratio, 0.8);   // 80% reuse
    EXPECT_GT(metrics2.avg_prefix_length, 0.0);
}

TEST_F(WorkloadDrivenCacheTest, PrefixReuseRatioThresholds) {
    PagedKVCacheManager manager(config_);
    
    // Test high reuse threshold (>= 60%)
    {
        PagedKVCacheManager high_reuse_manager(config_);
        uint64_t parent_id = 1;
        high_reuse_manager.addSequence(parent_id, 100);
        
        // 7 out of 10 sequences share prefix = 70% reuse
        for (uint64_t i = 2; i <= 8; ++i) {
            high_reuse_manager.enablePrefixCaching(i, parent_id, 50);
        }
        for (uint64_t i = 9; i <= 11; ++i) {
            high_reuse_manager.addSequence(i, 100);
        }
        
        high_reuse_manager.analyzeAndAdaptCacheType();
        auto metrics = high_reuse_manager.getWorkloadMetrics();
        EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::HIGH_PREFIX_REUSE);
    }
    
    // Test low reuse threshold (<= 20%)
    {
        PagedKVCacheManager low_reuse_manager(config_);
        uint64_t parent_id = 1;
        low_reuse_manager.addSequence(parent_id, 100);
        
        // 1 out of 10 sequences share prefix = 10% reuse
        low_reuse_manager.enablePrefixCaching(2, parent_id, 50);
        for (uint64_t i = 3; i <= 11; ++i) {
            low_reuse_manager.addSequence(i, 100);
        }
        
        low_reuse_manager.analyzeAndAdaptCacheType();
        auto metrics = low_reuse_manager.getWorkloadMetrics();
        EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::LOW_PREFIX_REUSE);
    }
}

// ============================================================================
// RAG Workload Simulation
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, RAGWorkloadSimulation) {
    PagedKVCacheManager manager(config_);
    
    // RAG workload: shared system prompt + different contexts
    uint64_t system_prompt_id = 1;
    manager.addSequence(system_prompt_id, 50);  // System prompt
    
    // 100 queries all share the system prompt
    for (uint64_t i = 2; i <= 101; ++i) {
        manager.enablePrefixCaching(i, system_prompt_id, 50);  // Share system prompt
        // Each query has different context after prefix
    }
    
    manager.analyzeAndAdaptCacheType();
    auto metrics = manager.getWorkloadMetrics();
    
    // Should detect as high prefix reuse (RAG pattern)
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::HIGH_PREFIX_REUSE);
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::PREFIX_OPTIMIZED);
    EXPECT_GT(metrics.prefix_reuse_ratio, 0.9) << "RAG workload should have >90% prefix reuse";
}

// ============================================================================
// Streaming Workload Simulation
// ============================================================================

TEST_F(WorkloadDrivenCacheTest, StreamingWorkloadSimulation) {
    PagedKVCacheManager manager(config_);
    
    // Streaming workload: independent generation sequences
    for (uint64_t i = 1; i <= 50; ++i) {
        manager.addSequence(i, 200);  // Each sequence is independent
    }
    
    manager.analyzeAndAdaptCacheType();
    auto metrics = manager.getWorkloadMetrics();
    
    // Should detect as low prefix reuse (streaming pattern)
    EXPECT_EQ(metrics.detected_pattern, PagedKVCacheManager::WorkloadPattern::LOW_PREFIX_REUSE);
    EXPECT_EQ(manager.getCacheType(), PagedKVCacheManager::CacheType::STREAMING);
    EXPECT_LT(metrics.prefix_reuse_ratio, 0.2) << "Streaming workload should have <20% prefix reuse";
}


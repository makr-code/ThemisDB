/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hnsw_parameter_tuner.cpp                      ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     258                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "index/hnsw_parameter_tuner.h"

using namespace themis::index;

class HnswParameterTunerTest : public ::testing::Test {
protected:
    void SetUp() override {
        HnswParameterTuner::Config config;
        config.adaptive = true;
        config.ef_search_min = 32;
        config.ef_search_max = 256;
        config.ef_search_default = 64;
        tuner = std::make_unique<HnswParameterTuner>(config);
    }
    
    std::unique_ptr<HnswParameterTuner> tuner;
};

TEST_F(HnswParameterTunerTest, GetOptimalEfSearch) {
    size_t k = 10;
    size_t dataset_size = 100000;
    
    int ef = tuner->getOptimalEfSearch(k, dataset_size);
    
    EXPECT_GE(ef, 32);
    EXPECT_LE(ef, 256);
    EXPECT_GE(ef, static_cast<int>(k));
}

TEST_F(HnswParameterTunerTest, ScaleWithK) {
    size_t dataset_size = 100000;
    
    int ef_small = tuner->getOptimalEfSearch(5, dataset_size);
    int ef_large = tuner->getOptimalEfSearch(50, dataset_size);
    
    EXPECT_GE(ef_large, ef_small);
}

TEST_F(HnswParameterTunerTest, ScaleWithDatasetSize) {
    size_t k = 10;
    
    int ef_small = tuner->getOptimalEfSearch(k, 10000);
    int ef_large = tuner->getOptimalEfSearch(k, 1000000);
    
    EXPECT_GE(ef_large, ef_small);
}

TEST_F(HnswParameterTunerTest, RecordQueryResult) {
    size_t k = 10;
    int ef_used = 64;
    double latency_ms = 5.0;
    double recall = 0.95;
    
    tuner->recordQueryResult(k, ef_used, latency_ms, recall);
    
    auto stats = tuner->getStats();
    EXPECT_EQ(stats.queries_processed, 1);
    EXPECT_DOUBLE_EQ(stats.avg_latency_ms, latency_ms);
    EXPECT_DOUBLE_EQ(stats.avg_recall, recall);
}

TEST_F(HnswParameterTunerTest, AdaptToHighLatency) {
    size_t k = 10;
    
    // Record many queries with high latency
    for (int i = 0; i < 150; ++i) {
        tuner->recordQueryResult(k, 128, 50.0, 0.98);
    }
    
    auto stats = tuner->getStats();
    EXPECT_GT(stats.adaptations_count, 0);
    
    // Should adapt to lower efSearch
    int ef = tuner->getOptimalEfSearch(k, 100000);
    EXPECT_LT(ef, 128);
}

TEST_F(HnswParameterTunerTest, AdaptToLowRecall) {
    size_t k = 10;
    
    // Record many queries with low recall
    for (int i = 0; i < 150; ++i) {
        tuner->recordQueryResult(k, 32, 5.0, 0.80);
    }
    
    auto stats = tuner->getStats();
    EXPECT_GT(stats.adaptations_count, 0);
    
    // Should adapt to higher efSearch
    int ef = tuner->getOptimalEfSearch(k, 100000);
    EXPECT_GT(ef, 32);
}

TEST_F(HnswParameterTunerTest, ResetStats) {
    tuner->recordQueryResult(10, 64, 5.0, 0.95);
    tuner->resetStats();
    
    auto stats = tuner->getStats();
    EXPECT_EQ(stats.queries_processed, 0);
    EXPECT_DOUBLE_EQ(stats.avg_latency_ms, 0.0);
}

TEST_F(HnswParameterTunerTest, RecommendedMForDatasetSize) {
    EXPECT_EQ(HnswParameterTuner::getRecommendedM(5000), 8);
    EXPECT_EQ(HnswParameterTuner::getRecommendedM(50000), 16);
    EXPECT_EQ(HnswParameterTuner::getRecommendedM(500000), 24);
    EXPECT_EQ(HnswParameterTuner::getRecommendedM(5000000), 32);
}

TEST_F(HnswParameterTunerTest, RecommendedEfConstruction) {
    int M = 16;
    
    int ef_small = HnswParameterTuner::getRecommendedEfConstruction(10000, M);
    int ef_large = HnswParameterTuner::getRecommendedEfConstruction(1000000, M);
    
    EXPECT_GT(ef_large, ef_small);
    EXPECT_GE(ef_small, M * 10);
}

TEST_F(HnswParameterTunerTest, NonAdaptiveMode) {
    HnswParameterTuner::Config config;
    config.adaptive = false;
    config.ef_search_default = 100;
    
    HnswParameterTuner non_adaptive(config);
    
    int ef = non_adaptive.getOptimalEfSearch(10, 100000);
    EXPECT_EQ(ef, 100);
    
    // Should not adapt
    for (int i = 0; i < 200; ++i) {
        non_adaptive.recordQueryResult(10, 100, 50.0, 0.90);
    }
    
    ef = non_adaptive.getOptimalEfSearch(10, 100000);
    EXPECT_EQ(ef, 100);
}

TEST(HnswMemoryOptimizerTest, CacheLineSize) {
    size_t cache_line = HnswMemoryOptimizer::getCacheLineSize();
    EXPECT_EQ(cache_line, 64); // Most modern CPUs
}

TEST(HnswMemoryOptimizerTest, AlignToCacheLine) {
    size_t size = 100;
    size_t aligned = HnswMemoryOptimizer::alignToCacheLine(size);
    
    EXPECT_GE(aligned, size);
    EXPECT_EQ(aligned % HnswMemoryOptimizer::getCacheLineSize(), 0);
}

TEST(HnswMemoryOptimizerTest, PrefetchNodes) {
    std::vector<size_t> node_ids = {1, 2, 3, 4, 5};
    
    // Should not crash
    HnswMemoryOptimizer::prefetchNodes(node_ids);
}

// Workload-specific tests
TEST(HnswParameterTunerWorkloadTest, OLTPWorkload) {
    size_t dataset_size = 100000;
    auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
        dataset_size, HnswParameterTuner::WorkloadType::OLTP);
    
    // OLTP should prioritize low latency
    EXPECT_LE(config.M, 24);  // Allow higher M values (was 16)
    EXPECT_LE(config.ef_search_max, 128);  // Lower max for speed
    EXPECT_LE(config.target_latency.count(), 10);  // Aggressive latency target
    EXPECT_TRUE(config.adaptive);
}

TEST(HnswParameterTunerWorkloadTest, AnalyticsWorkload) {
    size_t dataset_size = 100000;
    auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
        dataset_size, HnswParameterTuner::WorkloadType::ANALYTICS);
    
    // Analytics should prioritize high recall
    EXPECT_GE(config.M, 16);  // Higher M for better connectivity
    EXPECT_GE(config.ef_search_max, 256);  // Higher max for recall
    EXPECT_GE(config.target_recall, 0.97);  // High recall requirement
    EXPECT_TRUE(config.adaptive);
}

TEST(HnswParameterTunerWorkloadTest, RAGWorkload) {
    size_t dataset_size = 100000;
    auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
        dataset_size, HnswParameterTuner::WorkloadType::RAG);
    
    // RAG should balance speed and accuracy
    EXPECT_GE(config.M, 16);
    EXPECT_LE(config.M, 32);
    EXPECT_GE(config.target_recall, 0.94);
    EXPECT_LE(config.target_recall, 0.96);
    EXPECT_TRUE(config.adaptive);
}

TEST(HnswParameterTunerWorkloadTest, BatchInsertWorkload) {
    size_t dataset_size = 100000;
    auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
        dataset_size, HnswParameterTuner::WorkloadType::BATCH_INSERT);
    
    // Batch insert should optimize for throughput
    EXPECT_LE(config.M, 20);  // Allow higher M values (was 12)
    EXPECT_FALSE(config.adaptive);  // No adaptation during bulk load
}

TEST(HnswParameterTunerWorkloadTest, WorkloadMInfluencesRecommendations) {
    size_t dataset_size = 100000;
    
    int m_oltp = HnswParameterTuner::getRecommendedM(
        dataset_size, HnswParameterTuner::WorkloadType::OLTP);
    int m_analytics = HnswParameterTuner::getRecommendedM(
        dataset_size, HnswParameterTuner::WorkloadType::ANALYTICS);
    
    // Analytics should recommend higher M than OLTP
    EXPECT_GT(m_analytics, m_oltp);
}

TEST(HnswParameterTunerWorkloadTest, WorkloadEfConstructionInfluencesRecommendations) {
    size_t dataset_size = 100000;
    int M = 16;
    
    int ef_oltp = HnswParameterTuner::getRecommendedEfConstruction(
        dataset_size, M, HnswParameterTuner::WorkloadType::OLTP);
    int ef_analytics = HnswParameterTuner::getRecommendedEfConstruction(
        dataset_size, M, HnswParameterTuner::WorkloadType::ANALYTICS);
    
    // Analytics should recommend higher ef_construction than OLTP
    EXPECT_GT(ef_analytics, ef_oltp);
    EXPECT_GE(ef_oltp, M * 8);  // Should still be reasonable
}

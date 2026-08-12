#include <gtest/gtest.h>
#include "index/hnsw_production_defaults.h"

using namespace themis::index;

// ============================================================================
// HnswProductionDefaults Tests
// ============================================================================

TEST(HnswProductionDefaults, GetRecommendedParamsSmallDataset) {
    auto params = HnswProductionDefaults::getRecommendedParams(
        5000, 768, HnswProductionDefaults::PerformanceProfile::BALANCED);
    
    // Small dataset should have smaller M
    EXPECT_LE(params.M, 16);
    EXPECT_GT(params.M, 0);
    EXPECT_GT(params.ef_construction, 0);
    EXPECT_GT(params.ef_search, 0);
}

TEST(HnswProductionDefaults, GetRecommendedParamsLargeDataset) {
    auto params = HnswProductionDefaults::getRecommendedParams(
        5000000, 768, HnswProductionDefaults::PerformanceProfile::BALANCED);
    
    // Large dataset should have larger M for better connectivity
    EXPECT_GE(params.M, 24);
    EXPECT_GT(params.ef_construction, params.M * 8);
}

TEST(HnswProductionDefaults, GetRecommendedParamsHighDimensional) {
    auto params = HnswProductionDefaults::getRecommendedParams(
        100000, 1536, HnswProductionDefaults::PerformanceProfile::BALANCED);
    
    // High dimensional vectors may have adjusted M
    EXPECT_GT(params.M, 0);
    EXPECT_GT(params.ef_construction, 0);
}

TEST(HnswProductionDefaults, PerformanceProfiles) {
    size_t dataset_size = 100000;
    size_t dimension = 768;
    
    auto latency_opt = HnswProductionDefaults::getRecommendedParams(
        dataset_size, dimension, HnswProductionDefaults::PerformanceProfile::LATENCY_OPTIMIZED);
    
    auto balanced = HnswProductionDefaults::getRecommendedParams(
        dataset_size, dimension, HnswProductionDefaults::PerformanceProfile::BALANCED);
    
    auto recall_opt = HnswProductionDefaults::getRecommendedParams(
        dataset_size, dimension, HnswProductionDefaults::PerformanceProfile::RECALL_OPTIMIZED);
    
    // Latency optimized should have lower ef_search
    EXPECT_LE(latency_opt.ef_search, balanced.ef_search);
    
    // Recall optimized should have higher ef_search
    EXPECT_GE(recall_opt.ef_search, balanced.ef_search);
}

TEST(HnswProductionDefaults, GetRecommendedM) {
    // Small dataset
    EXPECT_EQ(HnswProductionDefaults::getRecommendedM(5000), 8);
    
    // Medium dataset
    EXPECT_EQ(HnswProductionDefaults::getRecommendedM(50000), 16);
    
    // Large dataset
    EXPECT_EQ(HnswProductionDefaults::getRecommendedM(500000), 24);
    
    // Very large dataset
    EXPECT_EQ(HnswProductionDefaults::getRecommendedM(5000000), 32);
}

TEST(HnswProductionDefaults, GetRecommendedEfConstruction) {
    int M = 16;
    
    // Should scale with dataset size
    int ef_small = HnswProductionDefaults::getRecommendedEfConstruction(M, 5000);
    int ef_large = HnswProductionDefaults::getRecommendedEfConstruction(M, 1000000);
    
    EXPECT_GT(ef_large, ef_small);
    EXPECT_GE(ef_small, M * 8);  // At least 8x M
}

TEST(HnswProductionDefaults, GetRecommendedEfSearch) {
    size_t k = 10;
    const int k_int = static_cast<int>(k);
    
    int ef_latency = HnswProductionDefaults::getRecommendedEfSearch(
        k, HnswProductionDefaults::PerformanceProfile::LATENCY_OPTIMIZED);
    
    int ef_balanced = HnswProductionDefaults::getRecommendedEfSearch(
        k, HnswProductionDefaults::PerformanceProfile::BALANCED);
    
    int ef_recall = HnswProductionDefaults::getRecommendedEfSearch(
        k, HnswProductionDefaults::PerformanceProfile::RECALL_OPTIMIZED);
    
    // ef_search must be at least k
    EXPECT_GE(ef_latency, k_int);
    EXPECT_GE(ef_balanced, k_int);
    EXPECT_GE(ef_recall, k_int);
    
    // Profile ordering
    EXPECT_LE(ef_latency, ef_balanced);
    EXPECT_LE(ef_balanced, ef_recall);
}

TEST(HnswProductionDefaults, ValidateParams) {
    HnswProductionDefaults::HnswParams good_params;
    good_params.M = 16;
    good_params.ef_construction = 200;
    good_params.ef_search = 64;
    
    EXPECT_TRUE(HnswProductionDefaults::validateParams(good_params, 100000));
    
    // Bad M
    HnswProductionDefaults::HnswParams bad_m;
    bad_m.M = 2;  // Too low
    bad_m.ef_construction = 200;
    bad_m.ef_search = 64;
    
    EXPECT_FALSE(HnswProductionDefaults::validateParams(bad_m, 100000));
    
    // Bad ef_construction
    HnswProductionDefaults::HnswParams bad_ef;
    bad_ef.M = 16;
    bad_ef.ef_construction = 50;  // Too low relative to M
    bad_ef.ef_search = 64;
    
    EXPECT_FALSE(HnswProductionDefaults::validateParams(bad_ef, 100000));
}

TEST(HnswProductionDefaults, EstimateMemoryUsage) {
    HnswProductionDefaults::HnswParams params;
    params.M = 16;
    params.ef_construction = 200;
    params.ef_search = 64;
    
    size_t mem = HnswProductionDefaults::estimateMemoryUsage(params, 100000, 768);
    
    // Should be a reasonable estimate (not zero, not absurdly large)
    EXPECT_GT(mem, 100000 * 768 * sizeof(float));  // At least vector data
    EXPECT_LT(mem, 10ULL * 1024 * 1024 * 1024);    // Less than 10 GB for 100k vectors
}

TEST(HnswProductionDefaults, EstimateBuildTime) {
    HnswProductionDefaults::HnswParams params;
    params.M = 16;
    params.ef_construction = 200;
    params.ef_search = 64;
    
    double time = HnswProductionDefaults::estimateBuildTime(params, 100000, 768);
    
    // Should give a reasonable estimate
    EXPECT_GT(time, 10.0);    // At least 10 seconds for 100k vectors
    EXPECT_LT(time, 3600.0);  // Less than 1 hour
}

// ============================================================================
// HnswRuntimeAdapter Tests
// ============================================================================

TEST(HnswRuntimeAdapter, AdjustEfSearchForLowRecall) {
    int current_ef = 50;
    double actual_latency = 5.0;
    double target_latency = 10.0;
    double actual_recall = 0.90;
    double target_recall = 0.95;
    
    int new_ef = HnswRuntimeAdapter::adjustEfSearch(
        current_ef, actual_latency, target_latency, actual_recall, target_recall);
    
    // Should increase ef_search to improve recall
    EXPECT_GT(new_ef, current_ef);
}

TEST(HnswRuntimeAdapter, AdjustEfSearchForHighLatency) {
    int current_ef = 100;
    double actual_latency = 20.0;
    double target_latency = 10.0;
    double actual_recall = 0.98;
    double target_recall = 0.95;
    
    int new_ef = HnswRuntimeAdapter::adjustEfSearch(
        current_ef, actual_latency, target_latency, actual_recall, target_recall);
    
    // Should decrease ef_search to reduce latency
    EXPECT_LT(new_ef, current_ef);
}

TEST(HnswRuntimeAdapter, AdjustEfSearchOptimal) {
    int current_ef = 50;
    double actual_latency = 5.0;
    double target_latency = 10.0;
    double actual_recall = 0.96;
    double target_recall = 0.95;
    
    int new_ef = HnswRuntimeAdapter::adjustEfSearch(
        current_ef, actual_latency, target_latency, actual_recall, target_recall);
    
    // Performance is good, might adjust slightly or keep same
    EXPECT_GE(new_ef, current_ef - 10);
    EXPECT_LE(new_ef, current_ef + 10);
}

TEST(HnswRuntimeAdapter, AdjustEfSearchBounds) {
    int current_ef = 500;
    double actual_latency = 50.0;
    double target_latency = 10.0;
    
    int new_ef = HnswRuntimeAdapter::adjustEfSearch(
        current_ef, actual_latency, target_latency);
    
    // Should be clamped to maximum
    EXPECT_LE(new_ef, 512);
    EXPECT_GE(new_ef, 10);
}

TEST(HnswRuntimeAdapter, ShouldRebuildIndex) {
    // Small growth - no rebuild
    EXPECT_FALSE(HnswRuntimeAdapter::shouldRebuildIndex(15000, 10000));
    
    // Large growth - rebuild recommended
    EXPECT_TRUE(HnswRuntimeAdapter::shouldRebuildIndex(60000, 10000));
}

TEST(HnswRuntimeAdapter, GetOverfetchMultiplier) {
    size_t k = 10;
    
    // High selectivity - minimal overfetch
    double mult_high = HnswRuntimeAdapter::getOverfetchMultiplier(0.95, k);
    EXPECT_LE(mult_high, 2.0);
    
    // Medium selectivity
    double mult_medium = HnswRuntimeAdapter::getOverfetchMultiplier(0.5, k);
    EXPECT_GE(mult_medium, 1.5);
    
    // Low selectivity - high overfetch
    double mult_low = HnswRuntimeAdapter::getOverfetchMultiplier(0.05, k);
    EXPECT_GE(mult_low, 5.0);
}

TEST(HnswRuntimeAdapter, GetOverfetchMultiplierVeryLowSelectivity) {
    size_t k = 10;
    double very_low_selectivity = 0.001;
    
    double mult = HnswRuntimeAdapter::getOverfetchMultiplier(very_low_selectivity, k);
    
    // Should be bounded
    EXPECT_GE(mult, 10.0);
    EXPECT_LE(mult, 20.0);
}

// ============================================================================
// Workload-specific Tests
// ============================================================================

TEST(HnswProductionDefaultsWorkload, OLTPWorkload) {
    auto params = HnswProductionDefaults::getWorkloadOptimizedParams(
        150000, 768, HnswProductionDefaults::WorkloadType::OLTP);
    
    // OLTP should prioritize low latency and fast writes
    EXPECT_LE(params.M, 16);  // Lower M for faster construction
    EXPECT_LE(params.ef_search, 128);  // Lower ef_search for speed
    EXPECT_TRUE(params.use_prefetch);  // Dataset > MEDIUM_DATASET enables prefetch
}

TEST(HnswProductionDefaultsWorkload, AnalyticsWorkload) {
    auto params = HnswProductionDefaults::getWorkloadOptimizedParams(
        100000, 768, HnswProductionDefaults::WorkloadType::ANALYTICS);
    
    // Analytics should prioritize high recall
    EXPECT_GE(params.M, 16);  // Higher M for better connectivity
    EXPECT_GE(params.ef_construction, 200);  // Higher quality index
    EXPECT_GE(params.ef_search, 64);  // Higher ef_search for recall
}

TEST(HnswProductionDefaultsWorkload, RAGWorkload) {
    auto params = HnswProductionDefaults::getWorkloadOptimizedParams(
        100000, 768, HnswProductionDefaults::WorkloadType::RAG);
    
    // RAG should balance speed and accuracy
    EXPECT_GE(params.M, 16);
    EXPECT_LE(params.M, 40);
    EXPECT_GE(params.ef_search, 32);
    EXPECT_LE(params.ef_search, 256);
}

TEST(HnswProductionDefaultsWorkload, BatchInsertWorkload) {
    auto params = HnswProductionDefaults::getWorkloadOptimizedParams(
        100000, 768, HnswProductionDefaults::WorkloadType::BATCH_INSERT);
    
    // Batch insert should optimize for fast construction
    EXPECT_LE(params.M, 12);  // Lower M for faster bulk loading
    EXPECT_GE(params.initial_capacity, static_cast<size_t>(100000 * 1.3));  // More headroom
}

TEST(HnswProductionDefaultsWorkload, MixedWorkload) {
    auto params = HnswProductionDefaults::getWorkloadOptimizedParams(
        100000, 768, HnswProductionDefaults::WorkloadType::MIXED);
    
    // Mixed should be balanced
    EXPECT_GE(params.M, 12);
    EXPECT_LE(params.M, 24);
    EXPECT_GE(params.ef_search, 32);
    EXPECT_LE(params.ef_search, 256);
}

TEST(HnswProductionDefaultsWorkload, WorkloadInfluencesParameters) {
    size_t dataset_size = 100000;
    size_t dimension = 768;
    
    auto oltp = HnswProductionDefaults::getWorkloadOptimizedParams(
        dataset_size, dimension, HnswProductionDefaults::WorkloadType::OLTP);
    
    auto analytics = HnswProductionDefaults::getWorkloadOptimizedParams(
        dataset_size, dimension, HnswProductionDefaults::WorkloadType::ANALYTICS);
    
    // Analytics should have higher M and ef values than OLTP
    EXPECT_GT(analytics.M, oltp.M);
    EXPECT_GT(analytics.ef_construction, oltp.ef_construction);
}

TEST(HnswProductionDefaultsWorkload, WorkloadWithPerformanceProfile) {
    size_t dataset_size = 100000;
    size_t dimension = 768;
    
    // Test that workload parameter is used correctly
    auto params = HnswProductionDefaults::getRecommendedParams(
        dataset_size, dimension,
        HnswProductionDefaults::PerformanceProfile::BALANCED,
        HnswProductionDefaults::WorkloadType::OLTP);
    
    // Should show OLTP characteristics
    EXPECT_LE(params.M, 16);
    EXPECT_LE(params.initial_capacity, static_cast<size_t>(dataset_size * 1.4));
}

// ============================================================================
// autoTuneParameters tests
// ============================================================================

TEST(HnswProductionDefaultsAutoTune, LowLatencyTargetReducesEfSearch) {
    size_t dataset_size = 100000;
    size_t dimension = 768;

    auto params_fast = HnswProductionDefaults::autoTuneParameters(
        dataset_size, dimension, 100, 2.0 /*ms*/, 0.95);
    auto params_slow = HnswProductionDefaults::autoTuneParameters(
        dataset_size, dimension, 100, 50.0 /*ms*/, 0.95);

    // Aggressive latency target should yield lower ef_search
    EXPECT_LE(params_fast.ef_search, params_slow.ef_search);
}

TEST(HnswProductionDefaultsAutoTune, HighRecallTargetIncreasesEfConstruction) {
    size_t dataset_size = 100000;
    size_t dimension = 768;

    auto params_high_recall = HnswProductionDefaults::autoTuneParameters(
        dataset_size, dimension, 100, 10.0, 0.99);
    auto params_normal_recall = HnswProductionDefaults::autoTuneParameters(
        dataset_size, dimension, 100, 10.0, 0.90);

    // Higher recall target should produce a denser graph (larger ef_construction)
    EXPECT_GT(params_high_recall.ef_construction, params_normal_recall.ef_construction);
}

TEST(HnswProductionDefaultsAutoTune, ReturnsValidParams) {
    size_t dataset_size = 50000;
    size_t dimension = 512;

    auto params = HnswProductionDefaults::autoTuneParameters(
        dataset_size, dimension, 100, 10.0, 0.95);

    EXPECT_TRUE(HnswProductionDefaults::validateParams(params, dataset_size));
    EXPECT_GT(params.M, 0);
    EXPECT_GT(params.ef_construction, 0);
    EXPECT_GT(params.ef_search, 0);
}

/**
 * @file test_tensor_mid_layer_abstractions.cpp
 * @brief Comprehensive tests for tensor mid-layer abstractions.
 * 
 * Tests cover:
 * - Compression strategies (TT, Quantization, Sampling, Hashing)
 * - Tensor summary types
 * - Routing and prioritization strategies
 * - Redundancy detection
 */

#include <gtest/gtest.h>

#include "tensor/compression_strategy.h"
#include "tensor/tensor_routing_strategy.h"
#include "tensor/tensor_redundancy_detection.h"
#include "tensor/tensor_summary_types.h"

namespace themis {
namespace tensor {
namespace test {

// ============================================================================
// CompressionStrategy Tests
// ============================================================================

TEST(CompressionStrategyTest, TTDecompositionBasic) {
    TTDecompositionStrategy strategy;
    EXPECT_EQ(strategy.name(), "TT_DECOMPOSITION");

    float data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    CompressionConfig config;
    config.tt_epsilon = 0.01f;
    config.max_tt_rank = 4;

    auto result = strategy.compress(data, 10, {2, 5}, config);
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.compression_ratio, 0.0f);
    EXPECT_LE(result.achieved_error, config.tt_epsilon + 0.01f);
}

TEST(CompressionStrategyTest, TTDecompositionEstimate) {
    TTDecompositionStrategy strategy;
    float data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    CompressionConfig config;
    config.tt_epsilon = 0.01f;

    float ratio = strategy.estimateRatio(data, 16, config);
    EXPECT_GT(ratio, 1.0f);  // Should estimate compression
    EXPECT_LT(ratio, 100.0f);  // But not extreme
}

TEST(CompressionStrategyTest, QuantizationINT8) {
    QuantizationStrategy strategy(8);
    EXPECT_EQ(strategy.name(), "QUANTIZE_INT8");

    float data[8] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    CompressionConfig config;

    auto result = strategy.compress(data, 8, {}, config);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.compression_ratio, 4.0f);  // float32 to int8
    EXPECT_LT(result.achieved_error, 0.02f);
}

TEST(CompressionStrategyTest, QuantizationINT16) {
    QuantizationStrategy strategy(16);
    EXPECT_EQ(strategy.name(), "QUANTIZE_INT16");

    auto config = CompressionConfig();
    float ratio = strategy.estimateRatio(nullptr, 100, config);
    EXPECT_EQ(ratio, 2.0f);  // float32 to int16
}

TEST(CompressionStrategyTest, SamplingStrategy) {
    SamplingStrategy strategy(0.5f);
    EXPECT_EQ(strategy.name(), "SAMPLING");

    float data[100];
    for (int i = 0; i < 100; ++i) {
      data[i] = i * 0.1f;
    }

    CompressionConfig config;
    auto result = strategy.compress(data, 100, {}, config);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.compression_ratio, 2.0f);  // 50% sampling
    EXPECT_EQ(result.achieved_error, 0.0f);  // No approximation error
}

TEST(CompressionStrategyTest, HashingStrategy) {
    HashingStrategy strategy(64);
    EXPECT_EQ(strategy.name(), "HASHING");

    float data[256];
    for (int i = 0; i < 256; ++i) {
      data[i] = i * 0.01f;
    }

    CompressionConfig config;
    auto result = strategy.compress(data, 256, {}, config);
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.compression_ratio, 100.0f);  // Very high compression
}

TEST(CompressionFactoryTest, CreateValidStrategies) {
    auto tt = CompressionFactory::create("TT_DECOMPOSITION");
    EXPECT_NE(tt, nullptr);
    EXPECT_EQ(tt->name(), "TT_DECOMPOSITION");

    auto q8 = CompressionFactory::create("QUANTIZE_INT8");
    EXPECT_NE(q8, nullptr);

    auto samp = CompressionFactory::create("SAMPLING");
    EXPECT_NE(samp, nullptr);
}

TEST(CompressionFactoryTest, CreateInvalidStrategy) {
    auto invalid = CompressionFactory::create("NONEXISTENT_STRATEGY");
    EXPECT_EQ(invalid, nullptr);
}

// ============================================================================
// TensorSummaryTypes Tests
// ============================================================================

TEST(TensorSummaryTypesTest, AdapterSummaryCreation) {
    CompressionResult comp_result;
    comp_result.success = true;
    comp_result.compression_ratio = 2.5f;
    comp_result.achieved_rank = 8;

    auto summary = SummaryFactory::createAdapterSummary(
        "__adapters__:t1:legal:llama3", "llama3-8b", comp_result);

    EXPECT_EQ(summary.adapter_key, "__adapters__:t1:legal:llama3");
    EXPECT_EQ(summary.base_model_id, "llama3-8b");
    EXPECT_TRUE(summary.inference_ready);
    EXPECT_EQ(summary.avg_tt_rank, 8);
}

TEST(TensorSummaryTypesTest, PackageSummaryCreation) {
    std::vector<std::string> adapter_keys = {
        "__adapters__:t1:legal:llama3",
        "__adapters__:t1:legal:gpt4"
    };

    auto summary = SummaryFactory::createPackageSummary(
        "legal_package_v1", adapter_keys);

    EXPECT_EQ(summary.package_id, "legal_package_v1");
    EXPECT_EQ(summary.adapter_count, 2);
    EXPECT_EQ(summary.adapter_keys.size(), 2);
    EXPECT_TRUE(summary.production_ready);
}

TEST(TensorSummaryTypesTest, ShardSummaryCreation) {
    CompressionResult comp_result;
    comp_result.success = true;
    comp_result.compression_ratio = 1.5f;

    auto summary = SummaryFactory::createShardSummary(
        "shard_0", 1000, comp_result);

    EXPECT_EQ(summary.shard_id, "shard_0");
    EXPECT_EQ(summary.candidates_before_compression, 1000);
    EXPECT_EQ(summary.candidates_after_compression, static_cast<size_t>(1000 / 1.5f));
    EXPECT_TRUE(summary.shard_healthy);
}

TEST(TensorSummaryTypesTest, BaseSummaryMetadata) {
    AdapterSummary summary;
    summary.id = "test_adapter";
    summary.tenant_id = "tenant_1";
    summary.domain = "legal";
    summary.similarity_score = 0.92f;
    summary.confidence = 0.95f;

    EXPECT_EQ(summary.id, "test_adapter");
    EXPECT_EQ(summary.similarity_score, 0.92f);
    EXPECT_EQ(summary.confidence, 0.95f);
}

// ============================================================================
// RoutingStrategy Tests
// ============================================================================

TEST(PrioritizationTest, SimilarityBased) {
    SimilarityBasedPrioritization strategy;
    EXPECT_EQ(strategy.name(), "SIMILARITY_BASED");

    std::vector<BaseTensorSummary> summaries;
    
    BaseTensorSummary s1;
    s1.similarity_score = 0.9f;
    s1.confidence = 0.8f;
    summaries.push_back(s1);

    BaseTensorSummary s2;
    s2.similarity_score = 0.7f;
    s2.confidence = 0.85f;
    summaries.push_back(s2);

    // Test sorting
    EXPECT_TRUE(strategy.sort(summaries));
    EXPECT_GE(summaries[0].similarity_score, summaries[1].similarity_score);
}

TEST(PrioritizationTest, RankBased) {
    RankBasedPrioritization strategy;
    EXPECT_EQ(strategy.name(), "RANK_BASED");

    std::vector<BaseTensorSummary> summaries;
    BaseTensorSummary s;
    s.similarity_score = 0.8f;
    summaries.push_back(s);

    EXPECT_TRUE(strategy.sort(summaries));
}

TEST(PrioritizationTest, CostBased) {
    CostBasedPrioritization strategy;
    EXPECT_EQ(strategy.name(), "COST_BASED");

    std::vector<BaseTensorSummary> summaries;
    BaseTensorSummary s;
    s.similarity_score = 0.8f;
    s.compression_info.compression_ratio = 2.0f;
    summaries.push_back(s);

    EXPECT_TRUE(strategy.sort(summaries));
}

TEST(RoutingStrategyTest, QualityBasedRouting) {
    QualityBasedRouting strategy;
    EXPECT_EQ(strategy.name(), "QUALITY_BASED");

    std::vector<BaseTensorSummary> summaries;
    BaseTensorSummary s;
    s.similarity_score = 0.95f;
    s.confidence = 0.9f;
    s.compression_info.compression_ratio = 3.0f;
    summaries.push_back(s);

    index::AnnQueryContext context;
    auto decision = strategy.route(summaries, 100, 3.0f, context);

    EXPECT_EQ(decision.primary_target, "GRAPH_VALIDATION");
    EXPECT_GT(decision.confidence, 0.5f);
}

TEST(RoutingStrategyTest, ShardAwareRouting) {
    ShardAwareRouting strategy;
    EXPECT_EQ(strategy.name(), "SHARD_AWARE");

    std::vector<BaseTensorSummary> summaries;
    ShardSummary s;
    s.shard_id = "shard_0";
    s.shard_healthy = true;
    s.retrieval_latency_ms = 50.0f;
    summaries.push_back(s);

    index::AnnQueryContext context;
    auto decision = strategy.route(summaries, 100, 2.0f, context);

    EXPECT_NE(decision.primary_target, "");
}

TEST(AdaptiveRoutingTest, RecordOutcome) {
    AdaptiveRouting strategy;
    EXPECT_EQ(strategy.name(), "ADAPTIVE");

    RoutingDecision decision;
    decision.primary_target = "GRAPH_VALIDATION";

    strategy.recordOutcome(decision, true, 50.0f);
    strategy.recordOutcome(decision, true, 45.0f);
    strategy.recordOutcome(decision, false, 100.0f);

    // Should have recorded 3 outcomes without crashing
    // In production, we'd check internal metrics
}

TEST(RoutingFactoryTest, CreateValidRoutingStrategies) {
    auto quality = RoutingFactory::createRouting("QUALITY_BASED");
    EXPECT_NE(quality, nullptr);

    auto shard = RoutingFactory::createRouting("SHARD_AWARE");
    EXPECT_NE(shard, nullptr);

    auto adaptive = RoutingFactory::createRouting("ADAPTIVE");
    EXPECT_NE(adaptive, nullptr);
}

TEST(RoutingFactoryTest, CreateValidPrioritizationStrategies) {
    auto sim = RoutingFactory::createPrioritization("SIMILARITY_BASED");
    EXPECT_NE(sim, nullptr);

    auto rank = RoutingFactory::createPrioritization("RANK_BASED");
    EXPECT_NE(rank, nullptr);

    auto cost = RoutingFactory::createPrioritization("COST_BASED");
    EXPECT_NE(cost, nullptr);
}

// ============================================================================
// RedundancyDetection Tests
// ============================================================================

TEST(RedundancyDetectionTest, SimilarityBased) {
    SimilarityBasedDetector detector;
    EXPECT_EQ(detector.name(), "SIMILARITY_BASED");

    std::vector<BaseTensorSummary> summaries;
    
    BaseTensorSummary s1;
    s1.id = "s1";
    s1.similarity_score = 0.9f;
    summaries.push_back(s1);

    BaseTensorSummary s2;
    s2.id = "s2";
    s2.similarity_score = 0.91f;  // Very similar
    summaries.push_back(s2);

    BaseTensorSummary s3;
    s3.id = "s3";
    s3.similarity_score = 0.5f;  // Different
    summaries.push_back(s3);

    std::vector<const BaseTensorSummary*> ptrs = {&s1, &s2, &s3};
    auto metrics = detector.detect(ptrs, 0.05f);

    EXPECT_GT(metrics.redundant_count, 0);  // s1 and s2 are redundant
    EXPECT_LT(metrics.unique_count, summaries.size());
}

TEST(RedundancyDetectionTest, ContentHashDetector) {
    ContentHashDetector detector;
    EXPECT_EQ(detector.name(), "CONTENT_HASH");

    std::vector<BaseTensorSummary> summaries;
    
    BaseTensorSummary s1;
    s1.id = "same";
    s1.domain = "legal";
    s1.tenant_id = "t1";
    summaries.push_back(s1);

    BaseTensorSummary s2;
    s2.id = "same";  // Same content
    s2.domain = "legal";
    s2.tenant_id = "t1";
    summaries.push_back(s2);

    auto copy_summaries = summaries;
    auto removed = detector.deduplicate(copy_summaries, 0.0f);
    EXPECT_GT(removed.size(), 0);
}

TEST(RedundancyDetectionTest, EmbeddingBased) {
    EmbeddingBasedDetector detector;
    EXPECT_EQ(detector.name(), "EMBEDDING_BASED");

    std::vector<BaseTensorSummary> summaries;
    
    BaseTensorSummary s1;
    s1.domain = "legal";
    summaries.push_back(s1);

    BaseTensorSummary s2;
    s2.domain = "legal";
    summaries.push_back(s2);

    std::vector<const BaseTensorSummary*> ptrs = {&s1, &s2};
    auto metrics = detector.detect(ptrs, 0.1f);

    EXPECT_EQ(metrics.total_candidates, 2);
}

TEST(RedundancyDetectionTest, MetadataBased) {
    MetadataBasedDetector detector;
    EXPECT_EQ(detector.name(), "METADATA_BASED");

    BaseTensorSummary s1;
    s1.domain = "legal";
    s1.tenant_id = "t1";

    BaseTensorSummary s2;
    s2.domain = "legal";
    s2.tenant_id = "t1";

    EXPECT_TRUE(detector.areRedundant(s1, s2, 0.0f));
}

TEST(RedundancyDetectionTest, CompositeDetector) {
    auto composite = RedundancyFactory::createDefaultComposite();
    EXPECT_EQ(composite->name(), "COMPOSITE");

    std::vector<BaseTensorSummary> summaries;
    BaseTensorSummary s;
    s.similarity_score = 0.9f;
    summaries.push_back(s);

    auto removed = composite->deduplicate(summaries, 0.0f);
    // Should handle gracefully even if no duplicates
}

TEST(RedundancyFactoryTest, CreateDetectors) {
    auto sim = RedundancyFactory::create("SIMILARITY_BASED");
    EXPECT_NE(sim, nullptr);

    auto hash = RedundancyFactory::create("CONTENT_HASH");
    EXPECT_NE(hash, nullptr);

    auto embed = RedundancyFactory::create("EMBEDDING_BASED");
    EXPECT_NE(embed, nullptr);

    auto composite = RedundancyFactory::createDefaultComposite();
    EXPECT_NE(composite, nullptr);
}

} // namespace test
} // namespace tensor
} // namespace themis

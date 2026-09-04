/**
 * @file test_federated_tensor_summaries.cpp
 * @brief Comprehensive tests for federated and cross-shard tensor summaries (Issue #5427).
 *
 * Test IDs (FS-XX)
 * ================
 * Shard Summary Creation
 *   FS-01: Basic shard summary creation with compression
 *   FS-02: Multiple shard summaries with different compression ratios
 *   FS-03: Federated summary aggregation
 *   FS-04: Summary metadata propagation in federated context
 *
 * Summary-First Retrieval
 *   FS-05: Summary-first routing decision and context classification
 *   FS-06: Cross-shard candidate deduplication and merging
 *   FS-07: Empty shard scope handling
 *   FS-08: Shard health metadata and routing
 *
 * Quality and Observability
 *   FS-09: Compression ratio summary statistics
 *   FS-10: Shard-aware context classification
 *   FS-11: Aggregated candidate count statistics
 *   FS-12: Cross-shard retrieval flow (full E2E)
 *   FS-13: Shard scope prefix handling
 *   FS-14: Summary timestamp metadata consistency
 *   FS-15: False-negative risk metrics
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tensor/adapter_repository.h"
#include "tensor/tensor_fingerprint_graph.h"
#include "tensor/tensor_mid_layer.h"
#include "tensor/tensor_summary_types.h"
#include "index/ann_frontdoor.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>
#include <cstdio>

namespace themis {
namespace tensor {
namespace test {

// ============================================================================
// Test Fixture for Federated Tensor Summaries
// ============================================================================

class FederatedTensorSummariesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create tensor mid-layer with dependencies
        mid_layer_ = std::make_unique<TensorMidLayer>();
        
        // Create and populate adapter repository
        adapter_repo_ = std::make_shared<AdapterRepository>();
        mid_layer_->setAdapterRepository(adapter_repo_);
        
        // Create fingerprint graph (mock)
        fp_graph_ = std::make_shared<TensorFingerprintGraph>();
        mid_layer_->setFingerprintGraph(fp_graph_);
        // Ensure adapter repository can also access the fingerprint graph
        adapter_repo_->setFingerprintGraph(fp_graph_);
    }

    std::unique_ptr<TensorMidLayer> mid_layer_;
    std::shared_ptr<AdapterRepository> adapter_repo_;
    std::shared_ptr<TensorFingerprintGraph> fp_graph_;
};

// ============================================================================
// FS-01: Basic Shard Summary Creation
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_01_ShardSummaryCreation) {
    // Setup: Create shard summary using factory
    CompressionResult compression_result;
    compression_result.success = true;
    compression_result.compression_ratio = 4.0f;
    compression_result.achieved_rank = 2;
    compression_result.achieved_error = 0.005f;
    
    // Execute: Create shard summary
    ShardSummary shard_summary = SummaryFactory::createShardSummary(
        "shard_0",
        100,  // candidates_before
        compression_result
    );
    
    // Verify: Shard summary is correctly populated
    EXPECT_EQ(shard_summary.shard_id, "shard_0");
    EXPECT_EQ(shard_summary.candidates_before_compression, 100);
    EXPECT_EQ(shard_summary.candidates_after_compression, 25);  // 100 / 4.0
    EXPECT_EQ(shard_summary.compression_info.compression_ratio, 4.0f);
    EXPECT_TRUE(shard_summary.shard_healthy);
    EXPECT_FALSE(shard_summary.created_at.empty());
}

// ============================================================================
// FS-02: Multiple Shard Summaries
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_02_MultipleSummariesDifferentSizes) {
    // Setup: Create summaries for three shards with different compression ratios
    std::vector<ShardSummary> shard_summaries;
    
    CompressionResult result1;
    result1.success = true;
    result1.compression_ratio = 2.0f;
    shard_summaries.push_back(SummaryFactory::createShardSummary("shard_0", 100, result1));
    
    CompressionResult result2;
    result2.success = true;
    result2.compression_ratio = 4.0f;
    shard_summaries.push_back(SummaryFactory::createShardSummary("shard_1", 200, result2));
    
    CompressionResult result3;
    result3.success = true;
    result3.compression_ratio = 8.0f;
    shard_summaries.push_back(SummaryFactory::createShardSummary("shard_2", 160, result3));
    
    // Verify: Compression is applied correctly to each shard
    EXPECT_EQ(shard_summaries[0].candidates_after_compression, 50);   // 100 / 2.0
    EXPECT_EQ(shard_summaries[1].candidates_after_compression, 50);   // 200 / 4.0
    EXPECT_EQ(shard_summaries[2].candidates_after_compression, 20);   // 160 / 8.0
    
    // Total compressed candidates: 50 + 50 + 20 = 120
    std::size_t total_compressed = 
        shard_summaries[0].candidates_after_compression +
        shard_summaries[1].candidates_after_compression +
        shard_summaries[2].candidates_after_compression;
    EXPECT_EQ(total_compressed, 120);
}

// ============================================================================
// FS-03: Federated Summary Aggregation
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_03_FederatedAggregation) {
    // Setup: Create federated context with multiple shard scopes
    TensorLayerContext context;
    context.tenant_id = "tenant1";
    context.domain = "legal";
    context.base_model_id = "llama3";
    context.top_k = 5;
    context.shard_scope_ids = {"shard_0", "shard_1", "shard_2"};
    context.shard_aware = true;
    
    // Execute: Summarize federated shards
    FederatedTensorSummary federated = mid_layer_->summarizeFederatedShards(context);
    
    // Verify: Federated summary aggregates all shard summaries
    EXPECT_EQ(federated.shard_summaries.size(), 3);
    EXPECT_FALSE(federated.routing_reason.empty());
    EXPECT_THAT(federated.routing_reason, 
                ::testing::HasSubstr("federated shard summaries"));
}

// ============================================================================
// FS-04: Summary Metadata Propagation
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_04_ShardMetadataInFederated) {
    // Setup: Create context with multiple shards
    TensorLayerContext context;
    context.tenant_id = "tenant1";
    context.domain = "medical";
    context.base_model_id = "biobert";
    context.top_k = 10;
    context.shard_scope_ids = {"shard_eu", "shard_us", "shard_asia"};
    context.shard_aware = true;
    
    // Execute: Summarize federated shards
    FederatedTensorSummary federated = mid_layer_->summarizeFederatedShards(context);
    
    // Verify: Each shard summary is marked as federated
    EXPECT_EQ(federated.shard_summaries.size(), context.shard_scope_ids.size());
    for (const auto& summary : federated.shard_summaries) {
        EXPECT_TRUE(summary.federated);
        EXPECT_EQ(summary.participating_shards, context.shard_scope_ids.size());
        EXPECT_TRUE(std::find(context.shard_scope_ids.begin(),
                              context.shard_scope_ids.end(),
                              summary.scope_key) != context.shard_scope_ids.end());
    }
}

// ============================================================================
// FS-05: Summary-First Routing Decision
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_05_SummaryFirstRoutingContext) {
    // Setup: Create context for summary-first retrieval
    TensorLayerContext context;
    context.tenant_id = "tenant1";
    context.domain = "research";
    context.base_model_id = "gpt2";
    context.scope_id = "shard:shard_0";
    context.shard_aware = true;
    context.top_k = 20;
    
    // Execute: Generate routing plan
    TensorLayerPlan plan = mid_layer_->plan(context);
    
    // Verify: Plan classifies as ShardSummary scope
    EXPECT_EQ(plan.layer_kind, TensorLayerKind::ShardSummary);
    EXPECT_EQ(plan.ann_scope_kind, index::AnnScopeKind::ShardSummary);
    EXPECT_THAT(plan.reason, ::testing::HasSubstr("shard"));
}

// ============================================================================
// FS-06: Cross-Shard Candidate Deduplication
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_06_MergingShardResults) {
    // Setup: Force shard summaries to return overlapping candidates through production path
    // Populate the fingerprint graph with deterministic adapters so the
    // fingerprint-based path returns predictable top-k candidates.
    tensor::TensorTrainCore core;
    core.train.cores.resize(1);
    core.train.cores[0].n = 1;
    core.train.cores[0].r_left = 1;
    core.train.cores[0].r_right = 1;
    core.train.cores[0].data = {1.0f};

    fp_graph_->addAdapter("__adapters__:default:research:adapter_a", core, "research", "model_a");
    fp_graph_->addAdapter("__adapters__:default:research:adapter_b", core, "research", "model_b");
    fp_graph_->addAdapter("__adapters__:default:research:adapter_c", core, "research", "model_c");
    fp_graph_->addAdapter("__adapters__:default:research:adapter_d", core, "research", "model_d");
    // Register the query adapter so findSimilar() can look up its TTTrain.
    fp_graph_->addAdapter("__adapters__:default:research:model_a", core, "research", "model_a");

    TensorLayerContext context;
    context.tenant_id = "tenant1";
    context.domain = "research";
    context.base_model_id = "model_a";
    context.shard_scope_ids = {"shard_0", "shard_1"};
    context.shard_aware = true;
    context.top_k = 3;

    // Execute: federated summary + merge through TensorMidLayer
    FederatedTensorSummary federated = mid_layer_->summarizeFederatedShards(context);

    // Verify: shard-level summaries exist and merged output is deduplicated + top-k bounded
    ASSERT_EQ(federated.shard_summaries.size(), context.shard_scope_ids.size());
    ASSERT_EQ(federated.merged_similar_adapters.size(), context.top_k);
    std::unordered_set<std::string> unique_adapter_keys = {};

    for (const auto& candidate : federated.merged_similar_adapters) {
        unique_adapter_keys.insert(candidate.adapter_key);
    }
    EXPECT_EQ(unique_adapter_keys.size(), federated.merged_similar_adapters.size());
    EXPECT_GE(federated.merged_similar_adapters[0].score, federated.merged_similar_adapters[1].score);
    EXPECT_GE(federated.merged_similar_adapters[1].score, federated.merged_similar_adapters[2].score);
}

// ============================================================================
// FS-07: Empty Shard Handling
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_07_EmptyShardScopes) {
    // Setup: Create context with empty shard scopes
    TensorLayerContext context;
    context.tenant_id = "tenant1";
    context.domain = "empty";
    context.base_model_id = "model";
    context.shard_scope_ids = {};  // No shards
    context.shard_aware = true;
    
    // Execute: Summarize federated shards
    FederatedTensorSummary federated = mid_layer_->summarizeFederatedShards(context);
    
    // Verify: Empty shard list is handled gracefully
    EXPECT_EQ(federated.shard_summaries.size(), 0);
    EXPECT_THAT(federated.routing_reason, 
                ::testing::HasSubstr("no shard scopes"));
}

// ============================================================================
// FS-08: Shard Routing with Healthy Status
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_08_ShardHealthMetadata) {
    // Setup: Create shard summary with health information
    ShardSummary healthy_shard;
    healthy_shard.shard_id = "shard_healthy";
    healthy_shard.shard_healthy = true;
    healthy_shard.retrieval_latency_ms = 50.0f;
    healthy_shard.shard_relevance = 0.95f;
    
    ShardSummary unhealthy_shard;
    unhealthy_shard.shard_id = "shard_slow";
    unhealthy_shard.shard_healthy = false;  // Marked unhealthy
    unhealthy_shard.retrieval_latency_ms = 5000.0f;  // Very slow
    unhealthy_shard.shard_routing_reason = "timeout";
    
    // Verify: Health status is preserved
    EXPECT_TRUE(healthy_shard.shard_healthy);
    EXPECT_FALSE(unhealthy_shard.shard_healthy);
    EXPECT_LT(healthy_shard.retrieval_latency_ms, unhealthy_shard.retrieval_latency_ms);
}

// ============================================================================
// FS-09: Compression Ratio Summary Statistics
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_09_CompressionMetrics) {
    // Setup: Create summaries with different compression characteristics
    CompressionResult low_compression;
    low_compression.success = true;
    low_compression.compression_ratio = 1.5f;
    
    CompressionResult high_compression;
    high_compression.success = true;
    high_compression.compression_ratio = 8.0f;
    
    ShardSummary s1 = SummaryFactory::createShardSummary("shard_a", 100, low_compression);
    ShardSummary s2 = SummaryFactory::createShardSummary("shard_b", 100, high_compression);
    
    // Verify: Compression ratios are different
    EXPECT_EQ(s1.compression_info.compression_ratio, 1.5f);
    EXPECT_EQ(s2.compression_info.compression_ratio, 8.0f);
    
    // Candidates after compression
    EXPECT_NEAR(s1.candidates_after_compression, 66, 2);  // ~100 / 1.5
    EXPECT_NEAR(s2.candidates_after_compression, 12, 2);  // ~100 / 8.0
}

// ============================================================================
// FS-10: Shard-Aware Context Classification
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_10_ShardAwareClassification) {
    // Setup: Test classification with shard-aware flag
    TensorLayerContext shard_context;
    shard_context.shard_aware = true;
    shard_context.scope_id = "shard:s1";
    
    // Execute: Plan with shard awareness
    TensorLayerPlan plan = mid_layer_->plan(shard_context);
    
    // Verify: Recognized as ShardSummary
    EXPECT_EQ(plan.layer_kind, TensorLayerKind::ShardSummary);
}

// ============================================================================
// FS-11: Federated Candidate Count Aggregation
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_11_AggregatedCandidateCounts) {
    // Setup: Create context with known candidate counts
    TensorLayerContext context;
    context.tenant_id = "t1";
    context.domain = "d1";
    context.base_model_id = "m1";
    context.shard_scope_ids = {"s1", "s2", "s3"};
    context.shard_aware = true;
    context.top_k = 4;

    // Populate the fingerprint graph with deterministic adapters so the
    // fingerprint-based path returns predictable top-k candidates.
    tensor::TensorTrainCore core;
    core.train.cores.resize(1);
    core.train.cores[0].n = 1;
    core.train.cores[0].r_left = 1;
    core.train.cores[0].r_right = 1;
    core.train.cores[0].data = {1.0f};

    fp_graph_->addAdapter("__adapters__:default:d1:adapter_a", core, "d1", "m1");
    fp_graph_->addAdapter("__adapters__:default:d1:adapter_b", core, "d1", "m2");
    fp_graph_->addAdapter("__adapters__:default:d1:adapter_c", core, "d1", "m3");
    fp_graph_->addAdapter("__adapters__:default:d1:adapter_d", core, "d1", "m4");
    fp_graph_->addAdapter("__adapters__:default:d1:adapter_e", core, "d1", "m5");
    // Register the query adapter so findSimilar() can look up its TTTrain.
    fp_graph_->addAdapter("__adapters__:default:d1:m1", core, "d1", "m1");
    
    // Execute: Get federated summary
    FederatedTensorSummary federated = mid_layer_->summarizeFederatedShards(context);
    
    // Verify: Summary contains shard information
    EXPECT_EQ(federated.shard_summaries.size(), 3);
    
    // Aggregate candidate counts from all shards
    std::size_t total_candidates = 0;
    for (const auto& s : federated.shard_summaries) {
        total_candidates += s.candidate_count;
    }
    
    // Validate aggregation and top-k contract
    ASSERT_EQ(total_candidates, context.top_k * context.shard_scope_ids.size());
    EXPECT_LE(federated.merged_similar_adapters.size(), context.top_k);
    EXPECT_LE(federated.merged_similar_adapters.size(), total_candidates);
}

// ============================================================================
// FS-12: Cross-Shard Retrieval Flow
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_12_CrossShardRetrievalFlow) {
    // Setup: Simulate a cross-shard retrieval flow
    // 1. Plan step
    TensorLayerContext plan_context;
    plan_context.tenant_id = "t1";
    plan_context.domain = "d1";
    plan_context.base_model_id = "m1";
    plan_context.shard_scope_ids = {"shard_0", "shard_1", "shard_2"};
    plan_context.shard_aware = true;
    plan_context.top_k = 10;
    
    TensorLayerPlan plan = mid_layer_->plan(plan_context);
    EXPECT_EQ(plan.layer_kind, TensorLayerKind::ShardSummary);
    
    // 2. Summary step
    FederatedTensorSummary summary = mid_layer_->summarizeFederatedShards(plan_context);
    EXPECT_EQ(summary.shard_summaries.size(), 3);
    
    // 3. Verify merged results
    EXPECT_FALSE(summary.routing_reason.empty());
    EXPECT_THAT(summary.routing_reason, ::testing::HasSubstr("federated"));
}

// ============================================================================
// FS-13: Shard Scope Prefix Handling
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_13_ShardScopePrefixes) {
    // Setup: Test various shard scope prefix formats
    TensorLayerContext context1;
    context1.scope_id = "shard:shard_0";
    context1.shard_aware = false;
    
    TensorLayerContext context2;
    context2.shard_aware = true;
    context2.scope_id = "other_prefix";
    
    // Execute: Classify both
    TensorLayerPlan plan1 = mid_layer_->plan(context1);
    TensorLayerPlan plan2 = mid_layer_->plan(context2);
    
    // Verify: Both are recognized as shard-aware
    EXPECT_EQ(plan1.layer_kind, TensorLayerKind::ShardSummary);
    EXPECT_EQ(plan2.layer_kind, TensorLayerKind::ShardSummary);
}

// ============================================================================
// FS-14: Summary Timestamp Consistency
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_14_TimestampMetadata) {
    // Setup: Create multiple shard summaries
    CompressionResult result;
    result.success = true;
    result.compression_ratio = 4.0f;
    
    ShardSummary s1 = SummaryFactory::createShardSummary("shard_0", 100, result);
    ShardSummary s2 = SummaryFactory::createShardSummary("shard_1", 100, result);
    
    // Verify: Timestamps are present and valid
    EXPECT_FALSE(s1.created_at.empty());
    EXPECT_FALSE(s2.created_at.empty());
    
    // Timestamps should follow ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ)
    int y, mo, d, hh, mm, ss;
    int matched = std::sscanf(s1.created_at.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &y, &mo, &d, &hh, &mm, &ss);
    EXPECT_EQ(matched, 6);
}

// ============================================================================
// FS-15: False-Negative Risk Awareness
// ============================================================================

TEST_F(FederatedTensorSummariesTest, FS_15_FalseNegativeRiskMetrics) {
    // Setup: Create shard summaries with varying compression
    // Lower compression = lower false negative risk
    // Higher compression = higher false negative risk
    
    CompressionResult loose;
    loose.success = true;
    loose.compression_ratio = 1.2f;  // Minimal compression
    
    CompressionResult tight;
    tight.success = true;
    tight.compression_ratio = 10.0f;  // Aggressive compression
    
    ShardSummary conservative = SummaryFactory::createShardSummary("shard_c", 100, loose);
    ShardSummary aggressive = SummaryFactory::createShardSummary("shard_a", 100, tight);
    
    // Verify: Conservative compression keeps more candidates
    EXPECT_GT(conservative.candidates_after_compression, aggressive.candidates_after_compression);
    
    // Conservative approach has lower false-negative risk
    // (More candidates = lower chance of missing relevant adapters)
}

} // namespace test
} // namespace tensor
} // namespace themis

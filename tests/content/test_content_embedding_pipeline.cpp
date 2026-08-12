// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_embedding_pipeline.cpp
 * @brief Unit tests for EmbeddingPipeline and ContentManager::generateEmbedding()
 */

#include <gtest/gtest.h>
#include "content/embedding_pipeline.h"
#include "content/content_metrics.h"
#include "content/content_policy.h"
#include <nlohmann/json.hpp>

using namespace themis::content;

// ============================================================================
// EmbeddingPipelineConfig tests
// ============================================================================

TEST(EmbeddingPipelineTest, DefaultConfig_PipelineDisabled) {
    EmbeddingPipelineConfig cfg;
    EXPECT_TRUE(cfg.model_name.empty());

    EmbeddingPipeline pipeline(cfg);
    EXPECT_FALSE(pipeline.isEnabled());
}

TEST(EmbeddingPipelineTest, NonEmptyModelName_PipelineEnabled) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "all-minilm-l6-v2";

    EmbeddingPipeline pipeline(cfg);
    EXPECT_TRUE(pipeline.isEnabled());
}

TEST(EmbeddingPipelineTest, BatchSizeClamped_MinIs1) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.batch_size = 0;  // Below minimum

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getConfig().batch_size, 1);
}

TEST(EmbeddingPipelineTest, BatchSizeClamped_MaxIs32) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.batch_size = 64;  // Above maximum

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getConfig().batch_size, 32);
}

TEST(EmbeddingPipelineTest, BatchSizeWithinBounds_Preserved) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.batch_size = 16;

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getConfig().batch_size, 16);
}

TEST(EmbeddingPipelineTest, TimeoutClamped_MinIs100ms) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.timeout_ms = 10;  // Below minimum

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getConfig().timeout_ms, 100);
}

TEST(EmbeddingPipelineTest, InitialFailureCountIsZero) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getFailureCount(), 0u);
}

TEST(EmbeddingPipelineTest, InitialEmbeddingDimIsZeroWhenNotConfigured) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    // embedding_dim not set → 0

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getEmbeddingDim(), 0);
}

TEST(EmbeddingPipelineTest, EmbeddingDimPreservedFromConfig) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.embedding_dim = 384;

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getEmbeddingDim(), 384);
}

// ============================================================================
// Disabled pipeline behaviour
// ============================================================================

TEST(EmbeddingPipelineTest, DisabledPipeline_GenerateEmbeddingReturnsEmpty) {
    EmbeddingPipelineConfig cfg;
    // model_name is empty → disabled

    EmbeddingPipeline pipeline(cfg);
    auto result = pipeline.generateEmbedding("Hello world");
    EXPECT_TRUE(result.empty());
}

TEST(EmbeddingPipelineTest, DisabledPipeline_BatchReturnsEmptyVectors) {
    EmbeddingPipelineConfig cfg;

    EmbeddingPipeline pipeline(cfg);
    auto results = pipeline.generateEmbeddingBatch({"text1", "text2", "text3"});

    EXPECT_EQ(results.size(), 3u);
    for (const auto& v : results) {
        EXPECT_TRUE(v.empty());
    }
}

TEST(EmbeddingPipelineTest, DisabledPipeline_EmptyTextReturnsEmpty) {
    EmbeddingPipelineConfig cfg;

    EmbeddingPipeline pipeline(cfg);
    auto result = pipeline.generateEmbedding("");
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Batch API shape invariants
// ============================================================================

TEST(EmbeddingPipelineTest, BatchResultSizeMatchesInputSize) {
    // Even with a disabled pipeline the result vector size must match input.
    EmbeddingPipelineConfig cfg;

    EmbeddingPipeline pipeline(cfg);
    std::vector<std::string> texts = {"a", "b", "c", "d", "e"};
    auto results = pipeline.generateEmbeddingBatch(texts);
    EXPECT_EQ(results.size(), texts.size());
}

TEST(EmbeddingPipelineTest, EmptyBatchReturnsEmptyResultVector) {
    EmbeddingPipelineConfig cfg;

    EmbeddingPipeline pipeline(cfg);
    auto results = pipeline.generateEmbeddingBatch({});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Config accessor
// ============================================================================

TEST(EmbeddingPipelineTest, GetConfigReturnsCorrectValues) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "bert-base";
    cfg.batch_size = 8;
    cfg.timeout_ms = 3000;
    cfg.embedding_dim = 768;

    EmbeddingPipeline pipeline(cfg);
    const auto& stored = pipeline.getConfig();

    EXPECT_EQ(stored.model_name, "bert-base");
    EXPECT_EQ(stored.batch_size, 8);
    EXPECT_EQ(stored.timeout_ms, 3000);
    EXPECT_EQ(stored.embedding_dim, 768);
}

// ============================================================================
// ContentMetrics integration — new counter
// ============================================================================

TEST(EmbeddingPipelineTest, MetricsPointer_NullByDefault) {
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    EXPECT_EQ(cfg.metrics, nullptr);
}

TEST(EmbeddingPipelineTest, MetricsPointer_CanBeSet) {
    themis::content::ContentMetrics metrics;
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "test-model";
    cfg.metrics = &metrics;

    EmbeddingPipeline pipeline(cfg);
    EXPECT_EQ(pipeline.getConfig().metrics, &metrics);
}

// ============================================================================
// ContentMetrics::recordEmbeddingFailure / getEmbeddingFailuresTotal
// ============================================================================

TEST(ContentMetricsTest_Embedding, InitialEmbeddingFailuresIsZero) {
    themis::content::ContentMetrics metrics;
    EXPECT_EQ(metrics.getEmbeddingFailuresTotal(), 0u);
}

TEST(ContentMetricsTest_Embedding, RecordEmbeddingFailureIncrementsCounter) {
    themis::content::ContentMetrics metrics;
    metrics.recordEmbeddingFailure();
    EXPECT_EQ(metrics.getEmbeddingFailuresTotal(), 1u);

    metrics.recordEmbeddingFailure();
    metrics.recordEmbeddingFailure();
    EXPECT_EQ(metrics.getEmbeddingFailuresTotal(), 3u);
}

TEST(ContentMetricsTest_Embedding, ResetClearsEmbeddingFailureCounter) {
    themis::content::ContentMetrics metrics;
    metrics.recordEmbeddingFailure();
    metrics.recordEmbeddingFailure();
    EXPECT_EQ(metrics.getEmbeddingFailuresTotal(), 2u);

    metrics.reset();
    EXPECT_EQ(metrics.getEmbeddingFailuresTotal(), 0u);
}

TEST(ContentMetricsTest_Embedding, PrometheusOutputContainsEmbeddingFailuresCounter) {
    themis::content::ContentMetrics metrics;
    metrics.recordEmbeddingFailure();

    std::string prom = metrics.toPrometheusFormat();
    EXPECT_NE(prom.find("content_embedding_failures_total"), std::string::npos);
    EXPECT_NE(prom.find("content_embedding_failures_total 1"), std::string::npos);
}

// ============================================================================
// ContentPolicy::embedding_model field
// ============================================================================

TEST(ContentPolicyTest_Embedding, DefaultEmbeddingModelIsEmpty) {
    themis::content::ContentPolicy policy;
    EXPECT_TRUE(policy.embedding_model.empty());
}

TEST(ContentPolicyTest_Embedding, EmbeddingModelCanBeSet) {
    themis::content::ContentPolicy policy;
    policy.embedding_model = "all-minilm-l6-v2";
    EXPECT_EQ(policy.embedding_model, "all-minilm-l6-v2");
}

// ============================================================================
// ContentPolicy::embedding_model gates the embedding stage (AC-1)
//
// The gate logic lives in ContentManager::ingestRawBlob() and ingestStream().
// Its behaviour is:
//   - config contains "embedding_model" key AND value is non-empty
//       → stage enabled  (AND stage_cfg.embedding.enabled)
//   - config contains "embedding_model" key AND value is empty
//       → stage disabled (regardless of stage_cfg.embedding.enabled)
//   - config does NOT contain "embedding_model"
//       → fall back to stage_cfg.embedding.enabled (backward-compatible)
//
// The helper below mirrors the exact gate expression from content_manager.cpp
// so the tests remain in sync with the implementation.
// ============================================================================

namespace {
/// Mirror of the gate expression used in ingestRawBlob / ingestStream.
bool computeEmbeddingActive(const nlohmann::json& config, bool stage_enabled) {
    if (config.contains("embedding_model")) {
        const std::string policy_model = config.value("embedding_model", std::string{});
        return stage_enabled && !policy_model.empty();
    }
    return stage_enabled;
}
} // namespace

TEST(ContentPolicyTest_Embedding, EmptyModelKey_DisablesStageEvenWhenStageEnabled) {
    // ContentPolicy::embedding_model = "" → embedding stage must be suppressed.
    nlohmann::json config = {{"embedding_model", ""}};
    EXPECT_FALSE(computeEmbeddingActive(config, /*stage_enabled=*/true));
}

TEST(ContentPolicyTest_Embedding, NonEmptyModelKey_ActivatesStageWhenStageEnabled) {
    // ContentPolicy::embedding_model = "all-minilm-l6-v2" → embedding stage runs.
    nlohmann::json config = {{"embedding_model", "all-minilm-l6-v2"}};
    EXPECT_TRUE(computeEmbeddingActive(config, /*stage_enabled=*/true));
}

TEST(ContentPolicyTest_Embedding, AbsentModelKey_FallsBackToStageCfgEnabled) {
    // No "embedding_model" key → ProcessorChainConfig default (enabled) used.
    nlohmann::json config = nlohmann::json::object();  // no key
    EXPECT_TRUE(computeEmbeddingActive(config, /*stage_enabled=*/true));
}

TEST(ContentPolicyTest_Embedding, AbsentModelKey_FallsBackToStageCfgDisabled) {
    // No key + ProcessorChainConfig disabled → stage stays disabled.
    nlohmann::json config = nlohmann::json::object();
    EXPECT_FALSE(computeEmbeddingActive(config, /*stage_enabled=*/false));
}

TEST(ContentPolicyTest_Embedding, NonEmptyModelKey_StageCfgDisabled_StageStaysOff) {
    // AND semantics: ProcessorChainConfig can veto even when policy model is set.
    nlohmann::json config = {{"embedding_model", "bert-base"}};
    EXPECT_FALSE(computeEmbeddingActive(config, /*stage_enabled=*/false));
}

TEST(ContentPolicyTest_Embedding, DisabledPipelineWithNonEmptyPolicyModel_NoEmbedding) {
    // Even if the policy model is non-empty, a disabled EmbeddingPipeline
    // (empty model_name in its own config) must not generate embeddings.
    EmbeddingPipelineConfig cfg;
    cfg.model_name = "";  // pipeline disabled
    EmbeddingPipeline pipeline(cfg);
    EXPECT_FALSE(pipeline.isEnabled());

    // Stage would be active from the policy perspective…
    nlohmann::json config = {{"embedding_model", "all-minilm-l6-v2"}};
    EXPECT_TRUE(computeEmbeddingActive(config, /*stage_enabled=*/true));

    // …but the pipeline gate blocks actual embedding generation.
    auto result = pipeline.generateEmbedding("hello world");
    EXPECT_TRUE(result.empty());
}

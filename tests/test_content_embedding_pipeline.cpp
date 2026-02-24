// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_embedding_pipeline.cpp
 * @brief Unit tests for EmbeddingPipeline and ContentManager::generateEmbedding()
 */

#include <gtest/gtest.h>
#include "content/embedding_pipeline.h"

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

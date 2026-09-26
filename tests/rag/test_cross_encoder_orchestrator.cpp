// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/cross_encoder_orchestrator.h"

namespace themis::rag {

class CrossEncoderOrchestratorTest : public ::testing::Test {
 protected:
  CrossEncoderOrchestratorTest() {
    RerankerConfig config;
    config.model_name = "cross-encoder/ms-marco-MiniLM-L-12-v2";
    config.batch_size = 32;
    orchestrator_ = std::make_unique<CrossEncoderOrchestrator>(config);
  }

  std::unique_ptr<CrossEncoderOrchestrator> orchestrator_;
};

TEST_F(CrossEncoderOrchestratorTest, InitializeWithValidConfig) {
  RerankerConfig config;
  config.model_name = "test-model";
  config.batch_size = 16;
  
  EXPECT_NO_THROW(CrossEncoderOrchestrator orchestrator(config));
}

TEST_F(CrossEncoderOrchestratorTest, ThrowOnEmptyModelName) {
  RerankerConfig config;
  config.model_name = "";
  config.batch_size = 16;
  
  EXPECT_THROW(CrossEncoderOrchestrator orchestrator(config), std::invalid_argument);
}

TEST_F(CrossEncoderOrchestratorTest, ThrowOnZeroBatchSize) {
  RerankerConfig config;
  config.model_name = "test";
  config.batch_size = 0;
  
  EXPECT_THROW(CrossEncoderOrchestrator orchestrator(config), std::invalid_argument);
}

TEST_F(CrossEncoderOrchestratorTest, RerankReturnsTopK) {
  std::vector<CrossEncoderOrchestrator::Document> docs = {
      {1, "Document about machine learning", 0.7f},
      {2, "Document about deep learning", 0.6f},
      {3, "Document about AI", 0.5f},
      {4, "Document about Python", 0.4f},
      {5, "Document about algorithms", 0.3f},
      {6, "Document about neural networks", 0.2f},
      {7, "Document about statistics", 0.1f},
      {8, "Document about data science", 0.05f},
      {9, "Document about cloud", 0.02f},
      {10, "Document about networks", 0.01f},
      {11, "Document about storage", 0.005f}
  };

  auto results = orchestrator_->Rerank("What is machine learning?", docs);
  
  EXPECT_LE(results.reranked_docs.size(), 10);
  EXPECT_EQ(results.reranked_docs.size(), results.relevance_scores.size());
  EXPECT_GT(results.latency_ms, 0);
}

TEST_F(CrossEncoderOrchestratorTest, RerankWithFewDocuments) {
  std::vector<CrossEncoderOrchestrator::Document> docs = {
      {1, "Doc A", 0.8f},
      {2, "Doc B", 0.6f}
  };

  auto results = orchestrator_->Rerank("query", docs);
  
  EXPECT_EQ(results.reranked_docs.size(), 2);
  EXPECT_EQ(results.relevance_scores.size(), 2);
}

TEST_F(CrossEncoderOrchestratorTest, BatchRerankingProducesCorrectSize) {
  std::vector<CrossEncoderOrchestrator::RerankerQuery> queries = {
      {"query1", {{1, "doc1", 0.8f}, {2, "doc2", 0.7f}}},
      {"query2", {{3, "doc3", 0.9f}, {4, "doc4", 0.6f}}},
      {"query3", {{5, "doc5", 0.7f}}}
  };

  auto batch_results = orchestrator_->ReankBatch(queries);
  
  EXPECT_EQ(batch_results.size(), 3);
  for (const auto& result : batch_results) {
    EXPECT_LE(result.reranked_docs.size(), 10);
    EXPECT_EQ(result.reranked_docs.size(), result.relevance_scores.size());
  }
}

TEST_F(CrossEncoderOrchestratorTest, CacheStatsInitialization) {
  auto stats = orchestrator_->GetCacheStats();
  
  EXPECT_EQ(stats.total_lookups, 0);
  EXPECT_EQ(stats.cache_hits, 0);
  EXPECT_EQ(stats.hit_rate, 0.0f);
}

TEST_F(CrossEncoderOrchestratorTest, ClearCacheResetsStats) {
  orchestrator_->Rerank("query", {{1, "doc", 0.8f}});
  
  orchestrator_->ClearCache();
  
  auto stats = orchestrator_->GetCacheStats();
  EXPECT_EQ(stats.cache_hits, 0);
  EXPECT_EQ(stats.total_lookups, 0);
}

TEST_F(CrossEncoderOrchestratorTest, ModelLoadingState) {
  EXPECT_EQ(orchestrator_->GetModelName(), "cross-encoder/ms-marco-MiniLM-L-12-v2");
  
  // First rerank should trigger model load
  orchestrator_->Rerank("query", {{1, "doc", 0.8f}});
  
  EXPECT_TRUE(orchestrator_->IsModelLoaded());
  
  orchestrator_->UnloadModel();
  EXPECT_FALSE(orchestrator_->IsModelLoaded());
}

TEST_F(CrossEncoderOrchestratorTest, TokenCounting) {
  std::vector<CrossEncoderOrchestrator::Document> docs = {
      {1, "short doc", 0.8f}
  };
  
  auto results = orchestrator_->Rerank("This is a test query with several tokens", docs);
  
  EXPECT_GT(results.tokens_used, 0);
}

}  // namespace themis::rag

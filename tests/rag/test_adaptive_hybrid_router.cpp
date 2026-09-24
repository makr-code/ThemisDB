// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/adaptive_hybrid_router.h"

namespace themis::rag {

class AdaptiveHybridRouterTest : public ::testing::Test {
 protected:
  AdaptiveHybridRouterTest() {
    // Initialize with fallback weights
    config_.fallback_lexical_weight = 0.3f;
    config_.fallback_dense_weight = 0.5f;
    config_.fallback_graph_weight = 0.2f;
  }

  AdaptiveHybridRouter::Config config_;
  AdaptiveHybridRouter router_{config_};
};

TEST_F(AdaptiveHybridRouterTest, RouteWithStaticWeights) {
  AdaptiveHybridRouter::RoutingDecision decision = router_.Route(
      "What is machine learning?",
      {0.0f, 0.0f, 0.0f}  // No policy weights (static fallback)
  );

  EXPECT_EQ(decision.lexical_weight, 0.3f);
  EXPECT_EQ(decision.dense_weight, 0.5f);
  EXPECT_EQ(decision.graph_weight, 0.2f);
  EXPECT_NEAR(decision.lexical_weight + decision.dense_weight + 
              decision.graph_weight, 1.0f, 0.01f);
}

TEST_F(AdaptiveHybridRouterTest, WeightNormalization) {
  // Test with arbitrary policy weights that need normalization
  AdaptiveHybridRouter::RoutingDecision decision = router_.Route(
      "Some query",
      {1.0f, 2.0f, 3.0f}  // Raw weights that should normalize to sum=1
  );

  float sum = decision.lexical_weight + decision.dense_weight + 
              decision.graph_weight;
  EXPECT_NEAR(sum, 1.0f, 0.01f);
  EXPECT_GE(decision.dense_weight, decision.lexical_weight);  // 3x vs 1x
  EXPECT_GE(decision.dense_weight, decision.graph_weight);
}

TEST_F(AdaptiveHybridRouterTest, ZeroWeightHandling) {
  // Test with all zero weights (should fallback)
  AdaptiveHybridRouter::RoutingDecision decision = router_.Route(
      "Query",
      {0.0f, 0.0f, 0.0f}
  );

  EXPECT_EQ(decision.lexical_weight, 0.3f);
  EXPECT_EQ(decision.dense_weight, 0.5f);
  EXPECT_EQ(decision.graph_weight, 0.2f);
}

TEST_F(AdaptiveHybridRouterTest, RetrieveWithDocuments) {
  std::vector<AdaptiveHybridRouter::Document> docs = {
      {1, "doc1", 0.8f},
      {2, "doc2", 0.7f},
      {3, "doc3", 0.6f}
  };

  auto results = router_.RetrieveAdaptive(
      "query",
      docs,
      {0.0f, 0.0f, 0.0f}  // Use fallback weights
  );

  EXPECT_EQ(results.size(), docs.size());
  for (const auto& result : results) {
    EXPECT_TRUE(result.query_id.empty() || result.query_id == "query");
  }
}

TEST_F(AdaptiveHybridRouterTest, RRFFusionDeterministic) {
  // Same query and weights should produce same results
  auto decision1 = router_.Route("test query", {1.0f, 1.0f, 1.0f});
  auto decision2 = router_.Route("test query", {1.0f, 1.0f, 1.0f});

  EXPECT_EQ(decision1.lexical_weight, decision2.lexical_weight);
  EXPECT_EQ(decision1.dense_weight, decision2.dense_weight);
  EXPECT_EQ(decision1.graph_weight, decision2.graph_weight);
}

TEST_F(AdaptiveHybridRouterTest, LargeWeightValues) {
  // Test with large weight values (should normalize)
  AdaptiveHybridRouter::RoutingDecision decision = router_.Route(
      "query",
      {1000.0f, 2000.0f, 3000.0f}
  );

  EXPECT_NEAR(decision.lexical_weight, 1.0f / 6.0f, 0.01f);
  EXPECT_NEAR(decision.dense_weight, 2.0f / 6.0f, 0.01f);
  EXPECT_NEAR(decision.graph_weight, 3.0f / 6.0f, 0.01f);
}

TEST_F(AdaptiveHybridRouterTest, NegativeWeightHandling) {
  // Negative weights should be clamped to fallback
  AdaptiveHybridRouter::RoutingDecision decision = router_.Route(
      "query",
      {-1.0f, -1.0f, -1.0f}
  );

  EXPECT_EQ(decision.lexical_weight, 0.3f);
  EXPECT_EQ(decision.dense_weight, 0.5f);
  EXPECT_EQ(decision.graph_weight, 0.2f);
}

}  // namespace themis::rag

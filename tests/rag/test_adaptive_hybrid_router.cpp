// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <memory>

#include "rag/adaptive_hybrid_router.h"
#include "rag/router_policy_store.h"

namespace themis::rag {

class AdaptiveHybridRouterTest : public ::testing::Test {
 protected:
  AdaptiveHybridRouterTest()
      : store_(std::make_shared<RouterPolicyStore>("inmemory-router-policy")),
        router_(store_) {}

  static AdaptiveHybridRouter::QueryContext MakeContext(const std::string& qid,
                                                        const std::string& text,
                                                        QueryIntentClassifier::Intent intent) {
    AdaptiveHybridRouter::QueryContext ctx;
    ctx.query_id = qid;
    ctx.query_text = text;
    ctx.tenant_id = "tenant_a";
    ctx.intent.intent = intent;
    ctx.intent.confidence = 0.9f;
    ctx.intent.query_id = qid;
    ctx.intent.intent_scores = {{intent, 0.9f}};
    return ctx;
  }

  std::shared_ptr<RouterPolicyStore> store_;
  AdaptiveHybridRouter router_;
};

TEST_F(AdaptiveHybridRouterTest, RouteWithFallbackWeightsWhenNoPolicy) {
  const auto ctx = MakeContext("q1", "What is machine learning?", QueryIntentClassifier::Intent::Factual);
  const auto decision = router_.Route(ctx);

  EXPECT_NEAR(decision.lexical_weight, 0.3f, 1e-6f);
  EXPECT_NEAR(decision.dense_weight, 0.5f, 1e-6f);
  EXPECT_NEAR(decision.graph_weight, 0.2f, 1e-6f);
  EXPECT_TRUE(AdaptiveHybridRouter::ValidateWeights(decision));
}

TEST_F(AdaptiveHybridRouterTest, RouteWithStoredPolicy) {
  RouterPolicyStore::PolicySpec policy{};
  policy.version = 1;
  policy.intent = QueryIntentClassifier::Intent::Temporal;
  policy.lexical_weight = 0.2f;
  policy.dense_weight = 0.4f;
  policy.graph_weight = 0.4f;
  policy.created_at = "2026-09-27T00:00:00Z";
  policy.baseline_ndcg = 0.6f;
  policy.confidence = 0.8f;
  policy.description = "unit-test";
  ASSERT_TRUE(store_->UpdatePolicy(policy));

  const auto ctx = MakeContext("q2", "recent updates", QueryIntentClassifier::Intent::Temporal);
  const auto decision = router_.Route(ctx);
  EXPECT_NEAR(decision.lexical_weight, 0.2f, 1e-6f);
  EXPECT_NEAR(decision.dense_weight, 0.4f, 1e-6f);
  EXPECT_NEAR(decision.graph_weight, 0.4f, 1e-6f);
}

TEST_F(AdaptiveHybridRouterTest, InvalidStoredPolicyFallsBack) {
  RouterPolicyStore::PolicySpec policy{};
  policy.version = 2;
  policy.intent = QueryIntentClassifier::Intent::Comparison;
  policy.lexical_weight = 1.0f;
  policy.dense_weight = 1.0f;
  policy.graph_weight = 1.0f;
  policy.created_at = "2026-09-27T00:00:00Z";
  policy.baseline_ndcg = 0.6f;
  policy.confidence = 0.8f;
  policy.description = "invalid-sum";
  ASSERT_TRUE(store_->UpdatePolicy(policy));

  const auto ctx = MakeContext("q3", "python vs go", QueryIntentClassifier::Intent::Comparison);
  const auto decision = router_.Route(ctx);
  EXPECT_NEAR(decision.lexical_weight, 0.3f, 1e-6f);
  EXPECT_NEAR(decision.dense_weight, 0.5f, 1e-6f);
  EXPECT_NEAR(decision.graph_weight, 0.2f, 1e-6f);
}

TEST_F(AdaptiveHybridRouterTest, RetrieveAdaptiveReturnsStablePlaceholder) {
  const auto ctx = MakeContext("q4", "query", QueryIntentClassifier::Intent::Factual);
  const auto decision = router_.Route(ctx);
  auto results = router_.RetrieveAdaptive(ctx, decision);
  EXPECT_TRUE(results.empty());
}

TEST_F(AdaptiveHybridRouterTest, RouteAndRetrieveEndToEnd) {
  const auto ctx = MakeContext("q5", "end to end", QueryIntentClassifier::Intent::MultiHop);
  auto results = router_.RouteAndRetrieve(ctx);
  EXPECT_TRUE(results.empty());
}

TEST_F(AdaptiveHybridRouterTest, FallbackWeightsAreValid) {
  const auto fallback = AdaptiveHybridRouter::GetFallbackWeights();
  EXPECT_TRUE(AdaptiveHybridRouter::ValidateWeights(fallback));
}

}  // namespace themis::rag

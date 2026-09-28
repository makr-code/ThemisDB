// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/router_policy_store.h"

namespace themis::rag {

namespace {

RouterPolicyStore::PolicySpec MakePolicy(
    QueryIntentClassifier::Intent intent,
    uint32_t version,
    float lexical_weight,
    float dense_weight,
    float graph_weight) {
  RouterPolicyStore::PolicySpec policy{};
  policy.version = version;
  policy.intent = intent;
  policy.lexical_weight = lexical_weight;
  policy.dense_weight = dense_weight;
  policy.graph_weight = graph_weight;
  policy.created_at = "2026-01-01T00:00:00Z";
  policy.baseline_ndcg = 0.0f;
  policy.confidence = 1.0f;
  policy.description = "test policy";
  return policy;
}

}  // namespace

class RouterPolicyStoreTest : public ::testing::Test {
 protected:
  RouterPolicyStoreTest() : store_("in-memory-test") {}

  RouterPolicyStore store_;
};

TEST_F(RouterPolicyStoreTest, GetCurrentPolicyReturnsDefault) {
  auto policy = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Factual);

  ASSERT_TRUE(policy.has_value());
  EXPECT_EQ(policy->lexical_weight, 0.3f);
  EXPECT_EQ(policy->dense_weight, 0.5f);
  EXPECT_EQ(policy->graph_weight, 0.2f);
  EXPECT_EQ(policy->version, 0);
}

TEST_F(RouterPolicyStoreTest, UpdatePolicyAndRetrieve) {
  auto new_policy = MakePolicy(QueryIntentClassifier::Intent::Temporal,
                               1,
                               0.2f,
                               0.6f,
                               0.2f);

  EXPECT_TRUE(store_.UpdatePolicy(new_policy));

  auto retrieved = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Temporal);
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->lexical_weight, 0.2f);
  EXPECT_EQ(retrieved->dense_weight, 0.6f);
  EXPECT_EQ(retrieved->graph_weight, 0.2f);
}

TEST_F(RouterPolicyStoreTest, PolicyVersionIncrement) {
  auto policy1 = MakePolicy(QueryIntentClassifier::Intent::Factual,
                            1,
                            0.1f,
                            0.8f,
                            0.1f);

  EXPECT_TRUE(store_.UpdatePolicy(policy1));
  auto v1 = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Factual);
  ASSERT_TRUE(v1.has_value());
  const auto version1 = v1->version;

  auto policy2 = MakePolicy(QueryIntentClassifier::Intent::Factual,
                            2,
                            0.2f,
                            0.7f,
                            0.1f);

  EXPECT_TRUE(store_.UpdatePolicy(policy2));
  auto v2 = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Factual);
  ASSERT_TRUE(v2.has_value());

  EXPECT_GT(v2->version, version1);
}

TEST_F(RouterPolicyStoreTest, RollbackPolicy) {
  // Set initial policy
  auto initial = MakePolicy(QueryIntentClassifier::Intent::Factual,
                            1,
                            0.3f,
                            0.5f,
                            0.2f);
  EXPECT_TRUE(store_.UpdatePolicy(initial));

  // Update to new policy
  auto updated = MakePolicy(QueryIntentClassifier::Intent::Factual,
                            2,
                            0.6f,
                            0.2f,
                            0.2f);
  EXPECT_TRUE(store_.UpdatePolicy(updated));

  // Rollback
  EXPECT_TRUE(store_.RollbackPolicy(QueryIntentClassifier::Intent::Factual, 1));
  auto rolled_back = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Factual);
  ASSERT_TRUE(rolled_back.has_value());

  EXPECT_EQ(rolled_back->lexical_weight, 0.3f);
  EXPECT_EQ(rolled_back->dense_weight, 0.5f);
}

TEST_F(RouterPolicyStoreTest, GetPolicyHistory) {
  // Create a few policy versions
  for (int i = 0; i < 3; ++i) {
    auto policy = MakePolicy(QueryIntentClassifier::Intent::MultiHop,
                             static_cast<uint32_t>(i + 1),
                             0.1f * (i + 1),
                             0.9f - 0.1f * i,
                             0.0f);
    EXPECT_TRUE(store_.UpdatePolicy(policy));
  }

  auto history = store_.GetHistory(QueryIntentClassifier::Intent::MultiHop);
  
  EXPECT_GE(history.size(), 3);
  // Verify versions are in order (oldest to newest)
  for (size_t i = 1; i < history.size(); ++i) {
    EXPECT_GE(history[i].version, history[i-1].version);
  }
}

TEST_F(RouterPolicyStoreTest, RecordMetrics) {
  EXPECT_TRUE(store_.RecordMetrics("decision-1",
                                   QueryIntentClassifier::Intent::Factual,
                                   0.82f,
                                   45.5f,
                                   0.03f));

  auto metrics = store_.GetMetrics(QueryIntentClassifier::Intent::Factual);
  EXPECT_FALSE(metrics.has_value());
  EXPECT_TRUE(store_.IsHealthy());
}

TEST_F(RouterPolicyStoreTest, MultiIntentPolicies) {
  // Store different policies for different intents
  auto factual_policy = MakePolicy(QueryIntentClassifier::Intent::Factual,
                                   1,
                                   0.2f,
                                   0.8f,
                                   0.0f);
  auto temporal_policy = MakePolicy(QueryIntentClassifier::Intent::Temporal,
                                    1,
                                    0.7f,
                                    0.2f,
                                    0.1f);

  EXPECT_TRUE(store_.UpdatePolicy(factual_policy));
  EXPECT_TRUE(store_.UpdatePolicy(temporal_policy));

  auto factual = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Factual);
  auto temporal = store_.GetCurrentPolicy(QueryIntentClassifier::Intent::Temporal);
  ASSERT_TRUE(factual.has_value());
  ASSERT_TRUE(temporal.has_value());

  EXPECT_NE(factual->dense_weight, temporal->lexical_weight);
}

}  // namespace themis::rag

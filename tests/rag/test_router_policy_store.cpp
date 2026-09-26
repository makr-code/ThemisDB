// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/router_policy_store.h"

namespace themis::rag {

class RouterPolicyStoreTest : public ::testing::Test {
 protected:
  RouterPolicyStoreTest() {
    // Initialize policy store (would connect to RocksDB in production)
    // For testing, use in-memory mock
  }

  RouterPolicyStore store_;
};

TEST_F(RouterPolicyStoreTest, GetCurrentPolicyReturnsDefault) {
  auto policy = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Factual);
  
  EXPECT_EQ(policy.lexical_weight, 0.3f);
  EXPECT_EQ(policy.dense_weight, 0.5f);
  EXPECT_EQ(policy.graph_weight, 0.2f);
  EXPECT_EQ(policy.version, 0);  // No custom policy yet
}

TEST_F(RouterPolicyStoreTest, UpdatePolicyAndRetrieve) {
  RouterPolicyStore::RoutingPolicy new_policy;
  new_policy.lexical_weight = 0.2f;
  new_policy.dense_weight = 0.6f;
  new_policy.graph_weight = 0.2f;

  store_.UpdatePolicy(RouterPolicyStore::Intent::Temporal, new_policy);

  auto retrieved = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Temporal);
  EXPECT_EQ(retrieved.lexical_weight, 0.2f);
  EXPECT_EQ(retrieved.dense_weight, 0.6f);
  EXPECT_EQ(retrieved.graph_weight, 0.2f);
}

TEST_F(RouterPolicyStoreTest, PolicyVersionIncrement) {
  RouterPolicyStore::RoutingPolicy policy1;
  policy1.lexical_weight = 0.1f;
  
  store_.UpdatePolicy(RouterPolicyStore::Intent::Factual, policy1);
  auto v1 = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Factual);
  int version1 = v1.version;

  RouterPolicyStore::RoutingPolicy policy2;
  policy2.lexical_weight = 0.2f;
  
  store_.UpdatePolicy(RouterPolicyStore::Intent::Factual, policy2);
  auto v2 = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Factual);
  
  EXPECT_GT(v2.version, version1);
}

TEST_F(RouterPolicyStoreTest, RollbackPolicy) {
  // Set initial policy
  RouterPolicyStore::RoutingPolicy initial;
  initial.lexical_weight = 0.3f;
  initial.dense_weight = 0.5f;
  store_.UpdatePolicy(RouterPolicyStore::Intent::Factual, initial);

  // Update to new policy
  RouterPolicyStore::RoutingPolicy updated;
  updated.lexical_weight = 0.6f;
  updated.dense_weight = 0.2f;
  store_.UpdatePolicy(RouterPolicyStore::Intent::Factual, updated);

  // Rollback
  store_.RollbackPolicy(RouterPolicyStore::Intent::Factual, 1);
  auto rolled_back = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Factual);
  
  EXPECT_EQ(rolled_back.lexical_weight, 0.3f);
  EXPECT_EQ(rolled_back.dense_weight, 0.5f);
}

TEST_F(RouterPolicyStoreTest, GetPolicyHistory) {
  // Create a few policy versions
  for (int i = 0; i < 3; ++i) {
    RouterPolicyStore::RoutingPolicy policy;
    policy.lexical_weight = 0.1f * (i + 1);
    policy.dense_weight = 0.9f - 0.1f * i;
    store_.UpdatePolicy(RouterPolicyStore::Intent::MultiHop, policy);
  }

  auto history = store_.GetPolicyHistory(RouterPolicyStore::Intent::MultiHop);
  
  EXPECT_GE(history.size(), 3);
  // Verify versions are in order (oldest to newest)
  for (size_t i = 1; i < history.size(); ++i) {
    EXPECT_GE(history[i].version, history[i-1].version);
  }
}

TEST_F(RouterPolicyStoreTest, RecordMetrics) {
  RouterPolicyStore::PerformanceMetrics metrics;
  metrics.intent = RouterPolicyStore::Intent::Factual;
  metrics.avg_retrieval_time_ms = 45.5f;
  metrics.hit_rate = 0.87f;
  metrics.nDCG10 = 0.82f;

  store_.RecordMetrics(metrics);

  auto retrieved = store_.GetLatestMetrics(RouterPolicyStore::Intent::Factual);
  EXPECT_EQ(retrieved.intent, RouterPolicyStore::Intent::Factual);
  EXPECT_NEAR(retrieved.avg_retrieval_time_ms, 45.5f, 0.1f);
  EXPECT_NEAR(retrieved.hit_rate, 0.87f, 0.01f);
}

TEST_F(RouterPolicyStoreTest, MultiIntentPolicies) {
  // Store different policies for different intents
  RouterPolicyStore::RoutingPolicy factual_policy;
  factual_policy.dense_weight = 0.8f;
  store_.UpdatePolicy(RouterPolicyStore::Intent::Factual, factual_policy);

  RouterPolicyStore::RoutingPolicy temporal_policy;
  temporal_policy.lexical_weight = 0.7f;
  store_.UpdatePolicy(RouterPolicyStore::Intent::Temporal, temporal_policy);

  auto factual = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Factual);
  auto temporal = store_.GetCurrentPolicy(RouterPolicyStore::Intent::Temporal);

  EXPECT_NE(factual.dense_weight, temporal.lexical_weight);
}

}  // namespace themis::rag

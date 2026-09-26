// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/query_intent_classifier.h"

namespace themis::rag {

class QueryIntentClassifierTest : public ::testing::Test {
 protected:
  QueryIntentClassifier classifier_;
};

TEST_F(QueryIntentClassifierTest, ClassifyFactualIntent) {
  auto result = classifier_.Classify("Who invented the lightbulb?");
  EXPECT_EQ(result.intent, QueryIntentClassifier::Intent::Factual);
  EXPECT_GE(result.confidence, 0.7f);
}

TEST_F(QueryIntentClassifierTest, ClassifyTemporalIntent) {
  auto result = classifier_.Classify("COVID-19 cases in September 2026");
  EXPECT_EQ(result.intent, QueryIntentClassifier::Intent::Temporal);
  EXPECT_GE(result.confidence, 0.7f);
}

TEST_F(QueryIntentClassifierTest, ClassifyMultiHopIntent) {
  auto result = classifier_.Classify("Which authors wrote books banned in Texas?");
  EXPECT_EQ(result.intent, QueryIntentClassifier::Intent::MultiHop);
  EXPECT_GE(result.confidence, 0.7f);
}

TEST_F(QueryIntentClassifierTest, ClassifyComparisonIntent) {
  auto result = classifier_.Classify("What is the difference between Python and Go?");
  EXPECT_EQ(result.intent, QueryIntentClassifier::Intent::Comparison);
  EXPECT_GE(result.confidence, 0.7f);
}

TEST_F(QueryIntentClassifierTest, ConfidenceScoreSumToOne) {
  auto result = classifier_.Classify("Some random query");
  float sum = 0.0f;
  for (const auto& [intent, score] : result.intent_scores) {
    sum += score;
  }
  EXPECT_NEAR(sum, 1.0f, 0.01f);  // Softmax should sum to 1
}

TEST_F(QueryIntentClassifierTest, LowConfidenceFallsBackToFactual) {
  auto result = classifier_.Classify("asdfghjkl zxcvbnm qwerty");  // Gibberish
  EXPECT_EQ(result.intent, QueryIntentClassifier::Intent::Factual);
  EXPECT_GE(result.confidence, 0.7f);  // Fallback threshold
}

TEST_F(QueryIntentClassifierTest, BatchClassificationDeterministic) {
  std::vector<std::string> queries = {
      "Who is the president?",
      "What happened yesterday?",
      "Compare cats and dogs"
  };
  
  auto batch_results = classifier_.ClassifyBatch(queries);
  
  EXPECT_EQ(batch_results.size(), 3);
  for (size_t i = 0; i < queries.size(); ++i) {
    auto single_result = classifier_.Classify(queries[i]);
    EXPECT_EQ(batch_results[i].intent, single_result.intent);
  }
}

TEST_F(QueryIntentClassifierTest, GetRoutingHints) {
  // Factual: dense > lexical > graph
  auto hints_factual = QueryIntentClassifier::GetRoutingHints(
      QueryIntentClassifier::Intent::Factual);
  EXPECT_GT(hints_factual.dense_weight, hints_factual.lexical_weight);
  EXPECT_GT(hints_factual.lexical_weight, hints_factual.graph_weight);
  
  // Temporal: lexical ≈ graph > dense
  auto hints_temporal = QueryIntentClassifier::GetRoutingHints(
      QueryIntentClassifier::Intent::Temporal);
  EXPECT_GT(hints_temporal.lexical_weight, hints_temporal.dense_weight);
  EXPECT_GT(hints_temporal.graph_weight, hints_temporal.dense_weight);
  
  // MultiHop: dense + graph > lexical
  auto hints_multihop = QueryIntentClassifier::GetRoutingHints(
      QueryIntentClassifier::Intent::MultiHop);
  EXPECT_LT(hints_multihop.lexical_weight, hints_multihop.dense_weight);
  
  // Comparison: lexical ≈ dense > graph
  auto hints_comparison = QueryIntentClassifier::GetRoutingHints(
      QueryIntentClassifier::Intent::Comparison);
  EXPECT_GT(hints_comparison.lexical_weight, hints_comparison.graph_weight);
}

TEST_F(QueryIntentClassifierTest, IntentToString) {
  EXPECT_EQ(QueryIntentClassifier::IntentToString(
      QueryIntentClassifier::Intent::Factual), "factual");
  EXPECT_EQ(QueryIntentClassifier::IntentToString(
      QueryIntentClassifier::Intent::Temporal), "temporal");
  EXPECT_EQ(QueryIntentClassifier::IntentToString(
      QueryIntentClassifier::Intent::MultiHop), "multi_hop");
  EXPECT_EQ(QueryIntentClassifier::IntentToString(
      QueryIntentClassifier::Intent::Comparison), "comparison");
}

}  // namespace themis::rag

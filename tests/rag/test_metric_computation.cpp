// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <vector>

#include "rag/metric_computation.h"

namespace themis::rag::testing {

class MetricComputationTest : public ::testing::Test {
 protected:
  MetricComputationEngine engine_;
};

TEST_F(MetricComputationTest, ComputeNDCG) {
  std::vector<uint32_t> relevances = {3, 2, 1, 0, 0};
  std::vector<uint32_t> ideal_relevances = {3, 2, 1, 0, 0};
  
  float ndcg = engine_.ComputeNDCG(relevances, ideal_relevances);
  EXPECT_NEAR(ndcg, 1.0f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeMRR) {
  std::vector<uint32_t> relevances = {0, 0, 1, 0};
  float mrr = engine_.ComputeMRR(relevances, 1);
  
  EXPECT_NEAR(mrr, 0.333f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeMAP) {
  std::vector<uint32_t> relevances = {1, 0, 1, 0, 0};
  float map = engine_.ComputeMAP(relevances, 1);
  
  EXPECT_GT(map, 0.0f);
  EXPECT_LE(map, 1.0f);
}

TEST_F(MetricComputationTest, ComputePrecision) {
  std::vector<uint32_t> relevances = {1, 1, 0, 0};
  float precision = engine_.ComputePrecision(relevances, 4, 1);
  
  EXPECT_NEAR(precision, 0.5f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeRecall) {
  std::vector<uint32_t> relevances = {1, 1, 0, 0, 0};
  float recall = engine_.ComputeRecall(relevances, 3, 1);
  
  EXPECT_GT(recall, 0.0f);
  EXPECT_LE(recall, 1.0f);
}

TEST_F(MetricComputationTest, PerfectRanking) {
  std::vector<uint32_t> relevances = {3, 2, 1};
  std::vector<uint32_t> ideal = {3, 2, 1};
  
  float ndcg = engine_.ComputeNDCG(relevances, ideal);
  EXPECT_NEAR(ndcg, 1.0f, 0.001f);
}

TEST_F(MetricComputationTest, WorstRanking) {
  std::vector<uint32_t> relevances = {0, 0, 0};
  std::vector<uint32_t> ideal = {3, 2, 1};
  
  float ndcg = engine_.ComputeNDCG(relevances, ideal);
  EXPECT_NEAR(ndcg, 0.0f, 0.001f);
}

TEST_F(MetricComputationTest, SingleRelevantDoc) {
  std::vector<uint32_t> relevances = {1, 0, 0};
  float mrr = engine_.ComputeMRR(relevances, 1);
  
  EXPECT_NEAR(mrr, 1.0f, 0.001f);
}

TEST_F(MetricComputationTest, NoRelevantDocs) {
  std::vector<uint32_t> relevances = {0, 0, 0};
  float precision = engine_.ComputePrecision(relevances, 3, 1);
  
  EXPECT_NEAR(precision, 0.0f, 0.001f);
}

TEST_F(MetricComputationTest, BinaryRelevance) {
  std::vector<uint32_t> relevances = {1, 1, 0, 1, 0};
  float precision = engine_.ComputePrecision(relevances, 5, 1);
  
  EXPECT_NEAR(precision, 0.6f, 0.01f);
}

TEST_F(MetricComputationTest, GradedRelevance) {
  std::vector<uint32_t> relevances = {3, 2, 2, 1, 0};
  std::vector<uint32_t> ideal = {3, 3, 2, 2, 1};
  
  float ndcg = engine_.ComputeNDCG(relevances, ideal);
  EXPECT_GT(ndcg, 0.7f);
  EXPECT_LT(ndcg, 1.0f);
}

TEST_F(MetricComputationTest, ComputeAllMetrics) {
  std::vector<uint32_t> relevances = {1, 0, 1, 0};
  std::vector<uint32_t> ideal = {1, 1, 0, 0};
  
  auto metrics = engine_.ComputeAll(relevances, ideal, 1);
  EXPECT_GT(metrics.ndcg_at_10, 0.0f);
}

}  // namespace themis::rag::testing

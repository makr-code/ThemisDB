// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <map>
#include <vector>

#include "rag/metric_computation.h"

namespace themis::rag::testing {

class MetricComputationTest : public ::testing::Test {
 protected:
  MetricComputer engine_;

  static std::vector<std::string> Ranked() {
    return {"d1", "d2", "d3", "d4", "d5"};
  }

  static std::map<std::string, uint32_t> Relevance() {
    return {{"d1", 3}, {"d2", 2}, {"d3", 1}, {"d4", 0}, {"d5", 0}};
  }
};

TEST_F(MetricComputationTest, ComputeNDCG) {
  float ndcg = engine_.ComputeNDCG(Ranked(), Relevance(), 5);
  EXPECT_NEAR(ndcg, 1.0f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeMRR) {
  std::vector<std::string> ranked = {"x1", "x2", "d3", "x3"};
  std::map<std::string, uint32_t> relevance = {{"d3", 1}};
  float mrr = engine_.ComputeMRR(ranked, relevance, 10);
  EXPECT_NEAR(mrr, 1.0f / 3.0f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeMAP) {
  std::vector<std::string> ranked = {"d1", "x1", "d2", "x2", "d3"};
  std::map<std::string, uint32_t> relevance = {{"d1", 1}, {"d2", 1}, {"d3", 1}};
  float map = engine_.ComputeMAP(ranked, relevance, 5);
  EXPECT_GT(map, 0.0f);
  EXPECT_LE(map, 1.0f);
}

TEST_F(MetricComputationTest, ComputePrecision) {
  std::vector<std::string> ranked = {"d1", "d2", "x1", "x2"};
  std::map<std::string, uint32_t> relevance = {{"d1", 1}, {"d2", 1}};
  float precision = engine_.ComputePrecision(ranked, relevance, 4);
  EXPECT_NEAR(precision, 0.5f, 0.01f);
}

TEST_F(MetricComputationTest, ComputeRecall) {
  std::vector<std::string> ranked = {"d1", "x1", "d2"};
  std::map<std::string, uint32_t> relevance = {{"d1", 1}, {"d2", 1}, {"d3", 1}};
  float recall = engine_.ComputeRecall(ranked, relevance, 3);
  EXPECT_GT(recall, 0.0f);
  EXPECT_LE(recall, 1.0f);
}

TEST_F(MetricComputationTest, PerfectRanking) {
  std::vector<std::string> ranked = {"a", "b", "c"};
  std::map<std::string, uint32_t> rel = {{"a", 3}, {"b", 2}, {"c", 1}};
  float ndcg = engine_.ComputeNDCG(ranked, rel, 3);
  EXPECT_NEAR(ndcg, 1.0f, 0.001f);
}

TEST_F(MetricComputationTest, WorstRanking) {
  std::vector<std::string> ranked = {"x", "y", "z"};
  std::map<std::string, uint32_t> rel = {{"a", 3}, {"b", 2}, {"c", 1}};
  float ndcg = engine_.ComputeNDCG(ranked, rel, 3);
  EXPECT_NEAR(ndcg, 0.0f, 0.001f);
}

TEST_F(MetricComputationTest, SingleRelevantDoc) {
  std::vector<std::string> ranked = {"d1", "x", "y"};
  std::map<std::string, uint32_t> rel = {{"d1", 1}};
  float mrr = engine_.ComputeMRR(ranked, rel, 3);
  EXPECT_NEAR(mrr, 1.0f, 0.001f);
}

TEST_F(MetricComputationTest, NoRelevantDocs) {
  std::vector<std::string> ranked = {"x", "y", "z"};
  std::map<std::string, uint32_t> rel = {};
  float precision = engine_.ComputePrecision(ranked, rel, 3);
  EXPECT_NEAR(precision, 0.0f, 0.001f);
}

TEST_F(MetricComputationTest, BinaryRelevance) {
  std::vector<std::string> ranked = {"d1", "d2", "x1", "d3", "x2"};
  std::map<std::string, uint32_t> rel = {{"d1", 1}, {"d2", 1}, {"d3", 1}};
  float precision = engine_.ComputePrecision(ranked, rel, 5);
  EXPECT_NEAR(precision, 0.6f, 0.01f);
}

TEST_F(MetricComputationTest, GradedRelevance) {
  std::vector<std::string> ranked = {"a", "b", "c", "d", "e"};
  std::map<std::string, uint32_t> rel = {{"a", 3}, {"b", 2}, {"c", 2}, {"d", 1}, {"e", 0}};
  float ndcg = engine_.ComputeNDCG(ranked, rel, 5);
  EXPECT_GT(ndcg, 0.7f);
  EXPECT_LE(ndcg, 1.0f);
}

TEST_F(MetricComputationTest, ComputeAllMetrics) {
  std::vector<std::string> ranked = {"d1", "x1", "d2", "x2"};
  std::map<std::string, uint32_t> rel = {{"d1", 1}, {"d2", 1}};
  auto metrics = engine_.ComputeAll(ranked, rel);
  EXPECT_GT(metrics.ndcg_10, 0.0f);
}

}  // namespace themis::rag::testing

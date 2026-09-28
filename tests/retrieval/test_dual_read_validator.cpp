/**
 * @file test_dual_read_validator.cpp
 * @brief Tests for DualReadValidator query-time metric collection
 *
 * Tests metric computation, regression detection, and batch validation.
 * Test cases: DUALREAD-01..DUALREAD-06
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <vector>
#include <map>

#include "retrieval/dual_read_validator.h"

using namespace themis::retrieval;

class DualReadValidatorTest : public ::testing::Test {
 protected:
  DualReadValidator validator_{"/tmp/current", "/tmp/previous"};
};

/**
 * @test DUALREAD-01: Detect regression (recall drop > 2pp)
 */
TEST_F(DualReadValidatorTest, DUALREAD_01_DetectRegression) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.80;  // 80% recall
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;  // 85% recall (5pp better)
  previous.ndcg_at_10 = 0.78;
  previous.mrr_at_10 = 0.92;

  // Delta is 5pp > 2pp tolerance → regression detected
  EXPECT_TRUE(current.HasRegression(previous, 2.0));
}

/**
 * @test DUALREAD-02: Accept small variance (< 2pp)
 */
TEST_F(DualReadValidatorTest, DUALREAD_02_AcceptSmallVariance) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.84;  // 84% recall
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;  // 85% recall (1pp better)
  previous.ndcg_at_10 = 0.76;
  previous.mrr_at_10 = 0.91;

  // Delta is 1pp < 2pp tolerance → no regression
  EXPECT_FALSE(current.HasRegression(previous, 2.0));
}

TEST_F(DualReadValidatorTest, ValidateQueryReturnsCachedGroundTruthMetrics) {
  std::map<std::string, std::vector<std::string>> judgments;
  judgments["query1"] = {"doc1", "doc3", "doc5"};
  validator_.SetGroundTruth(judgments);

  auto result = validator_.ValidateQuery("query1", 10);
  EXPECT_EQ(result.metrics.total_relevant, 3u);
  EXPECT_DOUBLE_EQ(result.metrics.recall_at_10, 0.0);
  EXPECT_DOUBLE_EQ(result.metrics.ndcg_at_10, 0.0);
  EXPECT_DOUBLE_EQ(result.metrics.mrr_at_10, 0.0);
  EXPECT_TRUE(result.current_results.empty());
  EXPECT_TRUE(result.previous_results.empty());
}

TEST_F(DualReadValidatorTest, ValidateQueryBatchAggregatesEmptyResults) {
  std::map<std::string, std::vector<std::string>> judgments;
  judgments["query1"] = {"doc1", "doc2"};
  judgments["query2"] = {"doc3"};
  validator_.SetGroundTruth(judgments);

  auto batch = validator_.ValidateQueryBatch({"query1", "query2"}, 5);
  EXPECT_EQ(batch.query_results.size(), 2u);
  EXPECT_DOUBLE_EQ(batch.aggregate_metrics.recall_at_10, 0.0);
  EXPECT_DOUBLE_EQ(batch.aggregate_metrics.ndcg_at_10, 0.0);
  EXPECT_DOUBLE_EQ(batch.aggregate_metrics.mrr_at_10, 0.0);
  EXPECT_GE(batch.total_time.count(), 0);
}

/**
 * @test DUALREAD-06: Compute metric deltas correctly
 */
TEST_F(DualReadValidatorTest, DUALREAD_06_ComputeDeltas) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.80;
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;
  previous.ndcg_at_10 = 0.78;
  previous.mrr_at_10 = 0.92;

  auto deltas = current.ComputeDeltas(previous);
  EXPECT_EQ(deltas.size(), 3);
  EXPECT_NEAR(deltas[0], 0.05, 0.001);   // recall delta
  EXPECT_NEAR(deltas[1], 0.03, 0.001);   // ndcg delta
  EXPECT_NEAR(deltas[2], 0.02, 0.001);   // mrr delta
}

/**
 * @test Set and retrieve ground truth
 */
TEST_F(DualReadValidatorTest, GroundTruthCaching) {
  std::map<std::string, std::vector<std::string>> judgments;
  judgments["query1"] = {"doc1", "doc3", "doc5"};
  judgments["query2"] = {"doc2", "doc4"};

  validator_.SetGroundTruth(judgments);

  auto gt1 = validator_.GetGroundTruth("query1");
  EXPECT_EQ(gt1.size(), 3);
  EXPECT_EQ(gt1[0], "doc1");

  auto gt2 = validator_.GetGroundTruth("query2");
  EXPECT_EQ(gt2.size(), 2);
}
TEST_F(DualReadValidatorTest, RegressionBoundary) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.83;  // 83%
  current.ndcg_at_10 = 0.76;
  current.mrr_at_10 = 0.91;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.84;  // 84% (1pp better)
  previous.ndcg_at_10 = 0.77;
  previous.mrr_at_10 = 0.92;

  // 1pp delta should not be flagged as regression
  EXPECT_FALSE(current.HasRegression(previous, 2.0));

  // Slightly more than 2pp
  current.recall_at_10 = 0.819;  // 81.9%
  // Now delta is 84 - 81.9 = 2.1pp > 2pp → regression
  EXPECT_TRUE(current.HasRegression(previous, 2.0));
}
